#ifndef GAME_LOOP_H
#define GAME_LOOP_H

#include <chrono>
#include <exception>
#include <map>
#include <memory>
#include <mutex>

#include "game_info.h"
#include "game_model.h"
#include "match_command.h"
#include "queue.h"
#include "session_channel.h"
#include "session_id.h"
#include "thread.h"

class GameLoop: public Thread {
private:
    struct PlayerSession {
        game::PlayerId player_id;
        std::shared_ptr<SessionChannel> channel;
    };

    game::GameModel& model;
    Queue<MatchCommand>& commands;
    const game::GameInfo& game_info;
    std::map<SessionId, PlayerSession> sessions;
    std::chrono::milliseconds tick_interval;
    std::exception_ptr failure;
    std::mutex close_mutex;
    bool commands_closed = false;

    void process_add_player(const AddPlayerCommand& command);
    void process_movement(const PlayerCommand& command);
    void remove_closed_sessions();
    void process_commands();
    void run() override;
    void close_commands();
    void close_sessions();
    void publish_state();
    void close_pending_channels();

public:
    GameLoop(game::GameModel& model, Queue<MatchCommand>& commands,
             std::chrono::milliseconds tick_interval, const game::GameInfo& game_info);

    void stop() override;
    void rethrow_if_failed() const;
};

#endif