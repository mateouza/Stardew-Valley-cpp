#ifndef SERVER_DEMO_MAP_H_
#define SERVER_DEMO_MAP_H_

#include "game_types.h"
#include "map.h"
#include "soil.h"

namespace game {

struct DemoMap {
    Map map;
    Position spawn_position;
    Soil soil;
};

DemoMap make_demo_map();

}

#endif
