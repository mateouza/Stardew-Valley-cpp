#ifndef RENDERER_H
#define RENDERER_H

#include <string>
#include <SDL2/SDL.h>

class Renderer{
    private: 
        SDL_Renderer* renderer;
    
    public:
        explicit Renderer(SDL_Window* window);
        ~Renderer();

        // Evita la copia y asignación del objeto Renderer
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        // Carga una textura desde un archivo y devuelve un puntero a SDL_Texture
        SDL_Texture* cargar_textura(const std::string& ruta) const;

        void limpiar(); // limpia la pantalla
        
        // dibuja la textura en la posición (x, y) con el ancho y alto especificados
        void dibujar(SDL_Texture* textura, int x, int y, int ancho, int alto); 
        
        void presentar(); // presenta el contenido renderizado en la pantalla

};

#endif
