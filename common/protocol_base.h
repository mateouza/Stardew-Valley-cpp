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
    static constexpr std::uint8_t GAME_STATE_MESSAGE = 0x03;
    static constexpr std::uint32_t MAX_PLAYER_NAME_BYTES = 256;
    static constexpr std::uint32_t MAX_PLAYERS_PER_STATE = 1024;
    static constexpr std::uint8_t CREATE_MATCH_MESSAGE = 0x04;
    static constexpr std::uint8_t JOIN_MATCH_MESSAGE = 0x05;
    static constexpr std::uint32_t MAX_MATCH_NAME_BYTES = 256;
    static constexpr std::uint32_t MAX_INVITATION_CODE_BYTES = 256;

    explicit ProtocolBase(Socket& socket);
    ~ProtocolBase() = default;

    std::optional<std::uint8_t> receive_first_byte();
    void receive_bytes(void* data, unsigned int size);
    void send_bytes(const void* data, unsigned int size);
    void send_uint32(std::uint32_t value);
    void send_double(double value);
    void send_string(const std::string& value);
    std::uint32_t receive_uint32();
    double receive_double();
    std::string receive_string(std::uint32_t max_length);
};

#endif
