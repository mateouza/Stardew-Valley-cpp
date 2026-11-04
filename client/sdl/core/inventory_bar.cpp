#include "inventory_bar.h"

#include <string>

#include "item_icons.h"

namespace {
    constexpr int ANCHO_SLOT_PX = 48;
    constexpr int ALTO_SLOT_PX = 48;
    constexpr int SEPARACION_PX = 4;
    constexpr int MARGEN_INFERIOR_PX = 16;
    constexpr int MARGEN_ICONO_PX = 6;
}

void InventoryBar::actualizar(const game::InventoryState& nuevo_estado) {
    estado_inventario = nuevo_estado;
}

void InventoryBar::dibujar_slot_vacio(Renderer& renderer, int x, int y, bool seleccionado) const {
    if (seleccionado) {
        renderer.dibujar_rectangulo(x, y, ANCHO_SLOT_PX, ALTO_SLOT_PX, 255, 255, 255);
    } else {
        renderer.dibujar_rectangulo(x, y, ANCHO_SLOT_PX, ALTO_SLOT_PX, 120, 120, 120);
    }
    renderer.dibujar_rectangulo(x+2, y+2, ANCHO_SLOT_PX-4, ALTO_SLOT_PX-4, 40, 40, 40);
}

void InventoryBar::dibujar_slot_ocupado(Renderer& renderer, TextureManager& texturas, SDL2pp::Font& fuente, const game::ItemStack& item, int x, int y, bool seleccionado) const {
    dibujar_slot_vacio(renderer, x, y, seleccionado); //fondo + borde

    const std::optional<IconItem> icono = obtener_icono_item(item.type);
    const int lado_icono = ANCHO_SLOT_PX - MARGEN_ICONO_PX * 2;

    if (icono){
        SDL2pp::Texture& textura = texturas.obtener_textura(icono->ruta);
        if (icono->origen) {
            renderer.dibujar(textura, *icono->origen, x + MARGEN_ICONO_PX, y + MARGEN_ICONO_PX, lado_icono, lado_icono, 255, 255, 255);
        } else {
            renderer.dibujar(textura, x + MARGEN_ICONO_PX, y + MARGEN_ICONO_PX, lado_icono, lado_icono, 255, 255, 255);
        }
    } else { // si no hay icono
        renderer.dibujar_rectangulo(x + MARGEN_ICONO_PX, y + MARGEN_ICONO_PX, lado_icono, lado_icono, 90, 140, 220);
    }

    const std::string texto_cantidad = std::to_string(item.quantity);
    SDL2pp::Texture textura_cantidad = renderer.cargar_textura_fuente(fuente, texto_cantidad, 255, 255, 255);
    const int x_cant = x + ANCHO_SLOT_PX - textura_cantidad.GetWidth() - 2;
    const int y_cant = y + ALTO_SLOT_PX - textura_cantidad.GetHeight() - 2;
    renderer.dibujar(textura_cantidad, x_cant, y_cant, textura_cantidad.GetWidth(), textura_cantidad.GetHeight(), 255, 255, 255);
}

void InventoryBar::dibujar(Renderer& renderer, TextureManager& texturas, SDL2pp::Font& fuente, int ancho_ventana, int alto_ventana) const {
    const int ancho_total = static_cast<int>(CANTIDAD_SLOTS) * ANCHO_SLOT_PX + (static_cast<int>(CANTIDAD_SLOTS - 1) * SEPARACION_PX);
    const int x_inicial = (ancho_ventana - ancho_total) / 2;
    const int y = alto_ventana - ALTO_SLOT_PX - MARGEN_INFERIOR_PX;

    for (std::size_t slot = 0; slot < CANTIDAD_SLOTS; ++slot) {
        const int x = x_inicial + static_cast<int>(slot) * (ANCHO_SLOT_PX + SEPARACION_PX);
        const bool seleccionado = (slot == estado_inventario.selected_slot);

        if (estado_inventario.items[slot]) {
            dibujar_slot_ocupado(renderer, texturas, fuente, *estado_inventario.items[slot], x, y, seleccionado);
        } else {
            dibujar_slot_vacio(renderer, x, y, seleccionado);
        }
    }
}