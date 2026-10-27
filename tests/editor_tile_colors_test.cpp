#include <QColor>

#include "editor/qt/tile_colors.h"
#include "gtest/gtest.h"

namespace editor {

TEST(TileColorsTest, EveryTileHasAColor) {
    for (ZoneKind kind: {ZoneKind::Farm, ZoneKind::Town, ZoneKind::Dungeon}) {
        for (Tile tile: tiles_for(kind)) {
            EXPECT_NE(tile_color(tile), QColor(Qt::magenta)) << tile_name(tile);
        }
    }
}

TEST(TileColorsTest, OnlyTilesThatAreNotTerrainShowTheirLetter) {
    for (Tile tile:
         {Tile::Grass, Tile::Path, Tile::Water, Tile::Obstacle, Tile::Floor, Tile::Wall}) {
        EXPECT_FALSE(shows_letter(tile)) << tile_name(tile);
    }
    for (Tile tile: {Tile::Cabin, Tile::Marnie, Tile::ExitToFarm, Tile::StairsDown}) {
        EXPECT_TRUE(shows_letter(tile)) << tile_name(tile);
    }
}

TEST(TileColorsTest, TilesThatShareAColorShowTheirLetter) {
    for (Tile tile: {Tile::Marnie, Tile::Clint, Tile::Pierre, Tile::Robin}) {
        EXPECT_EQ(tile_color(tile), tile_color(Tile::Marnie));
        EXPECT_TRUE(shows_letter(tile)) << tile_name(tile);
    }
    for (Tile tile: {Tile::ExitToTown, Tile::ExitToFarm, Tile::DungeonExit}) {
        EXPECT_EQ(tile_color(tile), tile_color(Tile::ExitToTown));
        EXPECT_TRUE(shows_letter(tile)) << tile_name(tile);
    }
}

}
