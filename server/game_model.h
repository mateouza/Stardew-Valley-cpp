#ifndef SERVER_GAME_MODEL_H_
#define SERVER_GAME_MODEL_H_

#include <chrono>
#include <string>
#include <optional>
#include <cstddef>
#include "inventory_state.h"

#include "game_results.h"
#include "game_state.h"
#include "game_types.h"

namespace game {

class GameModel {
public:
    virtual ~GameModel() = default;

    virtual AddPlayerResult add_player(const std::string& name) = 0;

    virtual ReconnectPlayerResult reconnect_player(PlayerId player_id) = 0;

    virtual SetDirectionResult set_direction(PlayerId player_id, MovementDirection direction) = 0;

    virtual void disconnect_player(PlayerId player_id) = 0;

    virtual void update_world(std::chrono::duration<double> elapsed) = 0;

    virtual GameState get_state() const = 0;

    virtual std::optional<InventoryState> get_inventory_state(PlayerId player_id) const = 0;

    virtual SelectSlotResult select_slot(PlayerId player_id, std::size_t slot) = 0;

    virtual PrepareSoilResult prepare_soil(PlayerId player_id, CellPosition cell) = 0;
};

}

#endif
