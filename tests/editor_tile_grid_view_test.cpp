#include <QColor>
#include <QImage>
#include <QPoint>
#include <QSize>
#include <QTest>

#include "editor/qt/tile_colors.h"
#include "editor/qt/tile_grid_view.h"
#include "gtest/gtest.h"

namespace editor {

namespace {

constexpr int TILE_SIZE = TileGridView::TILE_SIZE;

// Un punto dentro de la casilla, lejos del borde y de la letra.
QPoint inside_cell(int column, int row) { return QPoint(column, row) * TILE_SIZE + QPoint(3, 3); }

}

class TileGridViewTest: public ::testing::Test {
protected:
    Zone zone{ZoneKind::Farm, 4, 3};
    TileGridView view;

    void SetUp() override {
        view.set_zone(&zone);
        view.show();
        ASSERT_TRUE(QTest::qWaitForWindowExposed(&view));
    }
};

TEST_F(TileGridViewTest, SizeFollowsTheZone) {
    EXPECT_EQ(view.size(), QSize(4 * TILE_SIZE, 3 * TILE_SIZE));

    Zone level(ZoneKind::Dungeon, 6, 2);
    view.set_zone(&level);
    EXPECT_EQ(view.size(), QSize(6 * TILE_SIZE, 2 * TILE_SIZE));

    view.set_zone(nullptr);
    EXPECT_EQ(view.size(), QSize(0, 0));
}

TEST_F(TileGridViewTest, DrawsEachTileWithItsColor) {
    zone.set(1, 0, Tile::Water);
    zone.set(3, 2, Tile::Cabin);
    view.update();

    const QImage image = view.grab().toImage();
    EXPECT_EQ(image.pixelColor(inside_cell(0, 0)), tile_color(Tile::Grass));
    EXPECT_EQ(image.pixelColor(inside_cell(1, 0)), tile_color(Tile::Water));
    EXPECT_EQ(image.pixelColor(inside_cell(3, 2)), tile_color(Tile::Cabin));
}

}
