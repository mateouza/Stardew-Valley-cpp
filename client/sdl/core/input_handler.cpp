#include "input_handler.h"

#include <cstdint>

#include <SDL.h>


InputResultado InputHandler::procesar_eventos() {
    InputResultado resultado{{0, 0}, false, std::nullopt, std::nullopt, {0, 0}, false};
    SDL_Event evento;
    while (SDL_PollEvent(&evento)) {
        if (evento.type == SDL_QUIT) {
            resultado.salir = true;
        }
        if (evento.type == SDL_KEYDOWN) {
            const SDL_Scancode tecla = evento.key.keysym.scancode;
            if (tecla >= SDL_SCANCODE_1 && tecla <= SDL_SCANCODE_9) {
                resultado.slot_seleccionado = static_cast<std::size_t>(tecla - SDL_SCANCODE_1);
            } else if (tecla >= SDL_SCANCODE_KP_1 && tecla <= SDL_SCANCODE_KP_9) {
                resultado.slot_seleccionado = static_cast<std::size_t>(tecla - SDL_SCANCODE_KP_1);
            }
        }
        if (evento.type == SDL_MOUSEBUTTONDOWN && evento.button.button == SDL_BUTTON_LEFT) {
            resultado.click_mouse = ClickMouse{evento.button.x, evento.button.y};
        }
    }
    
    int x_mouse = 0;
    int y_mouse = 0;

    const Uint32 estado_mouse = SDL_GetMouseState(&x_mouse, &y_mouse);
    resultado.posicion_mouse = ClickMouse{x_mouse, y_mouse};
    resultado.click_izquierdo_presionado = (estado_mouse & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;

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