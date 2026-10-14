#ifndef SDL_WINDOW_H
#define SDL_WINDOW_H

#include <string>
#include <SDL2/SDL.h>

class SDLWindow {
    private:
        SDL_Window* window;

    public:
        SDLWindow(const std::string& titulo, int ancho, int alto, bool pantalla_completa);

        ~SDLWindow();

        SDLWindow(const SDLWindow&) = delete; // Evita la copia del objeto SDLWindow
        SDLWindow& operator=(const SDLWindow&) = delete; // Evita la asignación del objeto SDLWindow

        SDL_Window* obtener_handle() const; // devuelve el puntero al objeto SDL_Window
};

#endif
