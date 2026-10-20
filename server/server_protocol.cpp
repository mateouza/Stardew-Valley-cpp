#include "server_protocol.h"

#include <array>
#include <stdexcept>

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
