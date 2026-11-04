#include "inventory.h"

#include <algorithm>

namespace game {

InventoryResult Inventory::select(std::size_t slot) {
    if (slot >= items.size()) {
        return InventoryResult::InvalidSlot;
    }

    selected_slot = slot;
    return InventoryResult::Accepted;
}

std::size_t Inventory::get_selected_slot() const { return selected_slot; }

std::optional<ItemStack> Inventory::get_selected_item() const { return items[selected_slot]; }

const std::array<std::optional<ItemStack>, Inventory::CAPACITY>& Inventory::get_slots() const {
    return items;
}

InventoryResult Inventory::remove(std::size_t slot, std::uint32_t quantity) {
    if (slot >= items.size()) {
        return InventoryResult::InvalidSlot;
    }

    if (quantity == 0) {
        return InventoryResult::InvalidQuantity;
    }

    auto& item = items[slot];

    if (!item) {
        return InventoryResult::EmptySlot;
    }

    if (quantity > item->quantity) {
        return InventoryResult::NotEnoughItems;
    }

    item->quantity -= quantity;

    if (item->quantity == 0) {
        item.reset();
    }

    return InventoryResult::Accepted;
}

InventoryResult Inventory::add(ItemStack item) {
    if (item.quantity == 0) {
        return InventoryResult::InvalidQuantity;
    }

    std::uint32_t stack_limit = 1;

    switch (item.type) {
        case ItemType::Pickaxe:
        case ItemType::WateringCan:
        case ItemType::Shovel:
        case ItemType::Sword:
            break;
        case ItemType::StrawberrySeed:
            stack_limit = MAX_STACK_SIZE;
            break;
        default:
            return InventoryResult::InvalidItem;
    }

    switch (item.quality) {
        case ItemQuality::Base:
        case ItemQuality::Good:
        case ItemQuality::Excellent:
        case ItemQuality::Incredible:
        case ItemQuality::Perfect:
            break;
        default:
            return InventoryResult::InvalidItem;
    }

    auto updated_slots = items;
    auto remaining = item.quantity;

    for (auto& slot: updated_slots) {
        if (remaining == 0) {
            break;
        }

        if (!slot || slot->type != item.type || slot->quality != item.quality) {
            continue;
        }

        const auto available = stack_limit - slot->quantity;
        const auto added = std::min(available, remaining);

        slot->quantity += added;
        remaining -= added;
    }

    for (auto& slot: updated_slots) {
        if (remaining == 0) {
            break;
        }

        if (slot) {
            continue;
        }

        const auto added = std::min(stack_limit, remaining);
        slot = ItemStack{item.type, item.quality, added};
        remaining -= added;
    }

    if (remaining != 0) {
        return InventoryResult::NotEnoughSpace;
    }

    items = updated_slots;
    return InventoryResult::Accepted;
}

InventoryState Inventory::get_state() const { return {items, selected_slot}; }
}
