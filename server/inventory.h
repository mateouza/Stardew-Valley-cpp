#ifndef SERVER_INVENTORY_H_
#define SERVER_INVENTORY_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "item.h"

namespace game {

enum class InventoryResult {
    Accepted,
    InvalidItem,
    InvalidQuantity,
    InvalidSlot,
    EmptySlot,
    NotEnoughSpace,
    NotEnoughItems,
};

class Inventory {
private:
    static constexpr std::size_t CAPACITY = 9;
    static constexpr std::uint32_t MAX_STACK_SIZE = 50;

    std::array<std::optional<ItemStack>, CAPACITY> slots{};
    std::size_t selected_slot = 0;

public:
    Inventory() = default;

    InventoryResult add(ItemStack item);
    InventoryResult remove(std::size_t slot, std::uint32_t quantity);
    InventoryResult select(std::size_t slot);

    std::size_t get_selected_slot() const;
    std::optional<ItemStack> get_selected_item() const;

    const std::array<std::optional<ItemStack>, CAPACITY>& get_slots() const;
};

}

#endif
