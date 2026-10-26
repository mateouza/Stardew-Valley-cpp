#ifndef NETWORK_RECEIVER_H
#define NETWORK_RECEIVER_H

#include <exception>
#include <mutex>

#include "client_protocol.h"
#include "queue.h"
#include "server_message.h"
#include "thread.h"

class NetworkReceiver: public Thread {
private:
    ClientProtocol& protocol;
    Queue<ServerMessage>& messages;
    std::exception_ptr failure;

    std::mutex close_mutex;
    bool queue_closed = false;

    void run() override;

public:
    NetworkReceiver(ClientProtocol& protocol, Queue<ServerMessage>& messages);

    void stop() override;
    void rethrow_if_failed() const;
};

#endif
