#include "network_sender.h"

#include <variant>

#include "liberror.h"

NetworkSender::NetworkSender(ClientProtocol& protocol, Queue<ClientRequest>& requests):
        protocol(protocol), requests(requests) {}

void NetworkSender::send_request(const ClientRequest& request) {
    if (const auto* create = std::get_if<CreateMatchRequest>(&request)) {
        protocol.send_create_match(*create);
        return;
    }

    if (const auto* join = std::get_if<JoinMatchRequest>(&request)) {
        protocol.send_join_match(*join);
        return;
    }

    protocol.send_move(std::get<game::MoveRequest>(request));
}

void NetworkSender::run() {
    try {
        while (should_keep_running()) {
            const auto request = requests.pop();

            if (!should_keep_running()) {
                break;
            }

            send_request(request);
        }
    } catch (const ClosedQueue&) {
    } catch (const LibError&) {
        if (should_keep_running()) {
            failure = std::current_exception();
        }
    } catch (...) {
        failure = std::current_exception();
    }

    NetworkSender::stop();
}

void NetworkSender::stop() {
    Thread::stop();

    std::lock_guard<std::mutex> lock(close_mutex);

    if (!queue_closed) {
        requests.close();
        queue_closed = true;
    }
}

void NetworkSender::rethrow_if_failed() const {
    if (failure) {
        std::rethrow_exception(failure);
    }
}
