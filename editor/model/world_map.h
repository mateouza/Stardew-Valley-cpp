#ifndef EDITOR_MODEL_WORLD_MAP_H_
#define EDITOR_MODEL_WORLD_MAP_H_

#include <array>
#include <string>

#include "zone.h"

namespace editor {

struct WorldMap {
    static constexpr int DUNGEON_LEVELS = 10;

    static constexpr int SURFACE_WIDTH = 40;
    static constexpr int SURFACE_HEIGHT = 30;
    static constexpr int LEVEL_WIDTH = 30;
    static constexpr int LEVEL_HEIGHT = 20;

    std::string name;
    Zone farm;
    Zone town;
    // dungeon[0] es el nivel 1, el de más arriba.
    std::array<Zone, DUNGEON_LEVELS> dungeon;

    static WorldMap create_empty(const std::string& name);

    bool operator==(const WorldMap& other) const = default;
};

}

#endif
