#ifndef COMMON_GAME_COMMAND_H_
#define COMMON_GAME_COMMAND_H_

#include <cstdint>
#include <variant>

#include "game_types.h"

namespace game {

struct MoveRequest {
    MovementDirection direction;
};
struct SelectSlotRequest {
    std::uint8_t slot;
};
struct UseCellRequest {
    CellPosition cell;
};

struct CheatSeedsRequest {};

using GameRequest =
        std::variant<MoveRequest, SelectSlotRequest, UseCellRequest, CheatSeedsRequest>;
}

#endif
