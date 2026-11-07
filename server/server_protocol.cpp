#include "server_protocol.h"

#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <variant>
#include <vector>

ServerProtocol::ServerProtocol(Socket& socket): ProtocolBase(socket) {}

std::int8_t ServerProtocol::decode_direction(std::uint8_t value) {
    switch (value) {
        case 0x00:
            return 0;
        case 0x01:
            return 1;
        case 0xFF:
            return -1;
        default:
            throw std::runtime_error("Direccion de movimiento invalida");
    }
}

game::MoveRequest ServerProtocol::receive_move_payload() {
    std::array<std::uint8_t, 2> direction{};
    receive_bytes(direction.data(), direction.size());

    return game::MoveRequest{
            game::MovementDirection{decode_direction(direction[0]),
                                    decode_direction(direction[1])}};
}

std::optional<game::MoveRequest> ServerProtocol::receive_move() {
    const auto message_type = receive_first_byte();

    if (!message_type) {
        return std::nullopt;
    }

    if (*message_type != MOVE_MESSAGE) {
        throw std::runtime_error("Tipo de mensaje inesperado");
    }

    return receive_move_payload();
}

std::optional<game::GameRequest> ServerProtocol::receive_game_request() {
    const auto message_type = receive_first_byte();

    if (!message_type) {
        return std::nullopt;
    }

    switch (*message_type) {
        case MOVE_MESSAGE:
            return game::GameRequest{receive_move_payload()};

        case SELECT_SLOT_MESSAGE: {
            std::uint8_t slot{};
            receive_bytes(&slot, sizeof(slot));

            if (slot >= game::InventoryState::CAPACITY) {
                throw std::runtime_error("El espacio seleccionado recibido es invalido");
            }

            return game::GameRequest{game::SelectSlotRequest{slot}};
        }
        case USE_CELL_MESSAGE: {
            const auto row = receive_uint32();
            const auto column = receive_uint32();
            return game::UseCellRequest{game::CellPosition{row, column}};
        }

        default:
            throw std::runtime_error("Tipo de mensaje inesperado");
    }
}

void ServerProtocol::send_join_response(const JoinResponse& response) {
    if (const auto* player_id = std::get_if<game::PlayerId>(&response.result)) {
        const std::array<std::uint8_t, 2> header{JOIN_RESPONSE_MESSAGE, JOIN_ACCEPTED};

        send_bytes(header.data(), header.size());
        send_uint32(*player_id);
        return;
    }

    const auto* error = std::get_if<game::AddPlayerError>(&response.result);

    if (!error || *error != game::AddPlayerError::GameFull) {
        throw std::invalid_argument("Resultado de ingreso invalido");
    }

    const std::array<std::uint8_t, 2> header{JOIN_RESPONSE_MESSAGE, JOIN_GAME_FULL};
    send_bytes(header.data(), header.size());
}

void ServerProtocol::validate_game_state(const game::GameState& state) {
    if (state.players.size() > MAX_PLAYERS_PER_STATE) {
        throw std::length_error("La cantidad de jugadores supera el limite del protocolo");
    }

    for (const auto& player: state.players) {
        if (player.name.size() > MAX_PLAYER_NAME_BYTES) {
            throw std::length_error("El nombre supera la longitud permitida por el protocolo");
        }

        if (!std::isfinite(player.position.x) || !std::isfinite(player.position.y)) {
            throw std::invalid_argument("La posicion del jugador debe ser finita");
        }

        if (player.facing.horizontal < -1 || player.facing.horizontal > 1 ||
            player.facing.vertical < -1 || player.facing.vertical > 1) {
            throw std::invalid_argument("La orientacion del jugador es invalida");
        }
    }
}

void ServerProtocol::send_player_state(const game::PlayerState& player) {
    send_uint32(player.id);
    send_string(player.name);
    send_double(player.position.x);
    send_double(player.position.y);

    const std::array<std::uint8_t, 3> movement{static_cast<std::uint8_t>(player.facing.horizontal),
                                               static_cast<std::uint8_t>(player.facing.vertical),
                                               static_cast<std::uint8_t>(player.moving)};

    send_bytes(movement.data(), movement.size());
}

void ServerProtocol::send_game_state(const game::GameState& state) {
    validate_game_state(state);

    const std::uint8_t message_type = GAME_STATE_MESSAGE;
    send_bytes(&message_type, sizeof(message_type));
    send_uint32(static_cast<std::uint32_t>(state.players.size()));

    for (const auto& player: state.players) {
        send_player_state(player);
    }
}

