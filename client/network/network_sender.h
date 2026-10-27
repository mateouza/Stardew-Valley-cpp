#ifndef NETWORK_SENDER_H
#define NETWORK_SENDER_H

#include <exception>
#include <mutex>

#include "client_protocol.h"
#include "client_request.h"
#include "queue.h"
#include "thread.h"

class NetworkSender: public Thread {
private:
    ClientProtocol& protocol;
    Queue<ClientRequest>& requests;
    std::exception_ptr failure;

    std::mutex close_mutex;
    bool queue_closed = false;

    void send_request(const ClientRequest& request);
    void run() override;

public:
    NetworkSender(ClientProtocol& protocol, Queue<ClientRequest>& requests);

    void stop() override;
    void rethrow_if_failed() const;
};

#endif
