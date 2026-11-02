#include "editor/qt/editor_window.h"

#include <QComboBox>
#include <QFile>
#include <QListWidget>
#include <QPoint>
#include <QSize>
#include <QTemporaryDir>
#include <QTest>
#include <optional>
#include <string>

#include "editor/qt/tile_grid_view.h"
#include "gtest/gtest.h"

namespace editor {

namespace {

constexpr int TILE_SIZE = TileGridView::TILE_SIZE;

QPoint center_of(int column, int row) {
    return QPoint(column * TILE_SIZE + TILE_SIZE / 2, row * TILE_SIZE + TILE_SIZE / 2);
}

// Para que gtest muestre el mensaje si una operación falla cuando no debería.
std::string error_of(const std::optional<QString>& error) {
    return error ? error->toStdString() : "";
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

TEST_F(EditorWindowTest, SavedMapOpensAgainTheSame) {
    ASSERT_EQ(error_of(window.open_map(EXAMPLE_MAP_PATH)), "");
    choose_brush(Tile::Water);
    QTest::mouseClick(grid, Qt::LeftButton, Qt::NoModifier, center_of(1, 1));
    QTemporaryDir directory;
    const QString path = directory.filePath("mapa.toml");

    ASSERT_EQ(error_of(window.save_map_to(path)), "");
    EXPECT_FALSE(window.is_modified());

    EditorWindow reopened;
    ASSERT_EQ(error_of(reopened.open_map(path)), "");
    EXPECT_EQ(reopened.get_map().farm.at(1, 1), Tile::Water);
    EXPECT_TRUE(reopened.get_map() == window.get_map());
}

TEST_F(EditorWindowTest, MapWithProblemsIsNotSaved) {
    QTemporaryDir directory;
    const QString path = directory.filePath("mapa.toml");

    const auto error = window.save_map_to(path);

    ASSERT_TRUE(error.has_value());
    EXPECT_TRUE(error->contains("Granja: falta")) << error->toStdString();
    EXPECT_FALSE(QFile::exists(path));
}

TEST_F(EditorWindowTest, FileThatCannotBeOpenedKeepsTheCurrentMap) {
    choose_brush(Tile::Cabin);
    QTest::mouseClick(grid, Qt::LeftButton, Qt::NoModifier, center_of(0, 0));
    QTemporaryDir directory;
    QFile broken(directory.filePath("roto.toml"));
    ASSERT_TRUE(broken.open(QIODevice::WriteOnly));
    broken.write("esto no es [un mapa");
    broken.close();

    EXPECT_TRUE(window.open_map(broken.fileName()).has_value());
    EXPECT_EQ(window.get_map().farm.at(0, 0), Tile::Cabin);
    EXPECT_TRUE(window.is_modified());
}

TEST_F(EditorWindowTest, OpeningOrStartingAMapShowsItsFarmWithoutChanges) {
    zone_selector->setCurrentText("Pueblo");
    ASSERT_EQ(error_of(window.open_map(EXAMPLE_MAP_PATH)), "");
    EXPECT_EQ(zone_selector->currentText(), "Granja");
    EXPECT_EQ(palette->count(), static_cast<int>(tiles_for(ZoneKind::Farm).size()));

    QTest::mouseClick(grid, Qt::LeftButton, Qt::NoModifier, center_of(1, 1));
    window.new_map();
    EXPECT_TRUE(window.get_map() == WorldMap::create_empty("Mundo nuevo"));
    EXPECT_FALSE(window.is_modified());
}

}
