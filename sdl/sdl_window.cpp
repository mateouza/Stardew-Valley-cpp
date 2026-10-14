#include "sdl_window.h"

#include <stdexcept>

SDLWindow::SDLWindow(const std::string& titulo, int ancho, int alto, bool pantalla_completa) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { // si no se pudo inicializar SDL, lanza una excepción
        throw std::runtime_error("Error al inicializar SDL: " + std::string(SDL_GetError()));
    }

    Uint32 flags = SDL_WINDOW_SHOWN; // flag para mostrar la ventana
    if (pantalla_completa) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }

    // Crea la ventana con el título, ancho, alto y flags especificados
    window = SDL_CreateWindow(titulo.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ancho, alto, flags);
    
    if (!window) {
        SDL_Quit();
        throw std::runtime_error("Error al crear la ventana: " + std::string(SDL_GetError()));
    }
}

SDLWindow::~SDLWindow() {
    SDL_DestroyWindow(window);
    SDL_Quit();
}

SDL_Window* SDLWindow::obtener_handle() const {
    return window;
}