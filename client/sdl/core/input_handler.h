#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <SDL2/SDL.h>

#include "../state/client_state.h"

#include "game_types.h"

struct InputResultado {
    game::MovementDirection direccion;
    bool salir;
};

class InputHandler {
public:
    InputResultado procesar_eventos();
};

#endif