#include "network_sender.h"

#include <exception>
#include <variant>

NetworkSender::NetworkSender(ClientProtocol& protocol, Queue<ClientRequest>& requests,
                             ConnectionCloser& closer):
        protocol(protocol), requests(requests), closer(closer) {}

void NetworkSender::send_request(const ClientRequest& request) {
    if (const auto* create = std::get_if<CreateMatchRequest>(&request)) {
        protocol.send_create_match(*create);
        return;
    }

    if (const auto* join = std::get_if<JoinMatchRequest>(&request)) {
        protocol.send_join_match(*join);
        return;
    }

    if (const auto* selection = std::get_if<game::SelectSlotRequest>(&request)) {
        protocol.send_select_slot(*selection);
        return;
    }
    if (const auto* use_cell = std::get_if<game::UseCellRequest>(&request)) {
        protocol.send_use_cell(*use_cell);
        return;
    }
    if (std::holds_alternative<game::CheatSeedsRequest>(request)) {
        protocol.send_cheat_seeds();
        return;
    }

    protocol.send_move(std::get<game::MoveRequest>(request));
}

void NetworkSender::run() {
    std::exception_ptr error;

    try {
        while (should_keep_running()) {
            const auto request = requests.pop();

            if (!should_keep_running()) {
                break;
            }

            send_request(request);
        }
    } catch (const ClosedQueue&) {
    } catch (...) {
        error = std::current_exception();
    }

    closer.close(error);
}
