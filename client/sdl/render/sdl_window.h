#ifndef SDL_WINDOW_H
#define SDL_WINDOW_H

#include <string>

#include <SDL2pp/Window.hh>

class SDLWindow {
private:
    SDL2pp::Window window;

    static Uint32 obtener_flags(bool pantalla_completa);

public:
    SDLWindow(const std::string& titulo, int ancho, int alto, bool pantalla_completa);

    SDL2pp::Window& obtener_handle();
};

#endif
