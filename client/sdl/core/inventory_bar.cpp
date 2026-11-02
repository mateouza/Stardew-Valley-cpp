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