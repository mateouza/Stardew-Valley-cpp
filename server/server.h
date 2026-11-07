#ifndef SERVER_H
#define SERVER_H

#include <chrono>
#include <optional>

#include "acceptor.h"
#include "match_manager.h"
#include "soil.h"

class Server {
private:
    static constexpr unsigned int COMMAND_CAPACITY = 256;
    static constexpr unsigned int OUTGOING_CAPACITY = 256;
    static constexpr std::chrono::milliseconds TICK_INTERVAL{20};

    MatchManager matches;
    Acceptor acceptor;

public:
    Server(const char* service, game::Map map, game::Position spawn_position,
       game::WorldConfig config, std::optional<game::Soil> soil = std::nullopt);

    void run();
};

#endif
