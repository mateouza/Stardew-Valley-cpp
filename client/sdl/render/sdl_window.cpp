#include "sdl_window.h"

#include <SDL.h>

Uint32 SDLWindow::obtener_flags(bool pantalla_completa) {
    Uint32 flags = SDL_WINDOW_SHOWN;
    if (pantalla_completa) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }
    return flags;
}

SDLWindow::SDLWindow(const std::string& titulo, int ancho, int alto, bool pantalla_completa) 
    : window(titulo, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ancho, alto, 
        obtener_flags(pantalla_completa)) {}


SDL2pp::Window& SDLWindow::obtener_handle() {
    return window;
}