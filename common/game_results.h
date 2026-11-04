#ifndef COMMON_GAME_RESULTS_H
#define COMMON_GAME_RESULTS_H

#include <variant>

#include "game_types.h"

namespace game {

enum class AddPlayerError {
    GameFull,
};

using AddPlayerResult = std::variant<PlayerId, AddPlayerError>;

enum class ReconnectPlayerResult {
    Accepted,
    UnknownPlayer,
    AlreadyConnected,
    GameFull,
};

enum class SetDirectionResult {
    Accepted,
    UnknownPlayer,
    DisconnectedPlayer,
    InvalidDirection,
};

enum class SelectSlotResult {
    Accepted,
    UnknownPlayer,
    DisconnectedPlayer,
    InvalidSlot,
};

enum class PrepareSoilResult {
    Accepted,
    OutOfBounds,
    NotCultivable,
    AlreadyPrepared,
    UnknownPlayer,
    DisconnectedPlayer,
    OutOfReach,
    ShovelNotSelected,
};

}

#endif
