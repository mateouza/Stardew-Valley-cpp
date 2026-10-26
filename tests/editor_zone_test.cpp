#include <stdexcept>
#include <utility>

#include "editor/model/zone.h"
#include "gtest/gtest.h"

namespace editor {

TEST(ZoneTest, NewZoneIsFilledWithItsDefaultTile) {
    const Zone farm(ZoneKind::Farm, 4, 3);
    EXPECT_EQ(farm.kind(), ZoneKind::Farm);
    EXPECT_EQ(farm.width(), 4);
    EXPECT_EQ(farm.height(), 3);
    EXPECT_EQ(farm.count(Tile::Grass), 12);

    const Zone level(ZoneKind::Dungeon, 2, 2);
    EXPECT_EQ(level.count(Tile::Wall), 4);
}

TEST(ZoneTest, SetChangesOnlyThatTile) {
    Zone farm(ZoneKind::Farm, 4, 3);
    const Zone untouched = farm;

    farm.set(3, 1, Tile::Cabin);

    EXPECT_EQ(farm.at(3, 1), Tile::Cabin);
    EXPECT_EQ(farm.at(1, 2), Tile::Grass);
    EXPECT_EQ(farm.count(Tile::Cabin), 1);
    EXPECT_EQ(farm.count(Tile::Grass), 11);
    EXPECT_FALSE(farm == untouched);
}

TEST(ZoneTest, PositionsOutsideTheGridThrow) {
    Zone town(ZoneKind::Town, 4, 3);
    EXPECT_TRUE(town.contains(0, 0));
    EXPECT_TRUE(town.contains(3, 2));

    const std::pair<int, int> outside[] = {{-1, 0}, {4, 0}, {0, -1}, {0, 3}};
    for (const auto& [column, row]: outside) {
        EXPECT_FALSE(town.contains(column, row));
        EXPECT_THROW(town.at(column, row), std::out_of_range);
        EXPECT_THROW(town.set(column, row, Tile::Path), std::out_of_range);
    }
    EXPECT_EQ(town.count(Tile::Grass), 12);
}

TEST(ZoneTest, RejectsTilesFromOtherZones) {
    Zone farm(ZoneKind::Farm, 4, 3);
    EXPECT_THROW(farm.set(0, 0, Tile::Marnie), std::invalid_argument);
    EXPECT_THROW(farm.set(0, 0, Tile::Wall), std::invalid_argument);
    EXPECT_EQ(farm.at(0, 0), Tile::Grass);

    Zone level(ZoneKind::Dungeon, 4, 3);
    EXPECT_THROW(level.set(0, 0, Tile::Grass), std::invalid_argument);
    level.set(0, 0, Tile::StairsDown);
    EXPECT_EQ(level.at(0, 0), Tile::StairsDown);
}

TEST(ZoneTest, RejectsSizesOutOfRange) {
    const auto create = [](int width, int height) { return Zone(ZoneKind::Farm, width, height); };

    EXPECT_THROW(create(0, 3), std::invalid_argument);
    EXPECT_THROW(create(3, -1), std::invalid_argument);
    EXPECT_THROW(create(Zone::MAX_SIDE + 1, 3), std::invalid_argument);
    EXPECT_THROW(create(3, Zone::MAX_SIDE + 1), std::invalid_argument);
    EXPECT_EQ(create(Zone::MAX_SIDE, Zone::MAX_SIDE).count(Tile::Grass),
              Zone::MAX_SIDE * Zone::MAX_SIDE);
}

}
