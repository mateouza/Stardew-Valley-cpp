#ifndef COMMON_SOIL_DATA_H_
#define COMMON_SOIL_DATA_H_

#include <cstdint>
#include <vector>

namespace game {

enum class SoilState : std::uint8_t {
    NotCultivable = 0,
    Unprepared = 1,
    Prepared = 2,
};

struct SoilData {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<SoilState> cells;
};

}

#endif
