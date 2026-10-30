#include "demo_map.h"

#include <cstddef>
#include <utility>
#include <vector>

namespace game {

DemoMap make_demo_map() {
    constexpr std::size_t WIDTH = 10;
    constexpr std::size_t HEIGHT = 10;
    std::vector<std::vector<TileType>> tiles(HEIGHT,
                                             std::vector<TileType>(WIDTH, TileType::Walkable));

    for (std::size_t row = 2; row < 8; ++row) {
        tiles[row][5] = TileType::Blocked;
    }
    for (std::size_t column = 5; column < 8; ++column) {
        tiles[7][column] = TileType::Blocked;
    }

    return {Map{std::move(tiles)}, {2.5, 2.5}};
}

}
