#include <set>

#include "editor/model/tile.h"
#include "gtest/gtest.h"

namespace editor {

const ZoneKind ALL_ZONES[] = {ZoneKind::Farm, ZoneKind::Town, ZoneKind::Dungeon};

TEST(TileTest, EveryTileHasAUniqueCodeThatReadsBack) {
    std::set<char> codes;
    for (ZoneKind zone: ALL_ZONES) {
        for (Tile tile: tiles_for(zone)) {
            EXPECT_EQ(tile_from_code(tile_code(tile)), tile) << tile_name(tile);
            codes.insert(tile_code(tile));
        }
    }
    EXPECT_EQ(codes.size(), 17u);
}

TEST(TileTest, UnknownCodesAreRejected) {
    for (char code: {'?', ' ', 'z', '\0'}) {
        EXPECT_EQ(tile_from_code(code), std::nullopt) << static_cast<int>(code);
    }
}

TEST(TileTest, EachZoneAllowsOnlyItsOwnTiles) {
    EXPECT_TRUE(is_allowed(ZoneKind::Farm, Tile::Cabin));
    EXPECT_FALSE(is_allowed(ZoneKind::Farm, Tile::Marnie));
    EXPECT_FALSE(is_allowed(ZoneKind::Farm, Tile::Wall));

    for (Tile tile: {Tile::Marnie, Tile::Clint, Tile::Pierre, Tile::Robin, Tile::CommunityCenter,
                     Tile::DungeonEntrance, Tile::ExitToFarm}) {
        EXPECT_TRUE(is_allowed(ZoneKind::Town, tile)) << tile_name(tile);
    }
    EXPECT_FALSE(is_allowed(ZoneKind::Town, Tile::Cabin));

    EXPECT_EQ(tiles_for(ZoneKind::Dungeon).size(), 4u);
    EXPECT_FALSE(is_allowed(ZoneKind::Dungeon, Tile::Grass));
}

TEST(TileTest, DefaultTileIsAllowedInItsZone) {
    for (ZoneKind zone: ALL_ZONES) {
        EXPECT_TRUE(is_allowed(zone, default_tile(zone)));
    }
}

}
