#ifndef SERVER_WORLD_CONFIG_H_
#define SERVER_WORLD_CONFIG_H_

#include <cstddef>

namespace game {

struct WorldConfig {
    std::size_t max_players = 0;
    double player_speed = 0.0;
    double player_width = 0.0;
    double player_height = 0.0;
};

}

#endif
