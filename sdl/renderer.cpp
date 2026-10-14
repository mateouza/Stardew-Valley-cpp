#include "renderer.h"

#include <SDL2/SDL_image.h>

#include <stdexcept>

Renderer::Renderer(SDL_Window* window) {
    // Crea un renderer asociado a la ventana proporcionada
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        throw std::runtime_error("Error al crear el renderer: " + std::string(SDL_GetError()));
    }

    if (IMG_Init(IMG_INIT_PNG) == 0){
        SDL_DestroyRenderer(renderer);
        throw std::runtime_error(IMG_GetError());
    }
}

Renderer::~Renderer() {
    IMG_Quit();
    SDL_DestroyRenderer(renderer);
}

SDL_Texture* Renderer::cargar_textura(const std::string& ruta) const {
    // Carga una textura desde un archivo
    SDL_Surface* surface = IMG_Load(ruta.c_str());
    if (!surface) {
        throw std::runtime_error("Error al cargar la textura: " + std::string(IMG_GetError()));
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!texture) {
        throw std::runtime_error("Error al crear la textura: " + std::string(SDL_GetError()));
    }

    return texture;
}

void Renderer::limpiar() {
    // Limpia la pantalla con el color de fondo (negro por defecto)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

void Renderer::dibujar(SDL_Texture* textura, int x, int y, int ancho, int alto) {
    // Dibuja la textura en la posición (x, y) con el ancho y alto especificados
    SDL_Rect dstRect = {x, y, ancho, alto};
    SDL_RenderCopy(renderer, textura, nullptr, &dstRect);
}

void Renderer::presentar() {
    // Presenta el contenido renderizado en la pantalla
    SDL_RenderPresent(renderer);
}