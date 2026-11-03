#ifndef SERVER_PLAYER_H_
#define SERVER_PLAYER_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include "game_model.h"
#include "game_state.h"
#include "game_types.h"
#include "inventory.h"

namespace game {

class Player {
private:
    PlayerId id;
    std::string name;
    Position position;
    MovementDirection direction;
    MovementDirection facing{0, 1};
    bool connected = true;
    bool moving = false;
    Inventory inventory;

public:
    Player(PlayerId id, std::string name, Position position);

    SetDirectionResult set_direction(MovementDirection direction);
    MovementDirection get_direction() const;

    Position get_position() const;
    void set_position(Position position);
    void disconnect();
    ReconnectPlayerResult reconnect();

    bool is_connected() const;
    PlayerState get_state() const;
    InventoryResult add_item(ItemStack item);
    InventoryResult remove_item(std::size_t slot, std::uint32_t quantity);
    InventoryResult select_slot(std::size_t slot);

    std::size_t get_selected_slot() const;
    std::optional<ItemStack> get_selected_item() const;
};

}

#endif
