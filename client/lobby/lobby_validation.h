#ifndef CLIENT_LOBBY_LOBBY_VALIDATION_H_
#define CLIENT_LOBBY_LOBBY_VALIDATION_H_

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace lobby {

// Reglas que comparten el lobby y las pantallas, para que los dos acepten exactamente lo mismo.

// Letras ASCII, dígitos y guion bajo, entre 1 y MAX_USERNAME_LENGTH caracteres.
bool is_valid_username(std::string_view username);

// Número decimal entre 1 y 65535, sin signo ni espacios.
bool is_valid_port(std::string_view port);

// Saca los espacios (U+0020) de los extremos. Es el único espacio que admiten los nombres.
std::string trim_spaces(std::string_view text);

// Cantidad de caracteres de un nombre en UTF-8, o nada si tiene bytes inválidos o algún carácter
// fuera de la lista: A-Z, a-z, áéíóúüñ, ÁÉÍÓÚÜÑ, dígitos, espacio, guion y apóstrofo. Un carácter
// es un punto de código; con esta lista también es una unidad de QString y un símbolo en pantalla.
std::optional<std::size_t> count_name_characters(std::string_view name);

// Sin los espacios de los extremos, entre 1 y MAX_GAME_NAME_LENGTH caracteres de la lista.
bool is_valid_game_name(std::string_view name);

// Sin los espacios de los extremos, entre 1 y MAX_CHARACTER_NAME_LENGTH caracteres de la lista.
bool is_valid_character_name(std::string_view name);

}

#endif
