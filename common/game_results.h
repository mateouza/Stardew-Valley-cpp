#ifndef COMMON_GAME_RESULTS_H
#define COMMON_GAME_RESULTS_H

#include <variant>

#include "game_types.h"

namespace game {

enum class InventoryResult {
    Accepted,
    InvalidItem,
    InvalidQuantity,
    InvalidSlot,
    EmptySlot,
    NotEnoughSpace,
    NotEnoughItems,
    UnknownPlayer,
};

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

enum class PlantCropResult {
    Accepted,
    OutOfBounds,
    NotCultivable,
    SoilNotPrepared,
    AlreadyPlanted,
    InvalidCrop,
};

enum class WaterCropResult {
    Accepted,
    OutOfBounds,
    NoCrop,
    AlreadyWatered,
};

enum class UseItemError {
    UnknownPlayer,
    DisconnectedPlayer,
    OutOfBounds,
    NotCultivable,
    OutOfReach,
    NoItemSelected,
    UnsupportedItem,
};

using UseItemResult =
        std::variant<PrepareSoilResult, PlantCropResult, WaterCropResult, UseItemError>;

}

#endif
