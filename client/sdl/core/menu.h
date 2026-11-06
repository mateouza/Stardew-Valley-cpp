#ifndef MENU_H
#define MENU_H

#include <string>

#include "SDL2pp/Font.hh"
#include "SDL2pp/Rect.hh"

#include "../render/renderer.h"

class Menu{
    private:
        bool abierto;
        double volumen_musica; //0.0 a 1.0
        bool arrastrando_perilla;

        SDL2pp::Rect calcular_barra_musica(SDL2pp::Font& fuente, int ancho_ventana, int alto_ventana) const;
    public:
        Menu();
        bool esta_abierto() const;
        double obtener_volumen_musica() const;  

        bool procesar_click(int x, int y, int ancho_ventana, int alto_ventana, SDL2pp::Font& fuente);
        void procesar_arrastre(int x, int y, bool boton_presionado, int ancho_ventana, int alto_ventana, SDL2pp::Font& fuente);

        void dibujar(Renderer& renderer, SDL2pp::Font& fuente, const std::string& codigo_invitacion, int ancho_ventana, int alto_ventana) const;
};

#endif
