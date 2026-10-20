#ifndef SERVER_PROTOCOL_H
#define SERVER_PROTOCOL_H

#include <cstdint>
#include <optional>

#include "game_command.h"
#include "protocol_base.h"

class ServerProtocol: public ProtocolBase {
private:
    std::int8_t decode_direction(std::uint8_t value);

public:
    explicit ServerProtocol(Socket& socket);

    std::optional<game::MoveRequest> receive_move();
};

#endif
