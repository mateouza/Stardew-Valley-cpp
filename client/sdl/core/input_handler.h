#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <SDL2/SDL.h>
#include "../state/client_state.h"

class InputHandler {
    private:
        static constexpr int VELOCIDAD = 5;

    public:
        bool procesar_eventos(ClientState& estado_cliente);
};

#endif