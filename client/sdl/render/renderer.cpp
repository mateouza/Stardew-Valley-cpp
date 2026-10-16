#include "renderer.h"

#include <SDL2/SDL_image.h>

#include <stdexcept>

Renderer::Renderer(SDL2pp::Window& window) 
    : renderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC) {}

Renderer::~Renderer() = default;

SDL2pp::Texture Renderer::cargar_textura(const std::string& ruta) {
    return SDL2pp::Texture(renderer, ruta);
}

void Renderer::limpiar() {
    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.Clear();
}

void Renderer::dibujar(SDL2pp::Texture& textura, int x, int y, int ancho, int alto) {
    renderer.Copy(textura, SDL2pp::NullOpt, SDL2pp::Rect(x, y, ancho, alto));
}

void Renderer::presentar() {
    renderer.Present();
}