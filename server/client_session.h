#ifndef CLIENT_SESSION_H
#define CLIENT_SESSION_H

#include "socket.h"

class ClientSession {
private:
    Socket socket;

public:
    explicit ClientSession(Socket&& socket);

    ClientSession(const ClientSession&) = delete;
    ClientSession& operator=(const ClientSession&) = delete;
};

#endif