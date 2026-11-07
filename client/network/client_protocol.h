#ifndef CLIENT_PROTOCOL_H
#define CLIENT_PROTOCOL_H

#include <cstdint>
#include <optional>

#include "game_command.h"
#include "game_state.h"
#include "inventory_state.h"
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
    game::GameInfo receive_game_info();
    game::InventoryState receive_inventory_state();
    game::SoilData receive_soil_state();
    game::CropsState receive_crops_state();
    UseItemResponse receive_use_item_response();
    CheatSeedsResponse receive_cheat_seeds_response();

public:
    explicit ClientProtocol(Socket& socket);

    void send_move(const game::MoveRequest& request);
    std::optional<ServerMessage> receive_message();
    void send_create_match(const CreateMatchRequest& request);
    void send_join_match(const JoinMatchRequest& request);
    void send_select_slot(const game::SelectSlotRequest& request);
    void send_use_cell(const game::UseCellRequest& request);
    void send_cheat_seeds();
};

#endif
