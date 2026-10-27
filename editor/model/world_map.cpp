#include "world_map.h"

namespace editor {

WorldMap WorldMap::create_empty(const std::string& name) {
    const Zone level(ZoneKind::Dungeon, LEVEL_WIDTH, LEVEL_HEIGHT);

    // Zone no tiene constructor por defecto, así que hay que listar los DUNGEON_LEVELS niveles; si
    // cambia la cantidad, esta línea deja de compilar.
    return {name,
            Zone(ZoneKind::Farm, SURFACE_WIDTH, SURFACE_HEIGHT),
            Zone(ZoneKind::Town, SURFACE_WIDTH, SURFACE_HEIGHT),
            {level, level, level, level, level, level, level, level, level, level}};
}

}
