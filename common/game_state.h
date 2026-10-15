#ifndef COMMON_GAME_STATE_H_
#define COMMON_GAME_STATE_H_

#include <string>
#include <vector>

#include "game_types.h"

namespace game {

struct PlayerState {
    PlayerId id;
    std::string name;
    Position position;
    MovementDirection facing;
    bool moving;
};

struct GameState {
    std::vector<PlayerState> players;
};

}

#endif
