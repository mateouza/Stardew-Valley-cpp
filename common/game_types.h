#ifndef COMMON_GAME_TYPES_H_
#define COMMON_GAME_TYPES_H_

#include <cstdint>

namespace game {

using PlayerId = std::uint32_t;

struct Position {
    double x = 0.0;
    double y = 0.0;
};

struct MovementDirection {
    std::int8_t horizontal = 0;
    std::int8_t vertical = 0;
};

}

#endif
