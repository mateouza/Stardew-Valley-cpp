#include "server.h"

#include <exception>
#include <iostream>

Server::Server(const char* service): acceptor(service) {}

void Server::run() {
    acceptor.start();

    std::exception_ptr failure;

    try {
        char command;
        while (std::cin >> command && command != 'q') {}
    } catch (...) {
        failure = std::current_exception();
    }

    try {
        acceptor.stop();
    } catch (...) {
        if (!failure) {
            failure = std::current_exception();
        }
    }

    acceptor.join();

    if (failure) {
        std::rethrow_exception(failure);
    }

    acceptor.rethrow_if_failed();
}
