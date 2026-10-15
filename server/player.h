#ifndef SERVER_PLAYER_H_
#define SERVER_PLAYER_H_

#include <string>

#include "game_model.h"
#include "game_state.h"
#include "game_types.h"

namespace game {

class Player {
private:
    PlayerId id;
    std::string name;
    Position position;
    MovementDirection direction;
    MovementDirection facing{0, 1};
    bool connected = true;
    bool moving = false;

public:
    Player(PlayerId id, std::string name, Position position);

    SetDirectionResult set_direction(MovementDirection direction);
    MovementDirection get_direction() const;

    Position get_position() const;
    void set_position(Position position);
    void disconnect();
    ReconnectPlayerResult reconnect();

    bool is_connected() const;
    PlayerState get_state() const;
};

}

#endif
