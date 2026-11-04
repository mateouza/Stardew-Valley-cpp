#ifndef INVENTORY_BAR_H
#define INVENTORY_BAR_H

#include <cstddef>

#include "SDL2pp/Font.hh"

#include "../render/renderer.h"
#include "../render/texture_manager.h"
#include "inventory_state.h"

class InventoryBar {
    private:
        static constexpr std::size_t CANTIDAD_SLOTS = game::InventoryState::CAPACITY;
        game::InventoryState estado_inventario;

        void dibujar_slot_vacio(Renderer& renderer, int x, int y, bool seleccionado) const;
        void dibujar_slot_ocupado(Renderer& renderer, TextureManager& texturas, SDL2pp::Font& fuente, const game::ItemStack& item, int x, int y, bool seleccionado) const;
    
    public:
        void actualizar(const game::InventoryState& nuevo_estado);

    void dibujar(Renderer& renderer, TextureManager& texturas, SDL2pp::Font& fuente, int ancho_ventana, int alto_ventana) const;
};

#endif
