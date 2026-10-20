#ifndef PROTOCOL_BASE_H
#define PROTOCOL_BASE_H

#include <cstdint>
#include <optional>
#include <string>

#include "socket.h"

class ProtocolBase {
private:
    Socket& socket;

protected:
    static constexpr std::uint8_t MOVE_MESSAGE = 0x01;
    static constexpr std::uint8_t JOIN_RESPONSE_MESSAGE = 0x02;
    static constexpr std::uint8_t JOIN_ACCEPTED = 0x00;
    static constexpr std::uint8_t JOIN_GAME_FULL = 0x01;

    explicit ProtocolBase(Socket& socket);
    ~ProtocolBase() = default;

    std::optional<std::uint8_t> receive_first_byte();
    void receive_bytes(void* data, unsigned int size);
    void send_bytes(const void* data, unsigned int size);
    void send_uint32(std::uint32_t value);
    void send_double(double value);
    void send_string(const std::string& value);
};

#endif
