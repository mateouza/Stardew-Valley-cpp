#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

#include <toml++/toml.hpp>

#include "editor/model/map_file.h"
#include "gtest/gtest.h"

namespace editor {

class MapFileTest: public ::testing::Test {
protected:
    std::filesystem::path directory;

    void SetUp() override {
        std::string pattern =
                (std::filesystem::temp_directory_path() / "stardew-editor-XXXXXX").string();
        ASSERT_NE(mkdtemp(pattern.data()), nullptr);
        directory = pattern;
    }

    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(directory, error);
    }

    std::string path_of(const std::string& name) const { return (directory / name).string(); }

    std::string write_file(const std::string& text) const {
        const std::string path = path_of("mapa.toml");
        std::ofstream(path) << text;
        return path;
    }

    void expect_error(const std::string& text, const std::string& expected) const {
        try {
            load_map(write_file(text));
            ADD_FAILURE() << "Se esperaba un error que dijera: " << expected;
        } catch (const MapFileError& error) {
            EXPECT_NE(std::string(error.what()).find(expected), std::string::npos) << error.what();
        }
    }
};

namespace {

const std::string HEADER = "version = 1\nname = 'Valle Juanceto'\n";

std::string farm(const std::string& body) { return "[farm]\n" + body; }

const std::string FARM = farm("width = 3\nheight = 2\nrows = ['gH.', 'g>g']\n");

// Mapa chico escrito a mano. Cada test cambia una parte: el encabezado, la granja o los niveles.
std::string map_text(const std::string& header = HEADER, const std::string& farm_table = FARM,
                     int levels = WorldMap::DUNGEON_LEVELS) {
    std::string text = header + farm_table + "[town]\nwidth = 2\nheight = 1\nrows = ['MU']\n";
    for (int level = 0; level < levels; ++level) {
        text += "[[dungeon]]\nwidth = 1\nheight = 1\nrows = ['X']\n";
    }
    return text;
}

}

TEST_F(MapFileTest, SavesEveryZoneRowByRow) {
    WorldMap map = WorldMap::create_empty("Valle Juanceto");
    map.farm.set(2, 1, Tile::Cabin);
    map.dungeon[9].set(0, 19, Tile::DungeonExit);

    save_map(map, path_of("valle.toml"));

    const toml::table document = toml::parse_file(path_of("valle.toml"));
    EXPECT_EQ(document["version"].value<int>(), MAP_FILE_VERSION);
    EXPECT_EQ(document["name"].value<std::string>(), "Valle Juanceto");
    EXPECT_EQ(document["farm"]["width"].value<int>(), WorldMap::SURFACE_WIDTH);
    EXPECT_EQ(document["farm"]["height"].value<int>(), WorldMap::SURFACE_HEIGHT);
    EXPECT_EQ(document["farm"]["rows"][1].value<std::string>(), "ggH" + std::string(37, 'g'));
    EXPECT_EQ(document["town"]["rows"][29].value<std::string>(), std::string(40, 'g'));
    ASSERT_EQ(document["dungeon"].as_array()->size(), 10u);
    EXPECT_EQ(document["dungeon"][9]["rows"][19].value<std::string>(), "X" + std::string(29, '#'));
}

TEST_F(MapFileTest, SavingAgainReplacesThePreviousFile) {
    save_map(WorldMap::create_empty("Valle Juanceto"), path_of("valle.toml"));
    save_map(WorldMap::create_empty("Monte Juanceto"), path_of("valle.toml"));

    const toml::table document = toml::parse_file(path_of("valle.toml"));
    EXPECT_EQ(document["name"].value<std::string>(), "Monte Juanceto");
    EXPECT_EQ(std::distance(std::filesystem::directory_iterator(directory),
                            std::filesystem::directory_iterator()),
              1);
}

TEST_F(MapFileTest, SavingWhereItCannotWriteThrowsAndLeavesNothing) {
    EXPECT_THROW(save_map(WorldMap::create_empty("Valle Juanceto"), path_of("falta/valle.toml")),
                 MapFileError);
    EXPECT_TRUE(std::filesystem::is_empty(directory));
}

TEST_F(MapFileTest, SavedMapLoadsBackTheSame) {
    WorldMap map = WorldMap::create_empty("Valle de Ñandú");
    map.farm = Zone(ZoneKind::Farm, 3, 2);
    map.farm.set(1, 0, Tile::Cabin);
    map.town.set(39, 29, Tile::Robin);
    map.dungeon[4].set(10, 10, Tile::StairsDown);

    save_map(map, path_of("valle.toml"));

    EXPECT_TRUE(load_map(path_of("valle.toml")) == map);
}

TEST_F(MapFileTest, LoadsAHandWrittenMap) {
    const WorldMap map = load_map(write_file(map_text()));

    EXPECT_EQ(map.name, "Valle Juanceto");
    EXPECT_EQ(map.farm.width(), 3);
    EXPECT_EQ(map.farm.at(1, 0), Tile::Cabin);
    EXPECT_EQ(map.farm.at(1, 1), Tile::ExitToTown);
    EXPECT_EQ(map.town.at(0, 0), Tile::Marnie);
    EXPECT_EQ(map.dungeon[9].at(0, 0), Tile::DungeonExit);
}

TEST_F(MapFileTest, RejectsFilesThatDoNotFollowTheFormat) {
    EXPECT_THROW(load_map(path_of("no-existe.toml")), MapFileError);
    expect_error("[farm", "TOML");
    expect_error(map_text("version = 2\nname = 'Valle Juanceto'\n"), "versión");
    expect_error(map_text("version = 1\n"), "nombre");
    expect_error(map_text(HEADER, FARM, 9), "10 niveles");
    expect_error(map_text(HEADER, FARM, 11), "10 niveles");

    expect_error(map_text(HEADER, ""), "Granja: falta la tabla");
    expect_error(map_text(HEADER, farm("height = 2\nrows = ['gH.', 'g>g']\n")), "Granja: width");
    expect_error(map_text(HEADER, farm("width = 201\nheight = 1\nrows = ['g']\n")),
                 "Granja: width");
    expect_error(map_text(HEADER, farm("width = 3\nheight = 0\nrows = []\n")), "Granja: height");
    expect_error(map_text(HEADER, farm("width = 3\nheight = 3\nrows = ['gH.', 'g>g']\n")),
                 "Granja: rows");
    expect_error(map_text(HEADER, farm("width = 3\nheight = 2\nrows = ['gH', 'g>g']\n")),
                 "Granja, fila 1:");
    expect_error(map_text(HEADER, farm("width = 3\nheight = 2\nrows = ['gH.', 1]\n")),
                 "Granja, fila 2:");
    expect_error(map_text(HEADER, farm("width = 3\nheight = 2\nrows = ['gH?', 'g>g']\n")),
                 "Granja, fila 1, columna 3");
    expect_error(map_text(HEADER, farm("width = 3\nheight = 2\nrows = ['gH.', 'gM.']\n")),
                 "Granja, fila 2, columna 2");
}

}
