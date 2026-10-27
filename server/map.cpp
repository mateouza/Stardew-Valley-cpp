#include "map.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
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

MapData Map::get_data() const {
    const auto width = tiles.front().size();
    const auto height = tiles.size();
    if (!std::in_range<std::uint32_t>(width) || !std::in_range<std::uint32_t>(height)) {
        throw std::overflow_error("Las dimensiones del mapa no entran en MapData");
    }

    MapData data;
    data.width = static_cast<std::uint32_t>(width);
    data.height = static_cast<std::uint32_t>(height);
    if (height > data.tiles.max_size() / width) {
        throw std::length_error("El mapa supera la capacidad del vector de celdas");
    }

    data.tiles.reserve(width * height);
    for (const auto& row: tiles) {
        data.tiles.insert(data.tiles.end(), row.begin(), row.end());
    }
    return data;
}

Map::TileRange Map::get_tile_range(Position position, Position destination, double width,
                                   double height) const {
    const double map_width = static_cast<double>(tiles.front().size());
    const double map_height = static_cast<double>(tiles.size());
    const double left = std::max(0.0, std::min(position.x, destination.x) - width / 2.0);
    const double right = std::min(map_width, std::max(position.x, destination.x) + width / 2.0);
    const double top = std::max(0.0, std::min(position.y, destination.y) - height / 2.0);
    const double bottom = std::min(map_height, std::max(position.y, destination.y) + height / 2.0);

    return {static_cast<std::size_t>(std::floor(left)), static_cast<std::size_t>(std::ceil(right)),
            static_cast<std::size_t>(std::floor(top)), static_cast<std::size_t>(std::ceil(bottom))};
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

    const auto range = get_tile_range(position, position, width, height);
    for (std::size_t row = range.first_row; row < range.end_row; ++row) {
        for (std::size_t column = range.first_column; column < range.end_column; ++column) {
            if (tiles[row][column] == TileType::Blocked) {
                return false;
            }
        }
    }
    return true;
}

double Map::resolve_horizontal(Position position, double destination, double width,
                               double height) const {
    const double half_width = width / 2.0;
    const double map_width = static_cast<double>(tiles.front().size());
    const double target = std::clamp(destination, half_width, map_width - half_width);

    const auto range = get_tile_range(position, {target, position.y}, width, height);

    double result = target;
    for (std::size_t column = range.first_column; column < range.end_column; ++column) {
        for (std::size_t row = range.first_row; row < range.end_row; ++row) {
            if (tiles[row][column] == TileType::Blocked) {
                if (target > position.x) {
                    result = std::min(result, static_cast<double>(column) - half_width);
                } else {
                    result = std::max(result, static_cast<double>(column + 1) + half_width);
                }
            }
        }
    }
    return result;
}

double Map::resolve_vertical(Position position, double destination, double width,
                             double height) const {
    const double half_height = height / 2.0;
    const double map_height = static_cast<double>(tiles.size());
    const double target = std::clamp(destination, half_height, map_height - half_height);

    const auto range = get_tile_range(position, {position.x, target}, width, height);

    double result = target;
    for (std::size_t row = range.first_row; row < range.end_row; ++row) {
        for (std::size_t column = range.first_column; column < range.end_column; ++column) {
            if (tiles[row][column] == TileType::Blocked) {
                if (target > position.y) {
                    result = std::min(result, static_cast<double>(row) - half_height);
                } else {
                    result = std::max(result, static_cast<double>(row + 1) + half_height);
                }
            }
        }
    }
    return result;
}

Position Map::correct_rounding(Position position, Position result, double width,
                               double height) const {
    if (can_occupy(result, width, height)) {
        return result;
    }

    result.x = std::nextafter(result.x, position.x);
    result.y = std::nextafter(result.y, position.y);
    if (can_occupy(result, width, height)) {
        return result;
    }
    return position;
}

Position Map::resolve_movement(Position position, Position destination, double width,
                               double height) const {
    if (!can_occupy(position, width, height)) {
        throw std::invalid_argument("La posicion inicial y el cuerpo deben ser validos");
    }
    if (!std::isfinite(destination.x) || !std::isfinite(destination.y)) {
        throw std::invalid_argument("El destino debe tener coordenadas finitas");
    }

    Position horizontal = position;
    horizontal.x = resolve_horizontal(position, destination.x, width, height);
    horizontal = correct_rounding(position, horizontal, width, height);

    Position vertical = horizontal;
    vertical.y = resolve_vertical(horizontal, destination.y, width, height);
    return correct_rounding(horizontal, vertical, width, height);
}

}
