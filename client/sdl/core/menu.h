#ifndef MENU_H
#define MENU_H

#include <string>

#include "SDL2pp/Font.hh"

#include "../render/renderer.h"

class Menu{
    private:
        bool abierto;
    public:
        Menu();
        bool esta_abierto() const;  

        bool procesar_click(int x, int y, int ancho_ventana, int alto_ventana);

        void dibujar(Renderer& renderer, SDL2pp::Font& fuente, const std::string& codigo_invitacion, int ancho_ventana, int alto_ventana) const;
};

#endif
