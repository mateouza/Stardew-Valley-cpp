#include "acceptor.h"

#include <utility>

#include <sys/socket.h>

#include "liberror.h"

Acceptor::Acceptor(const char* service): server_socket(service) {}

void Acceptor::run() {
    try {
        while (should_keep_running()) {
            Socket client_socket = server_socket.accept();
            auto session = std::make_unique<ClientSession>(std::move(client_socket));
            sessions.push_back(std::move(session));
        }
    } catch (const LibError&) {
        if (should_keep_running()) {
            failure = std::current_exception();
        }
    } catch (...) {
        failure = std::current_exception();
    }
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