std::optional<LobbyRequest> ServerProtocol::receive_lobby_request() {
    const auto message_type = receive_first_byte();

    if (!message_type) {
        return std::nullopt;
    }

    switch (*message_type) {
        case CREATE_MATCH_MESSAGE: {
            CreateMatchRequest request{receive_string(MAX_MATCH_NAME_BYTES),
                                       receive_string(MAX_PLAYER_NAME_BYTES)};

            if (request.match_name.empty() || request.player_name.empty()) {
                throw std::runtime_error("Los nombres no pueden estar vacios");
            }

            return LobbyRequest{request};
        }

        case JOIN_MATCH_MESSAGE: {
            JoinMatchRequest request{receive_string(MAX_INVITATION_CODE_BYTES),
                                     receive_string(MAX_PLAYER_NAME_BYTES)};

            if (request.invitation_code.empty() || request.player_name.empty()) {
                throw std::runtime_error("El codigo y el nombre no pueden estar vacios");
            }

            return LobbyRequest{request};
        }

        default:
            throw std::runtime_error("Tipo de pedido de lobby inesperado");
    }
}

void ServerProtocol::send_match_response(const MatchResponse& response) {
    if (response.invitation_code &&
        (response.invitation_code->empty() ||
         response.invitation_code->size() > MAX_INVITATION_CODE_BYTES)) {
        throw std::invalid_argument("El codigo de invitacion es invalido");
    }

    const std::array<std::uint8_t, 2> header{
            MATCH_RESPONSE_MESSAGE, response.invitation_code ? MATCH_FOUND : MATCH_NOT_FOUND};

    send_bytes(header.data(), header.size());

    if (response.invitation_code) {
        send_string(*response.invitation_code);
    }
}

void ServerProtocol::validate_game_info(const game::GameInfo& info) {
    const auto& map = info.map;

    if (map.width == 0 || map.height == 0) {
        throw std::invalid_argument("Las dimensiones del mapa deben ser positivas");
    }

    const auto count = static_cast<std::uint64_t>(map.width) * map.height;

    if (count > MAX_MAP_CELLS) {
        throw std::length_error("El mapa supera el limite de celdas del protocolo");
    }

    if (map.tiles.size() != count) {
        throw std::invalid_argument("La cantidad de celdas no coincide con las dimensiones");
    }

    if (!std::isfinite(info.player_width) || !std::isfinite(info.player_height) ||
        info.player_width <= 0.0 || info.player_height <= 0.0 || info.player_width > map.width ||
        info.player_height > map.height) {
        throw std::invalid_argument("Las dimensiones del jugador son invalidas");
    }

    for (const auto tile: map.tiles) {
        if (tile != game::TileType::Walkable && tile != game::TileType::Blocked) {
            throw std::invalid_argument("El mapa contiene un tipo de celda invalido");
        }
    }
}

void ServerProtocol::send_game_info(const game::GameInfo& info) {
    validate_game_info(info);

    std::vector<std::uint8_t> tiles;
    tiles.reserve(info.map.tiles.size());

    for (const auto tile: info.map.tiles) {
        tiles.push_back(static_cast<std::uint8_t>(tile));
    }

    const std::uint8_t message_type = GAME_INFO_MESSAGE;
    send_bytes(&message_type, sizeof(message_type));
    send_uint32(info.map.width);
    send_uint32(info.map.height);
    send_double(info.player_width);
    send_double(info.player_height);
    send_bytes(tiles.data(), static_cast<unsigned int>(tiles.size()));
}

void ServerProtocol::validate_inventory_state(const game::InventoryState& state) {
    if (state.selected_slot >= state.items.size()) {
        throw std::invalid_argument("El espacio seleccionado es invalido");
    }

    for (const auto& slot: state.items) {
        if (!slot) {
            continue;
        }

        std::uint32_t stack_limit = 1;

        switch (slot->type) {
            case game::ItemType::Pickaxe:
            case game::ItemType::WateringCan:
            case game::ItemType::Shovel:
            case game::ItemType::Sword:
                break;
            case game::ItemType::StrawberrySeed:
                stack_limit = game::InventoryState::MAX_STACK_SIZE;
                break;
            default:
                throw std::invalid_argument("El inventario contiene un objeto invalido");
        }

        switch (slot->quality) {
            case game::ItemQuality::Base:
            case game::ItemQuality::Good:
            case game::ItemQuality::Excellent:
            case game::ItemQuality::Incredible:
            case game::ItemQuality::Perfect:
                break;
            default:
                throw std::invalid_argument("El inventario contiene una calidad invalida");
        }

        if (slot->quantity == 0 || slot->quantity > stack_limit) {
            throw std::invalid_argument("El inventario contiene una cantidad invalida");
        }
    }
}

