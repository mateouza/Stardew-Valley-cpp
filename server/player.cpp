#include "player.h"

#include <utility>

namespace game {

Player::Player(PlayerId id, std::string name, Position position):
        id(id), name(std::move(name)), position(position) {}

SetDirectionResult Player::set_direction(MovementDirection direction) {
    if (!connected) {
        return SetDirectionResult::DisconnectedPlayer;
    }

    if (direction.horizontal < -1 || direction.horizontal > 1 || direction.vertical < -1 ||
        direction.vertical > 1) {
        return SetDirectionResult::InvalidDirection;
    }

    this->direction = direction;
    if (direction.horizontal != 0 || direction.vertical != 0) {
        facing = direction;
    }
    return SetDirectionResult::Accepted;
}

MovementDirection Player::get_direction() const { return direction; }

void Player::set_position(Position position) {
    moving = position.x != this->position.x || position.y != this->position.y;
    this->position = position;
}

void Player::disconnect() {
    connected = false;
    direction = {};
}

ReconnectPlayerResult Player::reconnect() {
    if (connected) {
        return ReconnectPlayerResult::AlreadyConnected;
    }

    connected = true;
    direction = {};
    return ReconnectPlayerResult::Accepted;
}

bool Player::is_connected() const { return connected; }

PlayerState Player::get_state() const { return {id, name, position, facing, moving}; }

}
