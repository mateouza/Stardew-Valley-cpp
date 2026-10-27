#ifndef MATCH_COMMAND_H
#define MATCH_COMMAND_H

#include <memory>
#include <string>
#include <variant>

#include "player_command.h"
#include "session_channel.h"

struct AddPlayerCommand {
    std::string name;
    std::shared_ptr<SessionChannel> channel;
};

using MatchCommand = std::variant<PlayerCommand, AddPlayerCommand>;

#endif