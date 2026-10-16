#ifndef SERVER_GAME_MODEL_H_
#define SERVER_GAME_MODEL_H_

#include <chrono>
#include <string>
#include <variant>

#include "game_state.h"
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

class GameModel {
public:
    virtual ~GameModel() = default;

    virtual AddPlayerResult add_player(const std::string& name) = 0;

    virtual ReconnectPlayerResult reconnect_player(PlayerId player_id) = 0;

    virtual SetDirectionResult set_direction(PlayerId player_id, MovementDirection direction) = 0;

    virtual void disconnect_player(PlayerId player_id) = 0;

    virtual void update_world(std::chrono::duration<double> elapsed) = 0;

    virtual GameState get_state() const = 0;
};

}

#endif
