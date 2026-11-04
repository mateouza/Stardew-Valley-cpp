#ifndef SERVER_WORLD_H_
#define SERVER_WORLD_H_

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <optional>
#include "inventory_state.h"

#include "game_model.h"
#include "game_state.h"
#include "game_types.h"
#include "map.h"
#include "player.h"
#include "soil.h"
#include "world_config.h"

namespace game {


class World: public GameModel {
private:
    Map map;
    std::optional<Soil> soil;
    Position spawn_position;
    WorldConfig config;
    std::map<PlayerId, Player> players;
    std::uint64_t next_player_id = 0;

    std::size_t count_connected_players() const;
    Position calculate_position(const Player& player, double distance) const;

public:
    World(Map map, Position spawn_position, WorldConfig config,
        std::optional<Soil> soil = std::nullopt);
    AddPlayerResult add_player(const std::string& name) override;
    ReconnectPlayerResult reconnect_player(PlayerId player_id) override;
    SetDirectionResult set_direction(PlayerId player_id, MovementDirection direction) override;
    void disconnect_player(PlayerId player_id) override;
    void update_world(std::chrono::duration<double> elapsed) override;
    GameState get_state() const override;
    std::optional<InventoryState> get_inventory_state(PlayerId player_id) const override;
    SelectSlotResult select_slot(PlayerId player_id, std::size_t slot) override;
    PrepareSoilResult prepare_soil(PlayerId player_id, CellPosition cell) override;
};

}

#endif
