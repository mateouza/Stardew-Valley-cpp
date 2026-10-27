#include "tile.h"

#include <algorithm>
#include <array>

namespace editor {

namespace {

struct TileInfo {
    Tile tile;
    char code;
    const char* name;
};

constexpr std::array<TileInfo, 17> TILES{{
        {Tile::Grass, 'g', "Pasto"},
        {Tile::Path, '.', "Camino"},
        {Tile::Water, '~', "Agua"},
        {Tile::Obstacle, 'o', "Obstáculo"},
        {Tile::Cabin, 'H', "Cabaña"},
        {Tile::ExitToTown, '>', "Salida al pueblo"},
        {Tile::Marnie, 'M', "Marnie"},
        {Tile::Clint, 'C', "Clint"},
        {Tile::Pierre, 'P', "Pierre"},
        {Tile::Robin, 'R', "Robin"},
        {Tile::CommunityCenter, 'U', "Centro comunitario"},
        {Tile::DungeonEntrance, 'E', "Entrada a la mazmorra"},
        {Tile::ExitToFarm, '<', "Salida a la granja"},
        {Tile::Floor, '_', "Suelo"},
        {Tile::Wall, '#', "Pared"},
        {Tile::StairsDown, 'S', "Escalera al siguiente nivel"},
        {Tile::DungeonExit, 'X', "Salida de la mazmorra"},
}};

const TileInfo& info(Tile tile) {
    return *std::find_if(TILES.begin(), TILES.end(),
                         [tile](const TileInfo& entry) { return entry.tile == tile; });
}

}

char tile_code(Tile tile) { return info(tile).code; }

std::optional<Tile> tile_from_code(char code) {
    const auto entry = std::find_if(TILES.begin(), TILES.end(), [code](const TileInfo& candidate) {
        return candidate.code == code;
    });
    if (entry == TILES.end()) {
        return std::nullopt;
    }
    return entry->tile;
}

const char* tile_name(Tile tile) { return info(tile).name; }

const std::vector<Tile>& tiles_for(ZoneKind zone) {
    static const std::vector<Tile> farm{Tile::Grass,    Tile::Path,  Tile::Water,
                                        Tile::Obstacle, Tile::Cabin, Tile::ExitToTown};
    static const std::vector<Tile> town{
            Tile::Grass,           Tile::Path,      Tile::Water,
            Tile::Obstacle,        Tile::Marnie,    Tile::Clint,
            Tile::Pierre,          Tile::Robin,     Tile::CommunityCenter,
            Tile::DungeonEntrance, Tile::ExitToFarm};
    static const std::vector<Tile> dungeon{Tile::Floor, Tile::Wall, Tile::StairsDown,
                                           Tile::DungeonExit};

    switch (zone) {
        case ZoneKind::Farm:
            return farm;
        case ZoneKind::Town:
            return town;
        case ZoneKind::Dungeon:
            return dungeon;
    }
    return farm;
}

bool is_allowed(ZoneKind zone, Tile tile) {
    const auto& allowed = tiles_for(zone);
    return std::find(allowed.begin(), allowed.end(), tile) != allowed.end();
}

Tile default_tile(ZoneKind zone) { return zone == ZoneKind::Dungeon ? Tile::Wall : Tile::Grass; }

}
