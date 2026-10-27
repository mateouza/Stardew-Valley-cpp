#ifndef CLIENT_SENDER_H
#define CLIENT_SENDER_H

#include <exception>
#include <memory>

#include "server_message.h"
#include "server_protocol.h"
#include "session_channel.h"
#include "thread.h"

class ClientSender: public Thread {
private:
    ServerProtocol& protocol;
    std::shared_ptr<SessionChannel> channel;
    std::exception_ptr failure;

    void send_message(const ServerMessage& message);
    void run() override;

public:
    ClientSender(ServerProtocol& protocol, std::shared_ptr<SessionChannel> channel);

    void stop() override;
    void rethrow_if_failed() const;
};

#endif