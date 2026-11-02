#ifndef INVENTORY_BAR_H
#define INVENTORY_BAR_H

#include <cstddef>

#include "render/renderer.h"

class InventoryBar {
    private:
        static constexpr std::size_t CANTIDAD_SLOTS = 9;
        std::size_t slot_seleccionado;
    
    public:
        InventoryBar();
        void seleccionar(std::size_t indice);
        std::size_t obtener_seleccionado() const;
        void dibujar(Renderer& renderer, int ancho_ventana, int alto_ventana) const;
};

#endif
