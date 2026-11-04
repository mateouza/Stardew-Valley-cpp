#ifndef EDITOR_MODEL_ZONE_H_
#define EDITOR_MODEL_ZONE_H_

#include <cstddef>
#include <vector>

#include "tile.h"

namespace editor {

class Zone {
private:
    ZoneKind zone_kind;
    int zone_width;
    int zone_height;
    std::vector<Tile> tiles;

    std::size_t index_of(int column, int row) const;

public:
    static constexpr int MAX_SIDE = 200;

    Zone(ZoneKind kind, int width, int height);

    ZoneKind kind() const;
    int width() const;
    int height() const;

    bool contains(int column, int row) const;

    Tile at(int column, int row) const;

    void set(int column, int row, Tile tile);

    int count(Tile tile) const;

    bool operator==(const Zone& other) const = default;
};

}

#endif
