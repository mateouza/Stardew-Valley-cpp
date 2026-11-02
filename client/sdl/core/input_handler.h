#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <optional>

#include <SDL2/SDL.h>

#include "game_types.h"

struct InputResultado {
    game::MovementDirection direccion;
    bool salir;
    std::optional<std::size_t> slot_seleccionado;
};

class InputHandler {
public:
    InputResultado procesar_eventos();
};

#endif