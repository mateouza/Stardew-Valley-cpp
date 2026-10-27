#include "tile_colors.h"

#include <QRgb>
#include <algorithm>
#include <iterator>

namespace editor {

namespace {

struct TileColor {
    Tile tile;
    QRgb color;
};

constexpr QRgb MERCHANT = qRgb(0xf4, 0xa6, 0xc6);
constexpr QRgb EXIT = qRgb(0xf2, 0xc9, 0x4c);

constexpr TileColor TILE_COLORS[] = {
        {Tile::Grass, qRgb(0x8c, 0xc6, 0x5a)},
        {Tile::Path, qRgb(0xe3, 0xcf, 0x9c)},
        {Tile::Water, qRgb(0x6c, 0xb4, 0xe4)},
        {Tile::Obstacle, qRgb(0x8a, 0x75, 0x60)},
        {Tile::Cabin, qRgb(0xe0, 0x95, 0x6b)},
        {Tile::ExitToTown, EXIT},
        {Tile::Marnie, MERCHANT},
        {Tile::Clint, MERCHANT},
        {Tile::Pierre, MERCHANT},
        {Tile::Robin, MERCHANT},
        {Tile::CommunityCenter, qRgb(0xc9, 0xa7, 0xeb)},
        {Tile::DungeonEntrance, qRgb(0xa3, 0xa3, 0xb5)},
        {Tile::ExitToFarm, EXIT},
        {Tile::Floor, qRgb(0xb9, 0xa9, 0x93)},
        {Tile::Wall, qRgb(0x5b, 0x4b, 0x42)},
        {Tile::StairsDown, qRgb(0xf0, 0xa3, 0x5e)},
        {Tile::DungeonExit, EXIT},
};

}

QColor tile_color(Tile tile) {
    const auto entry =
            std::find_if(std::begin(TILE_COLORS), std::end(TILE_COLORS),
                         [tile](const TileColor& candidate) { return candidate.tile == tile; });
    return entry == std::end(TILE_COLORS) ? QColor(Qt::magenta) : QColor(entry->color);
}

bool shows_letter(Tile tile) {
    return tile != Tile::Grass && tile != Tile::Path && tile != Tile::Water &&
           tile != Tile::Obstacle && tile != Tile::Floor && tile != Tile::Wall;
}

}
