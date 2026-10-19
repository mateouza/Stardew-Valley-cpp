#ifndef SERVER_H
#define SERVER_H

#include "acceptor.h"

class Server {
private:
    Acceptor acceptor;

public:
    explicit Server(const char* service);

    void run();
};

#endif
