#ifndef CLIENT_RECEIVER_H
#define CLIENT_RECEIVER_H

#include <exception>
#include <memory>

#include "match_command.h"
#include "queue.h"
#include "server_protocol.h"
#include "session_channel.h"
#include "thread.h"

class ClientReceiver: public Thread {
private:
    ServerProtocol& protocol;
    Queue<MatchCommand>& commands;
    std::shared_ptr<SessionChannel> channel;
    std::exception_ptr failure;

    void run() override;

public:
    ClientReceiver(ServerProtocol& protocol, Queue<MatchCommand>& commands,
                   std::shared_ptr<SessionChannel> channel);

    void rethrow_if_failed() const;
};

#endif