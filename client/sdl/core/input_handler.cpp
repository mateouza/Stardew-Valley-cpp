#include "input_handler.h"


bool InputHandler::procesar_eventos(ClientState& estado_cliente) {
    SDL_Event evento;
    while (SDL_PollEvent(&evento)) {
        if (evento.type == SDL_QUIT) {
            return false;
        }
    }
    const Uint8* estado_teclado = SDL_GetKeyboardState(nullptr);
    int dx = 0;
    int dy = 0;

    if (estado_teclado[SDL_SCANCODE_UP] || estado_teclado[SDL_SCANCODE_W]) {
        dy -= VELOCIDAD;
    }
    if (estado_teclado[SDL_SCANCODE_DOWN] || estado_teclado[SDL_SCANCODE_S]) {
        dy += VELOCIDAD;
    }
    if (estado_teclado[SDL_SCANCODE_LEFT] || estado_teclado[SDL_SCANCODE_A]) {
        dx -= VELOCIDAD;
    }
    if (estado_teclado[SDL_SCANCODE_RIGHT] || estado_teclado[SDL_SCANCODE_D]) {
        dx += VELOCIDAD;
    }

    if (dx != 0 || dy != 0) {
        estado_cliente.mover_a(estado_cliente.obtener_x() + dx, estado_cliente.obtener_y() + dy);
    }

    return true;
}