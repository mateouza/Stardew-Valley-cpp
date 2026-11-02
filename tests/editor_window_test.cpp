#include "editor/qt/editor_window.h"

#include <QComboBox>
#include <QListWidget>
#include <QPoint>
#include <QSize>
#include <QTest>

#include "editor/qt/tile_grid_view.h"
#include "gtest/gtest.h"

namespace editor {

namespace {

constexpr int TILE_SIZE = TileGridView::TILE_SIZE;

QPoint center_of(int column, int row) {
    return QPoint(column * TILE_SIZE + TILE_SIZE / 2, row * TILE_SIZE + TILE_SIZE / 2);
}

}

class EditorWindowTest: public ::testing::Test {
protected:
    EditorWindow window;
    QComboBox* zone_selector = window.findChild<QComboBox*>("zone_selector");
    QListWidget* palette = window.findChild<QListWidget*>("palette");
    TileGridView* grid = window.findChild<TileGridView*>("tile_grid");

    void SetUp() override {
        window.show();
        ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));
    }

    void choose_brush(Tile tile) {
        const auto found = palette->findItems(tile_name(tile), Qt::MatchExactly);
        ASSERT_EQ(found.size(), 1) << tile_name(tile);
        palette->setCurrentItem(found.front());
    }
};

TEST_F(EditorWindowTest, StartsOnTheFarmWithItsPalette) {
    EXPECT_EQ(zone_selector->currentText(), "Granja");
    EXPECT_EQ(zone_selector->count(), 2 + WorldMap::DUNGEON_LEVELS);
    EXPECT_EQ(palette->count(), static_cast<int>(tiles_for(ZoneKind::Farm).size()));
    EXPECT_EQ(grid->get_brush(), tiles_for(ZoneKind::Farm).front());
    EXPECT_EQ(grid->size(),
              QSize(WorldMap::SURFACE_WIDTH * TILE_SIZE, WorldMap::SURFACE_HEIGHT * TILE_SIZE));
    EXPECT_FALSE(window.is_modified());
}

TEST_F(EditorWindowTest, PaintsTheTileChosenInThePalette) {
    choose_brush(Tile::Cabin);

    QTest::mouseClick(grid, Qt::LeftButton, Qt::NoModifier, center_of(4, 2));

    EXPECT_EQ(window.get_map().farm.at(4, 2), Tile::Cabin);
    EXPECT_EQ(window.get_map().farm.count(Tile::Cabin), 1);
    EXPECT_TRUE(window.is_modified());
}

TEST_F(EditorWindowTest, ChangingTheZoneChangesThePaletteAndTheGrid) {
    zone_selector->setCurrentText("Pueblo");
    EXPECT_EQ(palette->findItems("Marnie", Qt::MatchExactly).size(), 1);

    zone_selector->setCurrentText("Mazmorra, nivel 10");
    EXPECT_EQ(palette->count(), static_cast<int>(tiles_for(ZoneKind::Dungeon).size()));
    EXPECT_EQ(grid->size(),
              QSize(WorldMap::LEVEL_WIDTH * TILE_SIZE, WorldMap::LEVEL_HEIGHT * TILE_SIZE));
}

TEST_F(EditorWindowTest, PaintsTheLevelThatIsShown) {
    zone_selector->setCurrentText("Mazmorra, nivel 3");
    choose_brush(Tile::StairsDown);

    QTest::mouseClick(grid, Qt::LeftButton, Qt::NoModifier, center_of(4, 2));

    EXPECT_EQ(window.get_map().dungeon[2].at(4, 2), Tile::StairsDown);
    EXPECT_EQ(window.get_map().dungeon[0].count(Tile::StairsDown), 0);
}

TEST_F(EditorWindowTest, KeepsTheBrushOnlyIfTheNewZoneAllowsIt) {
    choose_brush(Tile::Water);
    zone_selector->setCurrentText("Pueblo");
    EXPECT_EQ(grid->get_brush(), Tile::Water);
    EXPECT_EQ(palette->currentItem()->text(), tile_name(Tile::Water));

    zone_selector->setCurrentText("Mazmorra, nivel 1");
    EXPECT_EQ(grid->get_brush(), tiles_for(ZoneKind::Dungeon).front());
}

}
