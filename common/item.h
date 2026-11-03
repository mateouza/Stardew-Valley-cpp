#ifndef COMMON_ITEM_H_
#define COMMON_ITEM_H_

#include <cstdint>

namespace game {

enum class ItemType : std::uint8_t {
    Pickaxe,
    WateringCan,
    Shovel,
    Sword,
    StrawberrySeed,
};

enum class ItemQuality : std::uint8_t {
    Base,
    Good,
    Excellent,
    Incredible,
    Perfect,
};

struct ItemStack {
    ItemType type;
    ItemQuality quality;
    std::uint32_t quantity;
};

}

#endif
