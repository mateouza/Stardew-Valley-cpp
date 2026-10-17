#include "input_handler.h"

#include <cstdint>

#include <SDL.h>


InputResultado InputHandler::procesar_eventos() {
    InputResultado resultado{{0, 0}, false};
    SDL_Event evento;
    while (SDL_PollEvent(&evento)) {
        if (evento.type == SDL_QUIT) {
            resultado.salir = true;
        }
    }

    const Uint8* teclas = SDL_GetKeyboardState(nullptr);

    std::int8_t horizontal = 0;
    std::int8_t vertical = 0;

    if (teclas[SDL_SCANCODE_UP] || teclas[SDL_SCANCODE_W]) {
        vertical -= 1;
    }
    if (teclas[SDL_SCANCODE_DOWN] || teclas[SDL_SCANCODE_S]) {
        vertical += 1;
    }
    if (teclas[SDL_SCANCODE_LEFT] || teclas[SDL_SCANCODE_A]) {
        horizontal -= 1;
    }
    if (teclas[SDL_SCANCODE_RIGHT] || teclas[SDL_SCANCODE_D]) {
        horizontal += 1;
    }

    resultado.direccion = game::MovementDirection{horizontal, vertical};
    return resultado;
}