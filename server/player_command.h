#ifndef PLAYER_COMMAND_H
#define PLAYER_COMMAND_H

#include "game_command.h"
#include "session_id.h"

struct PlayerCommand {
    SessionId session_id;
    game::MoveRequest request;
};

#endif