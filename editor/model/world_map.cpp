#include "world_map.h"

namespace editor {

WorldMap WorldMap::create_empty(const std::string& name) {
    const Zone level(ZoneKind::Dungeon, LEVEL_WIDTH, LEVEL_HEIGHT);

    return {name,
            Zone(ZoneKind::Farm, SURFACE_WIDTH, SURFACE_HEIGHT),
            Zone(ZoneKind::Town, SURFACE_WIDTH, SURFACE_HEIGHT),
            {level, level, level, level, level, level, level, level, level, level}};
}

}
