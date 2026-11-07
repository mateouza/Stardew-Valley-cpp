#ifndef SERVER_INVENTORY_H_
#define SERVER_INVENTORY_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "inventory_state.h"
#include "item.h"
#include "game_results.h"

namespace game {

class Inventory {
private:
    static constexpr std::size_t CAPACITY = InventoryState::CAPACITY;
    static constexpr std::uint32_t MAX_STACK_SIZE = InventoryState::MAX_STACK_SIZE;

    std::array<std::optional<ItemStack>, CAPACITY> items{};
    std::size_t selected_slot = 0;

public:
    Inventory() = default;

    InventoryResult add(ItemStack item);
    InventoryResult remove(std::size_t slot, std::uint32_t quantity);
    InventoryResult select(std::size_t slot);

    std::size_t get_selected_slot() const;
    std::optional<ItemStack> get_selected_item() const;

    const std::array<std::optional<ItemStack>, CAPACITY>& get_slots() const;
    InventoryState get_state() const;
};

}

#endif
