#ifndef EDITOR_MODEL_ZONE_H_
#define EDITOR_MODEL_ZONE_H_

#include <cstddef>
#include <vector>

#include "tile.h"

namespace editor {

// Grilla de casillas de la granja, del pueblo o de un nivel de la mazmorra. Solo admite las
// casillas de su tipo de zona.
class Zone {
private:
    ZoneKind zone_kind;
    int zone_width;
    int zone_height;
    std::vector<Tile> tiles;

    std::size_t index_of(int column, int row) const;

public:
    // Lado máximo de una zona, para no reservar memoria de más al leer un archivo.
    static constexpr int MAX_SIDE = 200;

    // Arranca llena con la casilla inicial de su tipo. Lanza std::invalid_argument si el ancho o el
    // alto no están entre 1 y MAX_SIDE.
    Zone(ZoneKind kind, int width, int height);

    ZoneKind kind() const;
    int width() const;
    int height() const;

    bool contains(int column, int row) const;

    // Lanza std::out_of_range si la posición está fuera de la grilla.
    Tile at(int column, int row) const;

    // Lanza std::out_of_range como at(), y std::invalid_argument si la casilla no se admite en
    // esta zona. En los dos casos la zona queda como estaba.
    void set(int column, int row, Tile tile);

    int count(Tile tile) const;

    bool operator==(const Zone& other) const = default;
};

}

#endif
