#include "server.h"

#include <exception>
#include <iostream>
#include <utility>

Server::Server(const char* service, game::Map map, game::Position spawn_position,
               game::WorldConfig config):
        matches(std::move(map), spawn_position, config, COMMAND_CAPACITY, TICK_INTERVAL),
        acceptor(service, matches, OUTGOING_CAPACITY) {}

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

    try {
        acceptor.rethrow_if_failed();
    } catch (...) {
        if (!failure) {
            failure = std::current_exception();
        }
    }

    matches.stop();

    try {
        matches.rethrow_if_failed();
    } catch (...) {
        if (!failure) {
            failure = std::current_exception();
        }
    }

    if (failure) {
        std::rethrow_exception(failure);
    }
}