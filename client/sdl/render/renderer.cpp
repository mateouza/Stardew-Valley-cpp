#include "renderer.h"

#include <SDL2/SDL_image.h>

Renderer::Renderer(SDL2pp::Window& window):
        renderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC) {}

Renderer::~Renderer() = default;

SDL2pp::Texture Renderer::cargar_textura(const std::string& ruta) {
    return SDL2pp::Texture(renderer, ruta);
}

SDL2pp::Texture Renderer::cargar_textura_fuente(SDL2pp::Font& fuente, const std::string& texto, Uint8 r, Uint8 g, Uint8 b) {
    return SDL2pp::Texture(renderer, fuente.RenderText_Blended(texto, SDL_Color{r, g, b, 255}));
}

void Renderer::limpiar() {
    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.Clear();
}


void Renderer::dibujar(SDL2pp::Texture& textura, int x, int y, int ancho, int alto, Uint8 r, Uint8 g, Uint8 b) {
    textura.SetColorMod(r, g, b);
    renderer.Copy(textura, SDL2pp::NullOpt, SDL2pp::Rect(x, y, ancho, alto));
}

void Renderer::dibujar(SDL2pp::Texture& textura, const SDL2pp::Rect& origen, int x, int y, int ancho, int alto, Uint8 r, Uint8 g, Uint8 b) {
    renderer.SetDrawColor(r, g, b, 255);
    renderer.Copy(textura, origen, SDL2pp::Rect(x, y, ancho, alto));
}

void Renderer::dibujar_rectangulo(int x, int y, int ancho, int alto, Uint8 r, Uint8 g, Uint8 b) {
    renderer.SetDrawColor(r, g, b, 255);
    renderer.FillRect(SDL2pp::Rect(x, y, ancho, alto));
}

void Renderer::presentar() { renderer.Present(); }