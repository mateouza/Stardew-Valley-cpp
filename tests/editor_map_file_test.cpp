#include <cstdlib>
#include <filesystem>
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
};

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

}
