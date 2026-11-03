#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <map>
#include <string>

#include <SDL2pp/Texture.hh>

#include "renderer.h"

class TextureManager {
    private:
        Renderer& renderer;
        std::map<std::string, SDL2pp::Texture> textures;
    public:
        explicit TextureManager(Renderer& renderer);
        SDL2pp::Texture& obtener_textura(const std::string& ruta);
};

#endif
