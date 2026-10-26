#include <string>
#include <vector>

#include "editor/model/map_validation.h"
#include "gtest/gtest.h"

namespace editor {

namespace {

using Problems = std::vector<std::string>;

const Tile UNIQUE_TOWN_TILES[] = {Tile::Marnie, Tile::Clint,           Tile::Pierre,
                                  Tile::Robin,  Tile::CommunityCenter, Tile::DungeonEntrance};

// Mundo que cumple todas las reglas, para romper una sola en cada test. En el pueblo, la casilla
// UNIQUE_TOWN_TILES[i] está en la columna i de la fila 0.
WorldMap complete_map() {
    WorldMap map = WorldMap::create_empty("Valle Juanceto");
    map.farm.set(1, 1, Tile::Cabin);
    map.farm.set(0, 5, Tile::ExitToTown);

    int column = 0;
    for (Tile tile: UNIQUE_TOWN_TILES) {
        map.town.set(column++, 0, tile);
    }
    map.town.set(0, 5, Tile::ExitToFarm);
    return map;
}

Problems describe(const WorldMap& map) {
    Problems descriptions;
    for (const MapProblem& problem: validate(map)) {
        descriptions.push_back(problem.place + ": " + problem.description);
    }
    return descriptions;
}

}

TEST(MapValidationTest, CompleteMapHasNoProblems) {
    EXPECT_EQ(describe(complete_map()), Problems{});
}

TEST(MapValidationTest, EmptyMapListsEverythingMissing) {
    const Problems problems = describe(WorldMap::create_empty("Valle Juanceto"));

    // 2 de la granja y 7 del pueblo.
    EXPECT_EQ(problems.size(), 9u);
    EXPECT_EQ(problems.front(), "Granja: falta «Cabaña»");
    EXPECT_EQ(problems.back(), "Pueblo: falta «Salida a la granja»");
}

TEST(MapValidationTest, NameFollowsTheGameNameRule) {
    for (const char* name:
         {"", "   ", "Valle_Juanceto", "Un nombre demasiado largo para el mundo"}) {
        WorldMap map = complete_map();
        map.name = name;
        const std::vector<MapProblem> problems = validate(map);
        ASSERT_EQ(problems.size(), 1u) << name;
        EXPECT_EQ(problems.front().place, "Nombre del mundo");
    }
}

TEST(MapValidationTest, FarmNeedsOneCabinAndSomeExit) {
    WorldMap map = complete_map();
    map.farm.set(1, 1, Tile::Grass);
    EXPECT_EQ(describe(map), Problems{"Granja: falta «Cabaña»"});

    map.farm.set(1, 1, Tile::Cabin);
    map.farm.set(2, 1, Tile::Cabin);
    EXPECT_EQ(describe(map), Problems{"Granja: sobra «Cabaña»: hay 2 y tiene que haber 1"});

    map = complete_map();
    map.farm.set(0, 5, Tile::Grass);
    EXPECT_EQ(describe(map), Problems{"Granja: falta «Salida al pueblo»"});

    map.farm.set(0, 6, Tile::ExitToTown);
    map.farm.set(0, 7, Tile::ExitToTown);
    EXPECT_EQ(describe(map), Problems{});
}

TEST(MapValidationTest, TownNeedsEachMerchantAndPlaceOnce) {
    int column = 0;
    for (Tile tile: UNIQUE_TOWN_TILES) {
        const std::string name = std::string("«") + tile_name(tile) + "»";
        WorldMap map = complete_map();

        map.town.set(column, 0, Tile::Grass);
        EXPECT_EQ(describe(map), Problems{"Pueblo: falta " + name});

        map.town.set(column, 0, tile);
        map.town.set(column, 1, tile);
        EXPECT_EQ(describe(map), Problems{"Pueblo: sobra " + name + ": hay 2 y tiene que haber 1"});
        ++column;
    }

    WorldMap map = complete_map();
    map.town.set(0, 5, Tile::Grass);
    EXPECT_EQ(describe(map), Problems{"Pueblo: falta «Salida a la granja»"});
}

}
