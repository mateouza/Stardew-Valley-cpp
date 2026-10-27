#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include <exception>
#include <memory>
#include <vector>

#include "client_handler.h"
#include "match_manager.h"
#include "session_id.h"
#include "socket.h"
#include "thread.h"

class Acceptor: public Thread {
private:
    Socket server_socket;
    MatchManager& matches;
    unsigned int outgoing_capacity;
    std::vector<std::unique_ptr<ClientHandler>> handlers;
    SessionId next_session_id = 1;
    std::exception_ptr failure;

    void run() override;
    void reap_handlers();
    void finish_handler(ClientHandler& handler);

public:
    Acceptor(const char* service, MatchManager& matches, unsigned int outgoing_capacity);

    void stop() override;
    void rethrow_if_failed() const;
};

#endif