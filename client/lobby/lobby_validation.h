#ifndef CLIENT_LOBBY_LOBBY_VALIDATION_H_
#define CLIENT_LOBBY_LOBBY_VALIDATION_H_

#include <string_view>

namespace lobby {

// Reglas que comparten el lobby y las pantallas, para que los dos acepten exactamente lo mismo.

// Letras ASCII, dígitos y guion bajo, entre 1 y MAX_USERNAME_LENGTH caracteres.
bool is_valid_username(std::string_view username);

// Número decimal entre 1 y 65535, sin signo ni espacios.
bool is_valid_port(std::string_view port);

}

#endif
