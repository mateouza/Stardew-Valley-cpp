#include "item_icons.h"

std::optional<IconItem> obtener_icono_item(game::ItemType tipo_item) {
    switch (tipo_item) {
        case game::ItemType::Pickaxe:
            return IconItem{"assets/Misc/pickaxe.png", std::nullopt};
        case game::ItemType::Shovel:
            return IconItem{"assets/Misc/shovel.png", std::nullopt};
        case game::ItemType::Sword:
            return IconItem{"assets/Misc/sword.png", std::nullopt};
        case game::ItemType::StrawberrySeed:
            return IconItem{"assets/Farm/Crops/Spring Crops.png", SDL2pp::Rect(112, 16, 16, 16)};
        case game::ItemType::WateringCan:
            return std::nullopt;
    }
    return std::nullopt;
}