#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <SDL2/SDL.h>

#include "game_types.h"
#include "../state/client_state.h"

struct InputResultado{
    game::MovementDirection direccion;
    bool salir;
};

class InputHandler {
    public:
        InputResultado procesar_eventos();
};

#endif