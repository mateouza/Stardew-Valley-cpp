#ifndef SERVER_MESSAGE_H
#define SERVER_MESSAGE_H

#include <optional>
#include <string>
#include <variant>

#include "game_results.h"
#include "game_state.h"

struct JoinResponse {
    game::AddPlayerResult result;
};

struct MatchResponse {
    std::optional<std::string> invitation_code;
};

using ServerMessage = std::variant<JoinResponse, game::GameState, MatchResponse>;

#endif
