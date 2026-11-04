#include <QColor>
#include <QImage>
#include <QPoint>
#include <QSize>
#include <QTest>
#include <utility>
#include <vector>

#include "editor/qt/tile_colors.h"
#include "editor/qt/tile_grid_view.h"
#include "gtest/gtest.h"

namespace editor {

namespace {

using Cells = std::vector<std::pair<int, int>>;

constexpr int TILE_SIZE = TileGridView::TILE_SIZE;

QPoint inside_cell(int column, int row) { return QPoint(column, row) * TILE_SIZE + QPoint(3, 3); }

QPoint center_of(int column, int row) {
    return QPoint(column * TILE_SIZE + TILE_SIZE / 2, row * TILE_SIZE + TILE_SIZE / 2);
}

}

class TileGridViewTest: public ::testing::Test {
protected:
    Zone zone{ZoneKind::Farm, 4, 3};
    TileGridView view;
    Cells painted;

    void SetUp() override {
        view.set_zone(&zone);
        QObject::connect(&view, &TileGridView::tile_painted,
                         [this](int column, int row) { painted.emplace_back(column, row); });
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

TEST_F(TileGridViewTest, ClickPaintsTheBrushOnce) {
    view.set_brush(Tile::Cabin);

    QTest::mouseClick(&view, Qt::LeftButton, Qt::NoModifier, center_of(2, 1));
    QTest::mouseClick(&view, Qt::LeftButton, Qt::NoModifier, center_of(2, 1));

    EXPECT_EQ(zone.at(2, 1), Tile::Cabin);
    EXPECT_EQ(painted, (Cells{{2, 1}}));
    EXPECT_EQ(view.grab().toImage().pixelColor(inside_cell(2, 1)), tile_color(Tile::Cabin));
}

TEST_F(TileGridViewTest, DraggingPaintsEveryTileOnTheWay) {
    view.set_brush(Tile::Path);

    QTest::mousePress(&view, Qt::LeftButton, Qt::NoModifier, center_of(0, 2));
    QTest::mouseMove(&view, center_of(1, 2));
    QTest::mouseMove(&view, center_of(2, 2));
    QTest::mouseRelease(&view, Qt::LeftButton, Qt::NoModifier, center_of(2, 2));
    QTest::mouseMove(&view, center_of(3, 2));

    EXPECT_EQ(painted, (Cells{{0, 2}, {1, 2}, {2, 2}}));
    EXPECT_EQ(zone.at(3, 2), Tile::Grass);
}

TEST_F(TileGridViewTest, BrushFromAnotherZoneDoesNotPaint) {
    view.set_brush(Tile::Marnie);

    QTest::mouseClick(&view, Qt::LeftButton, Qt::NoModifier, center_of(0, 0));
    QTest::mouseClick(&view, Qt::RightButton, Qt::NoModifier, center_of(1, 0));

    EXPECT_EQ(zone.at(0, 0), Tile::Grass);
    EXPECT_TRUE(painted.empty());
}

}
