#ifndef PLAYER_COMMAND_H
#define PLAYER_COMMAND_H

#include "game_command.h"
#include "game_types.h"

struct PlayerCommand {
    game::PlayerId player_id;
    game::MoveRequest request;
};

#endif
