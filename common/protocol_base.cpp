#include "protocol_base.h"

#include <stdexcept>

ProtocolBase::ProtocolBase(Socket& socket): socket(socket) {}

std::optional<std::uint8_t> ProtocolBase::receive_first_byte() {
    std::uint8_t byte;

    if (socket.recvall(&byte, sizeof(byte)) == 0) {
        return std::nullopt;
    }

    return byte;
}

void ProtocolBase::receive_bytes(void* data, unsigned int size) {
    if (size == 0) {
        return;
    }

    if (socket.recvall(data, size) == 0) {
        throw std::runtime_error("Mensaje incompleto");
    }
}

void ProtocolBase::send_bytes(const void* data, unsigned int size) {
    if (size == 0) {
        return;
    }

    if (socket.sendall(data, size) == 0) {
        throw std::runtime_error("Conexion cerrada durante el envio");
    }
}
