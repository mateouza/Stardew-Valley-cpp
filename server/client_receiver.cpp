#include "client_receiver.h"

#include <stdexcept>

#include "liberror.h"

ClientReceiver::ClientReceiver(ServerProtocol& protocol, Queue<PlayerCommand>& commands,
                               game::PlayerId player_id):
        protocol(protocol), commands(commands), player_id(player_id) {}

void ClientReceiver::run() {
    try {
        while (should_keep_running()) {
            auto request = protocol.receive_move();

            if (!request || !should_keep_running()) {
                break;
            }

            PlayerCommand command{player_id, *request};

            if (!commands.try_push(command)) {
                throw std::runtime_error("La cola de comandos de la partida esta llena");
            }
        }
    } catch (const ClosedQueue&) {
    } catch (const LibError&) {
        if (should_keep_running()) {
            failure = std::current_exception();
        }
    } catch (...) {
        failure = std::current_exception();
    }
}

void ClientReceiver::rethrow_if_failed() const {
    if (failure) {
        std::rethrow_exception(failure);
    }
}
