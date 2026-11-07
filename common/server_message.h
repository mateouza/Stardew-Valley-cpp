#ifndef SERVER_MESSAGE_H
#define SERVER_MESSAGE_H

#include <optional>
#include <string>
#include <variant>

#include "game_info.h"
#include "game_results.h"
#include "game_state.h"
#include "inventory_state.h"
#include "soil_data.h"
#include "crop_state.h"
#include "use_item_response.h"
#include "cheat_seeds_response.h"

struct JoinResponse {
    game::AddPlayerResult result;
};

struct MatchResponse {
    std::optional<std::string> invitation_code;
};

using ServerMessage = std::variant<JoinResponse, game::GameState, MatchResponse, game::GameInfo,
                                   game::InventoryState, game::SoilData, game::CropsState,
                                   UseItemResponse, CheatSeedsResponse>;

#endif
