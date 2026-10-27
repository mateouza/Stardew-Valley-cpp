#ifndef COMMON_GAME_INFO_H_
#define COMMON_GAME_INFO_H_

#include "map_data.h"

namespace game {

struct GameInfo {
    MapData map;
    double player_width = 0.0;
    double player_height = 0.0;
};

}

#endif
