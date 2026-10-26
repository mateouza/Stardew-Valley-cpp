#include "zone.h"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace editor {

Zone::Zone(ZoneKind kind, int width, int height):
        zone_kind(kind), zone_width(width), zone_height(height) {
    if (width < 1 || width > MAX_SIDE || height < 1 || height > MAX_SIDE) {
        throw std::invalid_argument("El ancho y el alto de una zona tienen que estar entre 1 y " +
                                    std::to_string(MAX_SIDE));
    }
    tiles.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height),
                 default_tile(kind));
}

ZoneKind Zone::kind() const { return zone_kind; }

int Zone::width() const { return zone_width; }

int Zone::height() const { return zone_height; }

bool Zone::contains(int column, int row) const {
    return column >= 0 && column < zone_width && row >= 0 && row < zone_height;
}

Tile Zone::at(int column, int row) const { return tiles[index_of(column, row)]; }

void Zone::set(int column, int row, Tile tile) {
    const std::size_t index = index_of(column, row);
    if (!is_allowed(zone_kind, tile)) {
        throw std::invalid_argument(std::string(tile_name(tile)) +
                                    " no se puede poner en esta zona");
    }
    tiles[index] = tile;
}

int Zone::count(Tile tile) const {
    return static_cast<int>(std::count(tiles.begin(), tiles.end(), tile));
}

std::size_t Zone::index_of(int column, int row) const {
    if (!contains(column, row)) {
        throw std::out_of_range("La posición (" + std::to_string(column) + ", " +
                                std::to_string(row) + ") está fuera de la zona");
    }
    return static_cast<std::size_t>(row) * static_cast<std::size_t>(zone_width) +
           static_cast<std::size_t>(column);
}

}
