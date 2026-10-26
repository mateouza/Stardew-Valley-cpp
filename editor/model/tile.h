#ifndef EDITOR_MODEL_TILE_H_
#define EDITOR_MODEL_TILE_H_

#include <optional>
#include <vector>

namespace editor {

enum class ZoneKind { Farm, Town, Dungeon };

// Lista provisoria hasta acordar el formato del mapa con el servidor.
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

// Letra con la que se guarda la casilla en el archivo del mapa.
char tile_code(Tile tile);

std::optional<Tile> tile_from_code(char code);

const char* tile_name(Tile tile);

// Casillas que se pueden poner en cada zona, en el orden en que las muestra la paleta.
const std::vector<Tile>& tiles_for(ZoneKind zone);

bool is_allowed(ZoneKind zone, Tile tile);

// Casilla con la que arranca una zona nueva.
Tile default_tile(ZoneKind zone);

}

#endif
