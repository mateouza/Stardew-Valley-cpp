#ifndef NETWORK_SENDER_H
#define NETWORK_SENDER_H

#include "client_protocol.h"
#include "client_request.h"
#include "connection_closer.h"
#include "queue.h"
#include "thread.h"

class NetworkSender: public Thread {
private:
    ClientProtocol& protocol;
    Queue<ClientRequest>& requests;
    ConnectionCloser& closer;

    void send_request(const ClientRequest& request);
    void run() override;

public:
    NetworkSender(ClientProtocol& protocol, Queue<ClientRequest>& requests,
                  ConnectionCloser& closer);
};

#endif
