#include "server_protocol.h"

#include <array>
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
