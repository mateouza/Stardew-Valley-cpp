#include "demo_map.h"

#include <cstddef>
#include <utility>
#include <vector>

namespace game {

DemoMap make_demo_map() {
    constexpr std::size_t WIDTH = 50;
    constexpr std::size_t HEIGHT = 40;
    std::vector<std::vector<TileType>> tiles(HEIGHT,
                                             std::vector<TileType>(WIDTH, TileType::Walkable));

    for (std::size_t row = 2; row < 8; ++row) {
        tiles[row][5] = TileType::Blocked;
    }
    for (std::size_t column = 5; column < 8; ++column) {
        tiles[7][column] = TileType::Blocked;
    }

    std::vector<std::vector<SoilState>> soil_cells(
        HEIGHT, std::vector<SoilState>(WIDTH, SoilState::NotCultivable));

    for (std::size_t row = 2; row < 4; ++row) {
        for (std::size_t column = 3; column < 5; ++column) {
            soil_cells[row][column] = SoilState::Unprepared;
        }
    }

    return {Map{std::move(tiles)}, {2.5, 2.5}, Soil{std::move(soil_cells)}};
}

}
