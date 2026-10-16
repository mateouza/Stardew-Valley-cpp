#ifndef GAME_LOOP_H
#define GAME_LOOP_H

#include <chrono>
#include <exception>

#include "game_model.h"
#include "player_command.h"
#include "queue.h"
#include "thread.h"

class GameLoop: public Thread {
private:
    game::GameModel& model;
    Queue<PlayerCommand>& commands;
    std::chrono::milliseconds tick_interval;
    std::exception_ptr failure;

    void process_commands();
    void run() override;

public:
    GameLoop(game::GameModel& model, Queue<PlayerCommand>& commands,
             std::chrono::milliseconds tick_interval);

    void rethrow_if_failed() const;
};

#endif
