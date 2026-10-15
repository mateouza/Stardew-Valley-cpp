#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <SDL2/SDL.h>
#include "../state/client_state.h"

class InputHandler {
    private:
        static constexpr int VELOCIDAD = 5; // pixeles que se mueve por frame

    public:
        /* Recorre los eventos pendientes de SDL (por ahora solo nos interesa
           detectar el cierre de la ventana) y, con el estado actual del
           teclado, actualiza la posición del jugador en 'estado'.
           Devuelve false si se pidió cerrar el juego (SDL_QUIT), true si hay
           que seguir corriendo.
        */
        bool procesar_eventos(ClientState& estado_cliente);
};

#endif