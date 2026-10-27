#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <exception>

#include "client_session.h"
#include "match_manager.h"
#include "session_id.h"
#include "socket.h"
#include "thread.h"

class ClientHandler: public Thread {
private:
    MatchManager& matches;
    ClientSession session;
    SessionId session_id;
    unsigned int outgoing_capacity;

    std::exception_ptr failure;
    std::exception_ptr shutdown_failure;

    bool started = false;
    bool stopped = false;
    bool joined = false;

    void run() override;

public:
    ClientHandler(Socket&& socket, MatchManager& matches, SessionId session_id,
                  unsigned int outgoing_capacity);

    ~ClientHandler();

    void start();
    void stop() override;
    void join();
    void rethrow_if_failed() const;
};

#endif
