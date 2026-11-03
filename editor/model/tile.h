#ifndef EDITOR_MODEL_TILE_H_
#define EDITOR_MODEL_TILE_H_

#include <optional>
#include <vector>

namespace editor {

enum class ZoneKind { Farm, Town, Dungeon };

enum class Tile {
    Grass,
    Path,
    Water,
    Obstacle,
    Cabin,
    ExitToTown,
    Marnie,
    Clint,
    Pierre,
    Robin,
    CommunityCenter,
    DungeonEntrance,
    ExitToFarm,
    Floor,
    Wall,
    StairsDown,
    DungeonExit
};

char tile_code(Tile tile);

std::optional<Tile> tile_from_code(char code);

const char* tile_name(Tile tile);

const std::vector<Tile>& tiles_for(ZoneKind zone);

bool is_allowed(ZoneKind zone, Tile tile);

Tile default_tile(ZoneKind zone);

}

#endif
