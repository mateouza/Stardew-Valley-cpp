#ifndef SERVER_PROTOCOL_H
#define SERVER_PROTOCOL_H

#include <cstdint>
#include <optional>

#include "game_command.h"
#include "game_state.h"
#include "inventory_state.h"
#include "lobby_request.h"
#include "protocol_base.h"
#include "server_message.h"

class ServerProtocol: public ProtocolBase {
private:
    std::int8_t decode_direction(std::uint8_t value);
    void validate_game_state(const game::GameState& state);
    void send_player_state(const game::PlayerState& player);
    void validate_game_info(const game::GameInfo& info);
    void validate_inventory_state(const game::InventoryState& state);
    game::MoveRequest receive_move_payload();

public:
    explicit ServerProtocol(Socket& socket);

    std::optional<game::MoveRequest> receive_move();
    void send_join_response(const JoinResponse& response);
    void send_game_state(const game::GameState& state);
    std::optional<LobbyRequest> receive_lobby_request();
    void send_match_response(const MatchResponse& response);
    void send_game_info(const game::GameInfo& info);
    void send_inventory_state(const game::InventoryState& state);
    std::optional<game::GameRequest> receive_game_request();
    void send_soil_state(const game::SoilData& state);
    void send_crops_state(const game::CropsState& state);
    void send_use_item_response(const UseItemResponse& response);
};

#endif
