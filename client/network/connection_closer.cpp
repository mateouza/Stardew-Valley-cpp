#include "connection_closer.h"

#include <sys/socket.h>

ConnectionCloser::ConnectionCloser(Socket& socket, Queue<ClientRequest>& requests,
                                   Queue<ServerMessage>& messages):
        socket(socket), requests(requests), messages(messages) {}

void ConnectionCloser::close(const std::exception_ptr& cause) {
    std::lock_guard<std::mutex> lock(mutex);

    if (closed) {
        return;
    }

    closed = true;
    failure = cause;

    requests.close();
    messages.close();

    try {
        socket.shutdown(SHUT_RDWR);
    } catch (...) {
        if (!failure) {
            failure = std::current_exception();
        }
    }
}

bool ConnectionCloser::is_closed() const {
    std::lock_guard<std::mutex> lock(mutex);
    return closed;
}

void ConnectionCloser::rethrow_if_failed() const {
    std::lock_guard<std::mutex> lock(mutex);

    if (failure) {
        std::rethrow_exception(failure);
    }
}
