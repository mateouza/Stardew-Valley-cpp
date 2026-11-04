#include "texture_manager.h"


TextureManager::TextureManager(Renderer& renderer) : renderer(renderer) {}

SDL2pp::Texture& TextureManager::obtener_textura(const std::string& ruta) {
    std::map<std::string, SDL2pp::Texture>::iterator encontrada = textures.find(ruta);
    if (encontrada != textures.end()) {
        return encontrada->second;
    }

    std::pair<std::map<std::string, SDL2pp::Texture>::iterator, bool> insertada = textures.emplace(ruta, renderer.cargar_textura(ruta));
    return insertada.first->second; // Devuelve la referencia a la textura recién insertada
}
