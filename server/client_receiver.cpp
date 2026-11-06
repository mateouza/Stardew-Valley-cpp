#include "client_receiver.h"

#include <stdexcept>
#include <utility>
#include <variant>

#include "liberror.h"
#include "player_command.h"
#include "select_slot_command.h"

ClientReceiver::ClientReceiver(ServerProtocol& protocol, Queue<MatchCommand>& commands,
                               std::shared_ptr<SessionChannel> channel):
        protocol(protocol), commands(commands), channel(std::move(channel)) {
    if (!this->channel) {
        throw std::invalid_argument("El receptor debe tener un canal de sesion");
    }
}

void ClientReceiver::run() {
    try {
        while (should_keep_running() && !channel->is_closing()) {
            const auto request = protocol.receive_game_request();

            if (!request || !should_keep_running() || channel->is_closing()) {
                break;
            }

            MatchCommand command;

            if (const auto* movement = std::get_if<game::MoveRequest>(&*request)) {
                command = PlayerCommand{channel->get_session_id(), *movement};
            } else {
                command = SelectSlotCommand{
                        channel->get_session_id(), std::get<game::SelectSlotRequest>(*request)};
            }

            if (!commands.try_push(command)) {
                throw std::runtime_error("La cola de comandos de la partida esta llena");
            }
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

void ClientReceiver::rethrow_if_failed() const {
    if (failure) {
        std::rethrow_exception(failure);
    }
}