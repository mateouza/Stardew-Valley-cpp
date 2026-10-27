#ifndef CLIENT_PROTOCOL_H
#define CLIENT_PROTOCOL_H

#include <cstdint>
#include <optional>

#include "game_command.h"
#include "game_state.h"
#include "lobby_request.h"
#include "protocol_base.h"
#include "server_message.h"

class ClientProtocol: public ProtocolBase {
private:
    std::int8_t decode_direction(std::uint8_t value);
    JoinResponse receive_join_response();
    game::PlayerState receive_player_state();
    game::GameState receive_game_state();
    MatchResponse receive_match_response();

public:
    explicit ClientProtocol(Socket& socket);

    void send_move(const game::MoveRequest& request);
    std::optional<ServerMessage> receive_message();
    void send_create_match(const CreateMatchRequest& request);
    void send_join_match(const JoinMatchRequest& request);
};

#endif