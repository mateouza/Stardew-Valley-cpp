#ifndef COMMON_MAP_DATA_H_
#define COMMON_MAP_DATA_H_

#include <cstdint>
#include <vector>

namespace game {

enum class TileType : std::uint8_t {
    Walkable = 0,
    Blocked = 1,
};

struct MapData {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<TileType> tiles;
};

}

#endif
