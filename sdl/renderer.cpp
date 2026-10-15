#include "renderer.h"

#include <SDL2/SDL_image.h>

#include <stdexcept>

Renderer::Renderer(SDL2pp::Window& window) 
    : renderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC) {}


SDL2pp::Texture Renderer::cargar_textura(const std::string& ruta) {
    return SDL2pp::Texture(renderer, ruta);
}

void Renderer::limpiar() {
    // Limpia la pantalla con el color de fondo (negro por defecto)
    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.Clear();
}

void Renderer::dibujar(SDL2pp::Texture& textura, int x, int y, int ancho, int alto) {
    // Dibuja la textura en la posición (x, y) con el ancho y alto especificados
    renderer.Copy(textura, SDL2pp::NullOpt, SDL2pp::Rect(x, y, ancho, alto));
    //NullOpt para no especificar una región de origen
    //Rect para especificar la posición y el tamaño de la textura en la pantalla
}

void Renderer::presentar() {
    // Presenta el contenido renderizado en la pantalla
    renderer.Present();
}