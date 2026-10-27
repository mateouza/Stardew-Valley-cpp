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

    for (Zone& level: map.dungeon) {
        level.set(1, 1, Tile::DungeonExit);
        level.set(2, 2, Tile::StairsDown);
    }
    map.dungeon.back().set(2, 2, Tile::Wall);
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

    // 2 de la granja, 7 del pueblo, y 10 salidas y 9 escaleras de la mazmorra.
    EXPECT_EQ(problems.size(), 28u);
    EXPECT_EQ(problems.front(), "Granja: falta «Cabaña»");
    EXPECT_EQ(problems.back(), "Mazmorra, nivel 10: falta «Salida de la mazmorra»");
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

TEST(MapValidationTest, EachLevelNeedsAnExitAndStairsExceptTheLast) {
    WorldMap map = complete_map();
    map.dungeon[0].set(2, 2, Tile::Floor);
    map.dungeon[2].set(1, 1, Tile::Floor);
    map.dungeon[4].set(3, 3, Tile::StairsDown);
    map.dungeon[9].set(2, 2, Tile::StairsDown);

    EXPECT_EQ(describe(map),
              (Problems{"Mazmorra, nivel 1: falta «Escalera al siguiente nivel»",
                        "Mazmorra, nivel 3: falta «Salida de la mazmorra»",
                        "Mazmorra, nivel 5: sobra «Escalera al siguiente nivel»: hay 2 y tiene que "
                        "haber 1",
                        "Mazmorra, nivel 10: sobra «Escalera al siguiente nivel»: es el último "
                        "nivel"}));
}

}
