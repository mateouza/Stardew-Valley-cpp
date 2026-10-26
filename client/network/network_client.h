#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <exception>
#include <optional>

#include "client_protocol.h"
#include "client_request.h"
#include "network_receiver.h"
#include "network_sender.h"
#include "queue.h"
#include "server_message.h"
#include "socket.h"

class NetworkClient {
private:
    Socket socket;
    ClientProtocol protocol;

    Queue<ClientRequest> requests;
    Queue<ServerMessage> messages;

    NetworkSender sender;
    NetworkReceiver receiver;

    std::exception_ptr failure;

    bool started = false;
    bool stopped = false;
    bool sender_joinable = false;
    bool receiver_joinable = false;

public:
    NetworkClient(Socket&& socket, unsigned int request_capacity, unsigned int message_capacity);

    NetworkClient(const char* hostname, const char* service, unsigned int request_capacity,
                  unsigned int message_capacity);

    ~NetworkClient();

    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;

    void start();
    bool try_send(const ClientRequest& request);
    std::optional<ServerMessage> try_receive();

    bool has_finished() const;
    void stop();
    void join();
    void rethrow_if_failed() const;
};

#endif
