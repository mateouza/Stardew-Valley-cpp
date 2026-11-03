#ifndef CONNECTION_CLOSER_H
#define CONNECTION_CLOSER_H

#include <exception>
#include <mutex>

#include "client_request.h"
#include "queue.h"
#include "server_message.h"
#include "socket.h"

class ConnectionCloser {
private:
    Socket& socket;
    Queue<ClientRequest>& requests;
    Queue<ServerMessage>& messages;

    mutable std::mutex mutex;
    bool closed = false;
    std::exception_ptr failure;

public:
    ConnectionCloser(Socket& socket, Queue<ClientRequest>& requests,
                     Queue<ServerMessage>& messages);

    // Solo cuenta la causa del primer cierre.
    void close(const std::exception_ptr& cause = nullptr);
    bool is_closed() const;
    void rethrow_if_failed() const;
};

#endif
