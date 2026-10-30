#include "client_protocol.h"

#include <array>
#include <cstddef>
#include <stdexcept>
#include <vector>

ClientProtocol::ClientProtocol(Socket& socket): ProtocolBase(socket) {}

void ClientProtocol::send_move(const game::MoveRequest& request) {
    const auto& direction = request.direction;

    if (direction.horizontal < -1 || direction.horizontal > 1 || direction.vertical < -1 ||
        direction.vertical > 1) {
        throw std::invalid_argument("Direccion de movimiento invalida");
    }

    const std::array<std::uint8_t, 3> message{MOVE_MESSAGE,
                                              static_cast<std::uint8_t>(direction.horizontal),
                                              static_cast<std::uint8_t>(direction.vertical)};

    send_bytes(message.data(), message.size());
}

std::int8_t ClientProtocol::decode_direction(std::uint8_t value) {
    switch (value) {
        case 0x00:
            return 0;
        case 0x01:
            return 1;
        case 0xFF:
            return -1;
        default:
            throw std::runtime_error("Direccion recibida invalida");
    }
}

JoinResponse ClientProtocol::receive_join_response() {
    std::uint8_t status{};
    receive_bytes(&status, sizeof(status));

    switch (status) {
        case JOIN_ACCEPTED:
            return JoinResponse{game::PlayerId{receive_uint32()}};
        case JOIN_GAME_FULL:
            return JoinResponse{game::AddPlayerError::GameFull};
        default:
            throw std::runtime_error("Resultado de ingreso recibido invalido");
    }
}

game::PlayerState ClientProtocol::receive_player_state() {
    game::PlayerState player{};

    player.id = receive_uint32();
    player.name = receive_string(MAX_PLAYER_NAME_BYTES);
    player.position.x = receive_double();
    player.position.y = receive_double();

    std::array<std::uint8_t, 3> movement{};
    receive_bytes(movement.data(), movement.size());

    player.facing =
            game::MovementDirection{decode_direction(movement[0]), decode_direction(movement[1])};

    if (movement[2] > 1) {
        throw std::runtime_error("Indicador de movimiento recibido invalido");
    }

    player.moving = movement[2] == 1;

    return player;
}

game::GameState ClientProtocol::receive_game_state() {
    const auto count = receive_uint32();

    if (count > MAX_PLAYERS_PER_STATE) {
        throw std::length_error("La cantidad recibida de jugadores supera el limite");
    }

    game::GameState state;
    state.players.reserve(count);

    for (std::uint32_t index = 0; index < count; ++index) {
        state.players.push_back(receive_player_state());
    }

    return state;
}

std::optional<ServerMessage> ClientProtocol::receive_message() {
    const auto message_type = receive_first_byte();

    if (!message_type) {
        return std::nullopt;
    }

    switch (*message_type) {
        case JOIN_RESPONSE_MESSAGE:
            return ServerMessage{receive_join_response()};
        case GAME_STATE_MESSAGE:
            return ServerMessage{receive_game_state()};
        case MATCH_RESPONSE_MESSAGE:
            return ServerMessage{receive_match_response()};
        case GAME_INFO_MESSAGE:
            return ServerMessage{receive_game_info()};
        default:
            throw std::runtime_error("Tipo de mensaje recibido inesperado");
    }
}

void ClientProtocol::send_create_match(const CreateMatchRequest& request) {
    if (request.match_name.empty() || request.player_name.empty()) {
        throw std::invalid_argument("Los nombres no pueden estar vacios");
    }

    if (request.match_name.size() > MAX_MATCH_NAME_BYTES ||
        request.player_name.size() > MAX_PLAYER_NAME_BYTES) {
        throw std::length_error("Los nombres superan la longitud permitida");
    }

    const std::uint8_t message_type = CREATE_MATCH_MESSAGE;
    send_bytes(&message_type, sizeof(message_type));
    send_string(request.match_name);
    send_string(request.player_name);
}

void ClientProtocol::send_join_match(const JoinMatchRequest& request) {
    if (request.invitation_code.empty() || request.player_name.empty()) {
        throw std::invalid_argument("El codigo y el nombre no pueden estar vacios");
    }

    if (request.invitation_code.size() > MAX_INVITATION_CODE_BYTES ||
        request.player_name.size() > MAX_PLAYER_NAME_BYTES) {
        throw std::length_error("El codigo o el nombre superan la longitud permitida");
    }

    const std::uint8_t message_type = JOIN_MATCH_MESSAGE;
    send_bytes(&message_type, sizeof(message_type));
    send_string(request.invitation_code);
    send_string(request.player_name);
}

MatchResponse ClientProtocol::receive_match_response() {
    std::uint8_t status{};
    receive_bytes(&status, sizeof(status));

    switch (status) {
        case MATCH_FOUND: {
            auto code = receive_string(MAX_INVITATION_CODE_BYTES);

            if (code.empty()) {
                throw std::runtime_error("El codigo recibido no puede estar vacio");
            }

            return MatchResponse{code};
        }

        case MATCH_NOT_FOUND:
            return MatchResponse{std::nullopt};

        default:
            throw std::runtime_error("Resultado de seleccion de partida invalido");
    }
}

game::GameInfo ClientProtocol::receive_game_info() {
    game::GameInfo info;

    info.map.width = receive_uint32();
    info.map.height = receive_uint32();

    if (info.map.width == 0 || info.map.height == 0) {
        throw std::runtime_error("Las dimensiones recibidas del mapa son invalidas");
    }

    const auto count = static_cast<std::uint64_t>(info.map.width) * info.map.height;

    if (count > MAX_MAP_CELLS) {
        throw std::length_error("El mapa recibido supera el limite de celdas");
    }

    info.player_width = receive_double();
    info.player_height = receive_double();

    if (info.player_width <= 0.0 || info.player_height <= 0.0 ||
        info.player_width > info.map.width || info.player_height > info.map.height) {
        throw std::runtime_error("Las dimensiones recibidas del jugador son invalidas");
    }

    std::vector<std::uint8_t> tiles(static_cast<std::size_t>(count));
    receive_bytes(tiles.data(), static_cast<unsigned int>(tiles.size()));

    info.map.tiles.reserve(tiles.size());

    for (const auto tile: tiles) {
        if (tile != static_cast<std::uint8_t>(game::TileType::Walkable) &&
            tile != static_cast<std::uint8_t>(game::TileType::Blocked)) {
            throw std::runtime_error("El mapa recibido contiene un tipo de celda invalido");
        }

        info.map.tiles.push_back(static_cast<game::TileType>(tile));
    }

    return info;
}
