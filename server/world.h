#ifndef SERVER_WORLD_H_
#define SERVER_WORLD_H_

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

#include "game_model.h"
#include "game_state.h"
#include "game_types.h"
#include "map.h"
#include "player.h"
#include "world_config.h"

namespace game {


class World: public GameModel {
private:
    Map map;
    Position spawn_position;
    WorldConfig config;
    std::map<PlayerId, Player> players;
    std::uint64_t next_player_id = 0;

    std::size_t count_connected_players() const;
    Position calculate_position(const Player& player, double distance) const;

public:
    World(Map map, Position spawn_position, WorldConfig config);

    AddPlayerResult add_player(const std::string& name) override;
    ReconnectPlayerResult reconnect_player(PlayerId player_id) override;
    SetDirectionResult set_direction(PlayerId player_id, MovementDirection direction) override;
    void disconnect_player(PlayerId player_id) override;
    void update_world(std::chrono::duration<double> elapsed) override;
    GameState get_state() const override;
};

}

#endif
