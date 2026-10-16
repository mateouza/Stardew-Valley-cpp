#ifndef CLIENT_SESSION_H
#define CLIENT_SESSION_H

#include "server_protocol.h"
#include "socket.h"

class ClientSession {
private:
    Socket socket;
    ServerProtocol protocol;

public:
    explicit ClientSession(Socket&& socket);

    ClientSession(const ClientSession&) = delete;
    ClientSession& operator=(const ClientSession&) = delete;
};

#endif