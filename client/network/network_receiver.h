#ifndef NETWORK_RECEIVER_H
#define NETWORK_RECEIVER_H

#include "client_protocol.h"
#include "connection_closer.h"
#include "queue.h"
#include "server_message.h"
#include "thread.h"

class NetworkReceiver: public Thread {
private:
    ClientProtocol& protocol;
    Queue<ServerMessage>& messages;
    ConnectionCloser& closer;

    void run() override;

public:
    NetworkReceiver(ClientProtocol& protocol, Queue<ServerMessage>& messages,
                    ConnectionCloser& closer);
};

#endif
