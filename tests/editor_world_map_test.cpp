#include "editor/model/world_map.h"
#include "gtest/gtest.h"

namespace editor {

TEST(WorldMapTest, EmptyWorldHasEveryZoneFilledWithItsDefaultTile) {
    const WorldMap map = WorldMap::create_empty("Valle Juanceto");

    EXPECT_EQ(map.name, "Valle Juanceto");

    EXPECT_EQ(map.farm.kind(), ZoneKind::Farm);
    EXPECT_EQ(map.farm.width(), WorldMap::SURFACE_WIDTH);
    EXPECT_EQ(map.farm.height(), WorldMap::SURFACE_HEIGHT);
    EXPECT_EQ(map.farm.count(Tile::Grass), WorldMap::SURFACE_WIDTH * WorldMap::SURFACE_HEIGHT);

    EXPECT_EQ(map.town.kind(), ZoneKind::Town);
    EXPECT_EQ(map.town.count(Tile::Grass), WorldMap::SURFACE_WIDTH * WorldMap::SURFACE_HEIGHT);

    for (const Zone& level: map.dungeon) {
        EXPECT_EQ(level.kind(), ZoneKind::Dungeon);
        EXPECT_EQ(level.width(), WorldMap::LEVEL_WIDTH);
        EXPECT_EQ(level.height(), WorldMap::LEVEL_HEIGHT);
        EXPECT_EQ(level.count(Tile::Wall), WorldMap::LEVEL_WIDTH * WorldMap::LEVEL_HEIGHT);
    }
}

TEST(WorldMapTest, DungeonLevelsAreIndependent) {
    WorldMap map = WorldMap::create_empty("Valle Juanceto");

    map.dungeon[0].set(0, 0, Tile::StairsDown);

    EXPECT_EQ(map.dungeon[0].count(Tile::StairsDown), 1);
    for (int level = 1; level < WorldMap::DUNGEON_LEVELS; ++level) {
        EXPECT_EQ(map.dungeon[level].count(Tile::StairsDown), 0) << "nivel " << level + 1;
    }
}

TEST(WorldMapTest, WorldsCompareByNameAndTiles) {
    const WorldMap original = WorldMap::create_empty("Valle Juanceto");
    EXPECT_TRUE(original == WorldMap::create_empty("Valle Juanceto"));
    EXPECT_FALSE(original == WorldMap::create_empty("Monte Juanceto"));

    WorldMap edited = original;
    edited.dungeon[WorldMap::DUNGEON_LEVELS - 1].set(5, 5, Tile::DungeonExit);
    EXPECT_FALSE(original == edited);
}

}
