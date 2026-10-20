#ifndef SERVER_PROTOCOL_H
#define SERVER_PROTOCOL_H

#include <cstdint>
#include <optional>

#include "game_command.h"
#include "game_state.h"
#include "protocol_base.h"
#include "server_message.h"

class ServerProtocol: public ProtocolBase {
private:
    std::int8_t decode_direction(std::uint8_t value);
    void validate_game_state(const game::GameState& state);
    void send_player_state(const game::PlayerState& player);

public:
    explicit ServerProtocol(Socket& socket);

    std::optional<game::MoveRequest> receive_move();
    void send_join_response(const JoinResponse& response);
    void send_game_state(const game::GameState& state);
};

#endif
