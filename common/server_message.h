#ifndef SERVER_MESSAGE_H
#define SERVER_MESSAGE_H

#include <variant>

#include "game_results.h"
#include "game_state.h"

struct JoinResponse {
    game::AddPlayerResult result;
};

using ServerMessage = std::variant<JoinResponse, game::GameState>;

#endif
