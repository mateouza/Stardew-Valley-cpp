#ifndef SERVER_MAP_H_
#define SERVER_MAP_H_

#include <vector>

#include "game_types.h"

namespace game {

enum class TileType {
    Walkable,
    Blocked,
};

class Map {
private:
    std::vector<std::vector<TileType>> tiles;

public:
    explicit Map(std::vector<std::vector<TileType>> tiles);

    bool can_occupy(Position position, double width, double height) const;
};

}

#endif
