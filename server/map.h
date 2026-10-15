#ifndef SERVER_MAP_H_
#define SERVER_MAP_H_

#include <cstddef>
#include <vector>

#include "game_types.h"

namespace game {

enum class TileType {
    Walkable,
    Blocked,
};

class Map {
private:
    struct TileRange {
        std::size_t first_column;
        std::size_t end_column;
        std::size_t first_row;
        std::size_t end_row;
    };

    std::vector<std::vector<TileType>> tiles;

    TileRange get_tile_range(Position position, Position destination, double width,
                             double height) const;
    double resolve_horizontal(Position position, double destination, double width,
                              double height) const;
    double resolve_vertical(Position position, double destination, double width,
                            double height) const;
    Position correct_rounding(Position position, Position result, double width,
                              double height) const;

public:
    explicit Map(std::vector<std::vector<TileType>> tiles);

    bool can_occupy(Position position, double width, double height) const;

    Position resolve_movement(Position position, Position destination, double width,
                              double height) const;
};

}

#endif
