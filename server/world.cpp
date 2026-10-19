#include "world.h"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace game {

World::World(Map map, Position spawn_position, WorldConfig config):
        map(std::move(map)), spawn_position(spawn_position), config(config) {
    if (config.max_players == 0) {
        throw std::invalid_argument("El cupo debe ser mayor que cero");
    }
    if (!std::isfinite(config.player_speed) || config.player_speed <= 0.0) {
        throw std::invalid_argument("La velocidad debe ser positiva y finita");
    }
    if (!this->map.can_occupy(spawn_position, config.player_width, config.player_height)) {
        throw std::invalid_argument("El cuerpo debe poder ocupar la posicion inicial");
    }
}

std::size_t World::count_connected_players() const {
    std::size_t count = 0;
    for (const auto& entry: players) {
        if (entry.second.is_connected()) {
            ++count;
        }
    }
    return count;
}

AddPlayerResult World::add_player(const std::string& name) {
    if (count_connected_players() >= config.max_players) {
        return AddPlayerError::GameFull;
    }
    if (next_player_id > std::numeric_limits<PlayerId>::max()) {
        throw std::overflow_error("Se agotaron los identificadores de jugadores");
    }

    const auto player_id = static_cast<PlayerId>(next_player_id);
    players.emplace(player_id, Player(player_id, name, spawn_position));
    ++next_player_id;
    return player_id;
}

ReconnectPlayerResult World::reconnect_player(PlayerId player_id) {
    const auto entry = players.find(player_id);
    if (entry == players.end()) {
        return ReconnectPlayerResult::UnknownPlayer;
    }
    if (entry->second.is_connected()) {
        return ReconnectPlayerResult::AlreadyConnected;
    }
    if (count_connected_players() >= config.max_players) {
        return ReconnectPlayerResult::GameFull;
    }
    return entry->second.reconnect();
}

SetDirectionResult World::set_direction(PlayerId player_id, MovementDirection direction) {
    const auto entry = players.find(player_id);
    if (entry == players.end()) {
        return SetDirectionResult::UnknownPlayer;
    }
    return entry->second.set_direction(direction);
}

void World::disconnect_player(PlayerId player_id) {
    const auto entry = players.find(player_id);
    if (entry != players.end()) {
        entry->second.disconnect();
    }
}

Position World::calculate_position(const Player& player, double distance) const {
    const auto position = player.get_position();
    const auto direction = player.get_direction();
    if (distance == 0.0 || (direction.horizontal == 0 && direction.vertical == 0)) {
        return position;
    }

    const double horizontal = direction.horizontal;
    const double vertical = direction.vertical;
    const double length = std::hypot(horizontal, vertical);
    const Position destination{position.x + horizontal / length * distance,
                               position.y + vertical / length * distance};
    return map.resolve_movement(position, destination, config.player_width, config.player_height);
}

void World::update_world(std::chrono::duration<double> elapsed) {
    if (!std::isfinite(elapsed.count()) || elapsed.count() < 0.0) {
        throw std::invalid_argument("El intervalo debe ser finito y no negativo");
    }

    const double distance = config.player_speed * elapsed.count();
    if (!std::isfinite(distance)) {
        throw std::overflow_error("El desplazamiento supera el rango representable");
    }
    for (auto& entry: players) {
        auto& player = entry.second;
        if (player.is_connected()) {
            player.set_position(calculate_position(player, distance));
        }
    }
}

GameState World::get_state() const {
    GameState state;
    state.players.reserve(count_connected_players());
    for (const auto& entry: players) {
        if (entry.second.is_connected()) {
            state.players.push_back(entry.second.get_state());
        }
    }
    return state;
}

}
