#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <cstddef>
#include <optional>

#include <SDL2/SDL.h>

#include "game_types.h"

struct ClickMouse {
    int x;
    int y;
};

struct InputResultado {
    game::MovementDirection direccion;
    bool salir;
    std::optional<std::size_t> slot_seleccionado;
    std::optional<ClickMouse> click_mouse;
    ClickMouse posicion_mouse;
    bool click_izquierdo_presionado;
};

class InputHandler {
public:
    InputResultado procesar_eventos();
};

#endif