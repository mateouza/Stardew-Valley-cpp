#ifndef CLIENT_SESSION_H
#define CLIENT_SESSION_H

#include <exception>
#include <memory>
#include <string>

#include "client_receiver.h"
#include "match_command.h"
#include "queue.h"
#include "server_protocol.h"
#include "session_channel.h"
#include "socket.h"

class ClientSession {
private:
    Socket socket;
    ServerProtocol protocol;
    std::shared_ptr<SessionChannel> channel;
    std::unique_ptr<ClientReceiver> receiver;
    std::exception_ptr failure;

    bool started = false;
    bool stopped = false;
    bool joined = false;

public:
    explicit ClientSession(Socket&& socket);
    ~ClientSession();

    ClientSession(const ClientSession&) = delete;
    ClientSession& operator=(const ClientSession&) = delete;

    void start(Queue<MatchCommand>& commands, const std::string& name,
               std::shared_ptr<SessionChannel> channel);
    void stop();
    void join();
    bool has_finished() const;
    void rethrow_if_failed() const;
};

#endif