#ifndef MATCH_H
#define MATCH_H

#include <chrono>

#include "game_info.h"
#include "game_loop.h"
#include "match_command.h"
#include "queue.h"
#include "world.h"

class Match {
private:
    const game::GameInfo game_info;
    game::World model;
    Queue<MatchCommand> commands;
    GameLoop game_loop;
    bool started = false;
    bool joined = false;
    bool stopped = false;

public:
    Match(game::Map map, game::Position spawn_position, game::WorldConfig config,
          unsigned int command_capacity, std::chrono::milliseconds tick_interval);

    ~Match();
    void start();
    void stop();
    void join();
    void rethrow_if_failed() const;

    Queue<MatchCommand>& get_commands();
};

#endif
