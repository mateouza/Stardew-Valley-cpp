#include "server_protocol.h"

#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <variant>

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

std::optional<game::MoveRequest> ServerProtocol::receive_move() {
    auto message_type = receive_first_byte();

    if (!message_type) {
        return std::nullopt;
    }

    if (*message_type != MOVE_MESSAGE) {
        throw std::runtime_error("Tipo de mensaje inesperado");
    }

    std::array<std::uint8_t, 2> direction{};
    receive_bytes(direction.data(), direction.size());

    return game::MoveRequest{game::MovementDirection{decode_direction(direction[0]),
                                                     decode_direction(direction[1])}};
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
