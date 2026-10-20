#ifndef GAME_LOOP_H
#define GAME_LOOP_H

#include <chrono>
#include <exception>
#include <mutex>

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
    std::mutex close_mutex;
    bool commands_closed = false;

    void process_commands();
    void run() override;
    void close_commands();

public:
    GameLoop(game::GameModel& model, Queue<PlayerCommand>& commands,
             std::chrono::milliseconds tick_interval);

    void stop() override;
    void rethrow_if_failed() const;
};

#endif
