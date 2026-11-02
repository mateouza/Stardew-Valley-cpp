#include "inventory_bar.h"

namespace {
    constexpr int ANCHO_SLOT_PX = 48;
    constexpr int ALTO_SLOT_PX = 48;
    constexpr int SEPARACION_PX = 4;
    constexpr int MARGEN_INFERIOR_PX = 16;
}

InventoryBar::InventoryBar() : slot_seleccionado(0) {}

void InventoryBar::seleccionar(std::size_t indice) {
    if (indice < CANTIDAD_SLOTS) {
        slot_seleccionado = indice;
    }
}

std::size_t InventoryBar::obtener_seleccionado() const {
    return slot_seleccionado;
}

void InventoryBar::dibujar(Renderer& renderer, int ancho_ventana, int alto_ventana) const {
    const int ancho_total = static_cast<int>(CANTIDAD_SLOTS) * ANCHO_SLOT_PX + static_cast<int>(CANTIDAD_SLOTS - 1) * SEPARACION_PX;
    const int x_inicial = (ancho_ventana - ancho_total) / 2;
    const int y_inicial = alto_ventana - ALTO_SLOT_PX - MARGEN_INFERIOR_PX;

    for (std::size_t slot = 0; slot < CANTIDAD_SLOTS; ++slot) {
        const int x = x_inicial + static_cast<int>(slot) * (ANCHO_SLOT_PX + SEPARACION_PX);
        const int y = y_inicial;

        if (slot == slot_seleccionado) {
            renderer.dibujar_rectangulo(x, y, ANCHO_SLOT_PX, ALTO_SLOT_PX, 255, 255, 255);
        } else {
            renderer.dibujar_rectangulo(x, y, ANCHO_SLOT_PX, ALTO_SLOT_PX, 120, 120, 120);
        }
        renderer.dibujar_rectangulo(x+2, y+2, ANCHO_SLOT_PX-4, ALTO_SLOT_PX-4, 40, 40, 40);
    }
}