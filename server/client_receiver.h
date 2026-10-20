#ifndef CLIENT_RECEIVER_H
#define CLIENT_RECEIVER_H

#include <exception>

#include "player_command.h"
#include "queue.h"
#include "server_protocol.h"
#include "thread.h"

class ClientReceiver: public Thread {
private:
    ServerProtocol& protocol;
    Queue<PlayerCommand>& commands;
    game::PlayerId player_id;
    std::exception_ptr failure;

    void run() override;

public:
    ClientReceiver(ServerProtocol& protocol, Queue<PlayerCommand>& commands,
                   game::PlayerId player_id);

    void rethrow_if_failed() const;
};

#endif
