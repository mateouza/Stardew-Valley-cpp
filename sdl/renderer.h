#ifndef RENDERER_H
#define RENDERER_H

#include <string>
#include <SDL2pp/Rect.hh>
#include <SDL2pp/Renderer.hh>
#include <SDL2pp/Texture.hh>
#include <SDL2pp/Window.hh>

class Renderer{
    private: 
        SDL2pp::Renderer renderer;
    
    public:
        explicit Renderer(SDL2pp::Window& window);
        ~Renderer();

        // Carga una textura desde un archivo y devuelve un puntero a SDL_Texture
        SDL2pp::Texture cargar_textura(const std::string& ruta);

        void limpiar(); // limpia la pantalla
        
        // dibuja la textura en la posición (x, y) con el ancho y alto especificados
        void dibujar(SDL2pp::Texture& textura, int x, int y, int ancho, int alto); 
        
        void presentar(); // presenta el contenido renderizado en la pantalla

};

#endif
