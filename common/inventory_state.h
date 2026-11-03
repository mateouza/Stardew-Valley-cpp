#ifndef COMMON_INVENTORY_STATE_H_
#define COMMON_INVENTORY_STATE_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "item.h"

namespace game {

struct InventoryState {
    static constexpr std::size_t CAPACITY = 9;
    static constexpr std::uint32_t MAX_STACK_SIZE = 50;
    std::array<std::optional<ItemStack>, CAPACITY> slots{};
    std::size_t selected_slot = 0;
};

}

#endif
