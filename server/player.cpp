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

Position Player::get_position() const { return position; }

void Player::set_position(Position position) {
    moving = position.x != this->position.x || position.y != this->position.y;
    this->position = position;
}

void Player::disconnect() {
    connected = false;
    direction = {};
    moving = false;
}

ReconnectPlayerResult Player::reconnect() {
    if (connected) {
        return ReconnectPlayerResult::AlreadyConnected;
    }

    connected = true;
    direction = {};
    moving = false;
    return ReconnectPlayerResult::Accepted;
}

bool Player::is_connected() const { return connected; }

PlayerState Player::get_state() const { return {id, name, position, facing, moving}; }

InventoryResult Player::add_item(ItemStack item) { return inventory.add(item); }

InventoryResult Player::remove_item(std::size_t slot, std::uint32_t quantity) {
    return inventory.remove(slot, quantity);
}

InventoryResult Player::select_slot(std::size_t slot) { return inventory.select(slot); }

std::size_t Player::get_selected_slot() const { return inventory.get_selected_slot(); }

std::optional<ItemStack> Player::get_selected_item() const { return inventory.get_selected_item(); }
}
