#ifndef USE_CELL_COMMAND_H
#define USE_CELL_COMMAND_H

#include "game_command.h"
#include "session_id.h"

struct UseCellCommand {
    SessionId session_id;
    game::UseCellRequest request;
};

#endif