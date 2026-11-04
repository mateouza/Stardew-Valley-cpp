#ifndef RENDERER_H
#define RENDERER_H

#include <string>

#include <SDL2pp/Font.hh>
#include <SDL2pp/Rect.hh>
#include <SDL2pp/Renderer.hh>
#include <SDL2pp/Texture.hh>
#include <SDL2pp/Window.hh>

class Renderer {
private:
    SDL2pp::Renderer renderer;

public:
    explicit Renderer(SDL2pp::Window& ventana);
    ~Renderer();

    SDL2pp::Texture cargar_textura(const std::string& ruta);
    SDL2pp::Texture cargar_textura_fuente(SDL2pp::Font& fuente, const std::string& texto, Uint8 r, Uint8 g, Uint8 b);

    void limpiar();

    void dibujar(SDL2pp::Texture& textura, int x, int y, int ancho, int alto, Uint8 r, Uint8 g, Uint8 b);
    void dibujar(SDL2pp::Texture& textura, const SDL2pp::Rect& origen, int x, int y, int ancho, int alto, Uint8 r, Uint8 g, Uint8 b);

    void dibujar_rectangulo(int x, int y, int ancho, int alto, Uint8 r, Uint8 g, Uint8 b);

    void presentar();
};

#endif
