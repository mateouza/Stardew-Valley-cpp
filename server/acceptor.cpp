#include "acceptor.h"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

#include <sys/socket.h>

#include "liberror.h"

Acceptor::Acceptor(const char* service, MatchManager& matches, unsigned int outgoing_capacity):
        server_socket(service), matches(matches), outgoing_capacity(outgoing_capacity) {
    if (outgoing_capacity == 0) {
        throw std::invalid_argument("La capacidad de salida debe ser mayor que cero");
    }
}

void Acceptor::finish_handler(ClientHandler& handler) {
    handler.stop();
    handler.join();

    try {
        handler.rethrow_if_failed();
    } catch (const std::exception& error) {
        std::cerr << "Error de conexion: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "Error desconocido de conexion\n";
    }
}

void Acceptor::reap_handlers() {
    auto handler = handlers.begin();

    while (handler != handlers.end()) {
        if (!(*handler)->is_alive()) {
            finish_handler(**handler);
            handler = handlers.erase(handler);
        } else {
            ++handler;
        }
    }
}

void Acceptor::run() {
    try {
        while (should_keep_running()) {
            Socket client_socket = server_socket.accept();

            if (!should_keep_running()) {
                break;
            }

            reap_handlers();

            if (next_session_id == std::numeric_limits<SessionId>::max()) {
                throw std::overflow_error("Se agotaron los identificadores de sesion");
            }

            auto handler = std::make_unique<ClientHandler>(std::move(client_socket), matches,
                                                           next_session_id, outgoing_capacity);

            handlers.push_back(std::move(handler));
            handlers.back()->start();

            ++next_session_id;
        }
    } catch (const LibError&) {
        if (should_keep_running()) {
            failure = std::current_exception();
        }
    } catch (...) {
        failure = std::current_exception();
    }

    for (const auto& handler: handlers) {
        handler->stop();
    }

    for (const auto& handler: handlers) {
        finish_handler(*handler);
    }

    handlers.clear();
}

void Acceptor::stop() {
    Thread::stop();
    server_socket.shutdown(SHUT_RDWR);
}

void Acceptor::rethrow_if_failed() const {
    if (failure) {
        std::rethrow_exception(failure);
    }
}