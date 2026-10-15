#include "map.h"

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace game {

Map::Map(std::vector<std::vector<TileType>> tiles): tiles(std::move(tiles)) {
    if (this->tiles.empty() || this->tiles.front().empty()) {
        throw std::invalid_argument("El mapa debe tener al menos una celda");
    }

    const auto width = this->tiles.front().size();
    for (const auto& row: this->tiles) {
        if (row.size() != width) {
            throw std::invalid_argument("Las filas del mapa deben tener el mismo ancho");
        }
        for (const auto tile: row) {
            if (tile != TileType::Walkable && tile != TileType::Blocked) {
                throw std::invalid_argument("El mapa contiene un tipo de celda invalido");
            }
        }
    }
}

bool Map::can_occupy(Position position, double width, double height) const {
    if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(width) ||
        !std::isfinite(height) || width <= 0.0 || height <= 0.0) {
        return false;
    }

    const double left = position.x - width / 2.0;
    const double right = position.x + width / 2.0;
    const double top = position.y - height / 2.0;
    const double bottom = position.y + height / 2.0;

    if (left < 0.0 || top < 0.0 || right > static_cast<double>(tiles.front().size()) ||
        bottom > static_cast<double>(tiles.size()) || left >= right || top >= bottom) {
        return false;
    }

    const auto first_column = static_cast<std::size_t>(std::floor(left));
    const auto end_column = static_cast<std::size_t>(std::ceil(right));
    const auto first_row = static_cast<std::size_t>(std::floor(top));
    const auto end_row = static_cast<std::size_t>(std::ceil(bottom));

    for (std::size_t row = first_row; row < end_row; ++row) {
        for (std::size_t column = first_column; column < end_column; ++column) {
            if (tiles[row][column] == TileType::Blocked) {
                return false;
            }
        }
    }
    return true;
}

}
