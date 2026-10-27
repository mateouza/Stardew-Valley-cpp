#include "protocol_base.h"

#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

#include <arpa/inet.h>

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

void ProtocolBase::send_uint32(std::uint32_t value) {
    const std::uint32_t network_value = htonl(value);
    send_bytes(&network_value, sizeof(network_value));
}

void ProtocolBase::send_double(double value) {
    static_assert(sizeof(double) == sizeof(std::uint64_t));
    static_assert(std::numeric_limits<double>::is_iec559);

    if (!std::isfinite(value)) {
        throw std::invalid_argument("La coordenada debe ser finita");
    }

    const auto bits = std::bit_cast<std::uint64_t>(value);

    send_uint32(static_cast<std::uint32_t>(bits >> 32));
    send_uint32(static_cast<std::uint32_t>(bits));
}

void ProtocolBase::send_string(const std::string& value) {
    if (value.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw std::length_error("El texto supera la longitud permitida por el protocolo");
    }

    const auto length = static_cast<std::uint32_t>(value.size());

    send_uint32(length);
    send_bytes(value.data(), length);
}

std::uint32_t ProtocolBase::receive_uint32() {
    std::uint32_t network_value{};
    receive_bytes(&network_value, sizeof(network_value));

    return ntohl(network_value);
}

double ProtocolBase::receive_double() {
    static_assert(sizeof(double) == sizeof(std::uint64_t));
    static_assert(std::numeric_limits<double>::is_iec559);

    const std::uint64_t high = receive_uint32();
    const std::uint64_t low = receive_uint32();
    const std::uint64_t bits = (high << 32) | low;

    const double value = std::bit_cast<double>(bits);

    if (!std::isfinite(value)) {
        throw std::runtime_error("La coordenada recibida debe ser finita");
    }

    return value;
}

std::string ProtocolBase::receive_string(std::uint32_t max_length) {
    const auto length = receive_uint32();

    if (length > max_length) {
        throw std::length_error("El texto recibido supera la longitud permitida");
    }

    std::string value(length, '\0');
    receive_bytes(value.data(), length);

    return value;
}
