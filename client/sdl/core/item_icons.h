#ifndef ITEM_ICONS_H
#define ITEM_ICONS_H

#include <string>
#include <optional>

#include <SDL2pp/Rect.hh>

#include "item.h"

struct IconItem{
    std::string ruta;
    std::optional<SDL2pp::Rect> origen; //vacio: usar toda la textura, con valor: usar solo la subregion de la textura
};

std::optional<IconItem> obtener_icono_item(game::ItemType tipo_item);

#endif