void ServerProtocol::send_inventory_state(const game::InventoryState& state) {
    validate_inventory_state(state);

    const std::array<std::uint8_t, 2> header{INVENTORY_STATE_MESSAGE,
                                             static_cast<std::uint8_t>(state.selected_slot)};

    send_bytes(header.data(), header.size());

    for (const auto& slot: state.items) {
        if (!slot) {
            const std::uint8_t presence = INVENTORY_SLOT_EMPTY;
            send_bytes(&presence, sizeof(presence));
            continue;
        }

        const std::array<std::uint8_t, 3> item_header{INVENTORY_SLOT_OCCUPIED,
                                                      static_cast<std::uint8_t>(slot->type),
                                                      static_cast<std::uint8_t>(slot->quality)};

        send_bytes(item_header.data(), item_header.size());
        send_uint32(slot->quantity);
    }
}

void ServerProtocol::send_soil_state(const game::SoilData& state) {
    if (state.width == 0 || state.height == 0 ||
        state.width > MAX_MAP_CELLS / state.height) {
        throw std::invalid_argument("Las dimensiones del suelo son invalidas");
    }

    const auto count = static_cast<std::size_t>(state.width) * state.height;
    if (state.cells.size() != count) {
        throw std::invalid_argument("Las celdas no coinciden con las dimensiones del suelo");
    }

    std::vector<std::uint8_t> cells;
    cells.reserve(count);

    for (const auto cell: state.cells) {
        switch (cell) {
            case game::SoilState::NotCultivable:
            case game::SoilState::Unprepared:
            case game::SoilState::Prepared:
                cells.push_back(static_cast<std::uint8_t>(cell));
                break;
            default:
                throw std::invalid_argument("El estado de la celda es invalido");
        }
    }

    const std::uint8_t message_type = SOIL_STATE_MESSAGE;
    send_bytes(&message_type, sizeof(message_type));
    send_uint32(state.width);
    send_uint32(state.height);
    send_bytes(cells.data(), static_cast<unsigned int>(cells.size()));
}

void ServerProtocol::send_crops_state(const game::CropsState& state) {
    if (state.crops.size() > MAX_MAP_CELLS) {
        throw std::invalid_argument("La cantidad de cultivos supera el limite");
    }

    for (const auto& crop: state.crops) {
        if (crop.state.type != game::CropType::Strawberry) {
            throw std::invalid_argument("El tipo de cultivo es invalido");
        }
    }

    const std::uint8_t message_type = CROPS_STATE_MESSAGE;
    send_bytes(&message_type, sizeof(message_type));
    send_uint32(static_cast<std::uint32_t>(state.crops.size()));

    for (const auto& crop: state.crops) {
        send_uint32(crop.cell.row);
        send_uint32(crop.cell.column);

        const std::array<std::uint8_t, 2> details{
                static_cast<std::uint8_t>(crop.state.type),
                static_cast<std::uint8_t>(crop.state.needs_water)};
        send_bytes(details.data(), details.size());
    }
}

void ServerProtocol::send_use_item_response(const UseItemResponse& response) {
    std::uint8_t kind;
    std::uint8_t value;

    if (const auto* prepare = std::get_if<game::PrepareSoilResult>(&response.result)) {
        kind = USE_RESULT_PREPARE;
        value = static_cast<std::uint8_t>(*prepare);
    } else if (const auto* plant = std::get_if<game::PlantCropResult>(&response.result)) {
        kind = USE_RESULT_PLANT;
        value = static_cast<std::uint8_t>(*plant);
    } else if (const auto* water = std::get_if<game::WaterCropResult>(&response.result)) {
        kind = USE_RESULT_WATER;
        value = static_cast<std::uint8_t>(*water);
    } else {
        kind = USE_RESULT_ERROR;
        value = static_cast<std::uint8_t>(std::get<game::UseItemError>(response.result));
    }

    if (decode_use_item_result(kind, value) != response.result) {
        throw std::invalid_argument("El resultado de uso de objeto es invalido");
    }

    const std::array<std::uint8_t, 3> message{
            USE_ITEM_RESPONSE_MESSAGE, kind, value};
    send_bytes(message.data(), message.size());
}
