#ifndef SELECT_SLOT_COMMAND_H
#define SELECT_SLOT_COMMAND_H

#include "game_command.h"
#include "session_id.h"

struct SelectSlotCommand {
    SessionId session_id;
    game::SelectSlotRequest request;
};

#endif
