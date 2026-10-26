#include "client_sender.h"

#include <stdexcept>
#include <utility>
#include <variant>

#include "liberror.h"

ClientSender::ClientSender(ServerProtocol& protocol, std::shared_ptr<SessionChannel> channel):
        protocol(protocol), channel(std::move(channel)) {
    if (!this->channel) {
        throw std::invalid_argument("El emisor debe tener un canal de sesion");
    }
}

void ClientSender::send_message(const ServerMessage& message) {
    if (const auto* response = std::get_if<JoinResponse>(&message)) {
        protocol.send_join_response(*response);

        if (std::holds_alternative<game::AddPlayerError>(response->result)) {
            channel->request_close();
        }

        return;
    }

    if (const auto* response = std::get_if<MatchResponse>(&message)) {
        protocol.send_match_response(*response);
        return;
    }

    protocol.send_game_state(std::get<game::GameState>(message));
}

void ClientSender::run() {
    try {
        while (should_keep_running() && !channel->is_closing()) {
            const auto message = channel->get_outgoing_queue().pop();

            if (!should_keep_running() || channel->is_closing()) {
                break;
            }

            send_message(message);
        }
    } catch (const ClosedQueue&) {
    } catch (const LibError&) {
        if (should_keep_running() && !channel->is_closing()) {
            failure = std::current_exception();
        }
    } catch (...) {
        failure = std::current_exception();
    }

    channel->request_close();
}

void ClientSender::stop() {
    Thread::stop();
    channel->request_close();
}

void ClientSender::rethrow_if_failed() const {
    if (failure) {
        std::rethrow_exception(failure);
    }
}