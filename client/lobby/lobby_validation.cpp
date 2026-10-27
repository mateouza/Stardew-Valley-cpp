#include "lobby_validation.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <system_error>

#include "lobby_types.h"

namespace lobby {

namespace {

constexpr std::uint32_t MIN_PORT = 1;
constexpr std::uint32_t MAX_PORT = 65535;

// á é í ó ú ü ñ y sus mayúsculas.
constexpr std::array<char32_t, 14> ACCENTED_LETTERS{0xE1, 0xE9, 0xED, 0xF3, 0xFA, 0xFC, 0xF1,
                                                    0xC1, 0xC9, 0xCD, 0xD3, 0xDA, 0xDC, 0xD1};

bool is_username_character(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

bool is_ascii_name_character(unsigned char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' ||
           c == '-' || c == '\'';
}

// Bytes que ocupa el primer carácter de `text` si está en la lista de los nombres, o 0 si no.
// Las letras con tilde admitidas se codifican en UTF-8 con dos bytes: 110xxxxx 10xxxxxx. Un byte
// de continuación suelto, una secuencia cortada o una forma sobrelarga no dan una letra admitida.
std::size_t admitted_character_length(std::string_view text) {
    const auto lead = static_cast<unsigned char>(text[0]);
    if (lead < 0x80) {
        return is_ascii_name_character(lead) ? 1 : 0;
    }

    if ((lead & 0xE0) != 0xC0 || text.size() < 2) {
        return 0;
    }

    const auto next = static_cast<unsigned char>(text[1]);
    if ((next & 0xC0) != 0x80) {
        return 0;
    }

    const char32_t code_point = ((lead & 0x1Fu) << 6) | (next & 0x3Fu);
    const bool admitted = std::find(ACCENTED_LETTERS.begin(), ACCENTED_LETTERS.end(), code_point) !=
                          ACCENTED_LETTERS.end();
    return admitted ? 2 : 0;
}

bool is_valid_name(std::string_view name, std::size_t max_length) {
    const auto length = count_name_characters(trim_spaces(name));
    return length && *length > 0 && *length <= max_length;
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

std::string trim_spaces(std::string_view text) {
    const auto first = text.find_first_not_of(' ');
    if (first == std::string_view::npos) {
        return "";
    }

    const auto last = text.find_last_not_of(' ');
    return std::string(text.substr(first, last - first + 1));
}

std::optional<std::size_t> count_name_characters(std::string_view name) {
    std::size_t count = 0;
    while (!name.empty()) {
        const std::size_t length = admitted_character_length(name);
        if (length == 0) {
            return std::nullopt;
        }

        name.remove_prefix(length);
        ++count;
    }
    return count;
}

bool is_valid_game_name(std::string_view name) { return is_valid_name(name, MAX_GAME_NAME_LENGTH); }

bool is_valid_character_name(std::string_view name) {
    return is_valid_name(name, MAX_CHARACTER_NAME_LENGTH);
}

}
