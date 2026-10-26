#include "lobby_validation.h"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <system_error>

#include "lobby_types.h"

namespace lobby {

namespace {

constexpr std::uint32_t MIN_PORT = 1;
constexpr std::uint32_t MAX_PORT = 65535;

bool is_username_character(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

}

bool is_valid_username(std::string_view username) {
    return !username.empty() && username.size() <= MAX_USERNAME_LENGTH &&
           std::all_of(username.begin(), username.end(), is_username_character);
}

bool is_valid_port(std::string_view port) {
    const char* const end = port.data() + port.size();
    std::uint32_t value = 0;
    const auto [parsed_until, error] = std::from_chars(port.data(), end, value);
    return !port.empty() && error == std::errc() && parsed_until == end && value >= MIN_PORT &&
           value <= MAX_PORT;
}

}
