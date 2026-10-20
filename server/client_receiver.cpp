#include "client_receiver.h"

#include <stdexcept>
#include <utility>

#include "liberror.h"
#include "player_command.h"

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
            const auto request = protocol.receive_move();

            if (!request || !should_keep_running() || channel->is_closing()) {
                break;
            }

            const PlayerCommand command{channel->get_session_id(), *request};

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