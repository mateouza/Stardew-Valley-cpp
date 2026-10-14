#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include <exception>

#include "socket.h"
#include "thread.h"

class Acceptor: public Thread {
private:
    Socket server_socket;
    std::exception_ptr failure;

    void run() override;

public:
    explicit Acceptor(const char* service);

    void stop() override;
    void rethrow_if_failed() const;
};

#endif
