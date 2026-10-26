#include "map_validation.h"

#include "lobby/lobby_types.h"
#include "lobby/lobby_validation.h"

namespace editor {

namespace {

std::string quoted(Tile tile) { return std::string("«") + tile_name(tile) + "»"; }

void require_exactly_one(const std::string& place, const Zone& zone, Tile tile,
                         std::vector<MapProblem>& problems) {
    const int count = zone.count(tile);
    if (count == 0) {
        problems.push_back({place, "falta " + quoted(tile)});
    } else if (count > 1) {
        problems.push_back({place, "sobra " + quoted(tile) + ": hay " + std::to_string(count) +
                                           " y tiene que haber 1"});
    }
}

void require_at_least_one(const std::string& place, const Zone& zone, Tile tile,
                          std::vector<MapProblem>& problems) {
    if (zone.count(tile) == 0) {
        problems.push_back({place, "falta " + quoted(tile)});
    }
}

}

std::vector<MapProblem> validate(const WorldMap& map) {
    std::vector<MapProblem> problems;

    if (!lobby::is_valid_game_name(map.name)) {
        problems.push_back({"Nombre del mundo",
                            "tiene que tener entre 1 y " +
                                    std::to_string(lobby::MAX_GAME_NAME_LENGTH) +
                                    " caracteres: letras, números, espacios, guion o apóstrofo"});
    }

    require_exactly_one("Granja", map.farm, Tile::Cabin, problems);
    require_at_least_one("Granja", map.farm, Tile::ExitToTown, problems);

    for (Tile tile: {Tile::Marnie, Tile::Clint, Tile::Pierre, Tile::Robin, Tile::CommunityCenter,
                     Tile::DungeonEntrance}) {
        require_exactly_one("Pueblo", map.town, tile, problems);
    }
    require_at_least_one("Pueblo", map.town, Tile::ExitToFarm, problems);
    return problems;
}

}
