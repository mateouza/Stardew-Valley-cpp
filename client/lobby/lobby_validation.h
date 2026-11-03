#ifndef CLIENT_LOBBY_LOBBY_VALIDATION_H_
#define CLIENT_LOBBY_LOBBY_VALIDATION_H_

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include "lobby_types.h"

namespace lobby {

bool is_valid_username(std::string_view username);

bool is_valid_port(std::string_view port);

bool is_valid_invite_code(std::string_view invite_code);

std::string trim_spaces(std::string_view text);

std::optional<std::size_t> count_name_characters(std::string_view name);

bool is_valid_game_name(std::string_view name);

bool is_valid_character_name(std::string_view name);

bool is_valid_appearance(const CharacterAppearance& appearance);

std::optional<LobbyError> validate_character(const CharacterDraft& character);

}

#endif
