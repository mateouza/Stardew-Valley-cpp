#include "world_config.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <variant>

#include "gtest/gtest.h"

#include "world.h"

namespace game {

class WorldConfigTest: public ::testing::Test {
protected:
    std::filesystem::path directory;

    void SetUp() override {
        auto pattern = (std::filesystem::temp_directory_path() / "stardew-config-XXXXXX").string();
        const auto* created = mkdtemp(pattern.data());
        if (created == nullptr) {
            throw std::runtime_error("No se pudo crear el directorio temporal de la prueba");
        }
        directory = created;
    }

    void TearDown() override {
        if (!directory.empty()) {
            std::error_code error;
            std::filesystem::remove_all(directory, error);
            EXPECT_FALSE(error) << error.message();
        }
    }

    std::string write_config(const std::string& contents) {
        const auto path = directory / "world.toml";
        std::ofstream file(path);
        file.exceptions(std::ios::failbit | std::ios::badbit);
        file << contents;
        file.close();
        return path.string();
    }

    static std::string config_text(const std::string& capacity = "2",
                                   const std::string& speed = "1.5",
                                   const std::string& width = "0.5",
                                   const std::string& height = "0.75") {
        return "[world]\nmax_players = " + capacity + "\nplayer_speed = " + speed +
               "\nplayer_width = " + width + "\nplayer_height = " + height + "\n";
    }

    void expect_error(const std::string& contents, const std::string& field) {
        const auto path = write_config(contents);
        try {
            load_world_config(path);
            FAIL() << "Se esperaba un error de configuracion";
        } catch (const std::runtime_error& error) {
            const std::string message = error.what();
            EXPECT_NE(message.find(path), std::string::npos);
            EXPECT_NE(message.find(field), std::string::npos);
        }
    }

    static Map make_map() {
        return Map{
                {{TileType::Walkable, TileType::Walkable, TileType::Walkable, TileType::Walkable}}};
    }
};

TEST_F(WorldConfigTest, ReadsEveryRequiredField) {
    const auto config = load_world_config(write_config(config_text()));

    EXPECT_EQ(config.max_players, 2u);
    EXPECT_DOUBLE_EQ(config.player_speed, 1.5);
    EXPECT_DOUBLE_EQ(config.player_width, 0.5);
    EXPECT_DOUBLE_EQ(config.player_height, 0.75);
}

TEST_F(WorldConfigTest, AcceptsIntegerValuesForRealFields) {
    const auto config = load_world_config(write_config(config_text("2", "3", "1", "1")));

    EXPECT_DOUBLE_EQ(config.player_speed, 3.0);
    EXPECT_DOUBLE_EQ(config.player_width, 1.0);
    EXPECT_DOUBLE_EQ(config.player_height, 1.0);
}

TEST_F(WorldConfigTest, ReportsMissingFileWithItsPath) {
    const auto path = (directory / "missing.toml").string();
    try {
        load_world_config(path);
        FAIL() << "Se esperaba un error de lectura";
    } catch (const std::runtime_error& error) {
        EXPECT_NE(std::string(error.what()).find(path), std::string::npos);
    }
}

TEST_F(WorldConfigTest, RejectsInvalidToml) { expect_error("[world\n", "world.toml"); }

TEST_F(WorldConfigTest, RequiresWorldTable) {
    expect_error("", "[world]");
    expect_error("world = 1\n", "[world]");
    expect_error("[other]\nmax_players = 2\n", "[world]");
}

TEST_F(WorldConfigTest, RequiresEveryFieldWithoutUsingDefaults) {
    const std::string fields[] = {"max_players", "player_speed", "player_width", "player_height"};
    for (const auto& field: fields) {
        SCOPED_TRACE(field);
        auto contents = config_text();
        const auto start = contents.find(field);
        const auto end = contents.find('\n', start);
        contents.erase(start, end - start + 1);
        expect_error(contents, "world." + field);
    }
}

TEST_F(WorldConfigTest, RejectsNonIntegerCapacityWithoutTruncating) {
    const std::string values[] = {"2.5", "2.0", "true", "\"2\"", "[2]", "{ value = 2 }"};
    for (const auto& value: values) {
        SCOPED_TRACE(value);
        expect_error(config_text(value), "world.max_players");
    }
}

TEST_F(WorldConfigTest, RejectsNegativeCapacityBeforeUnsignedConversion) {
    expect_error(config_text("-1"), "world.max_players");
}

TEST_F(WorldConfigTest, RejectsNonNumericRealFields) {
    const std::string values[] = {"true", "\"1.5\"", "[1.5]", "{ value = 1.5 }"};
    for (const auto& value: values) {
        SCOPED_TRACE(value);
        expect_error(config_text("2", value), "world.player_speed");
        expect_error(config_text("2", "1.5", value), "world.player_width");
        expect_error(config_text("2", "1.5", "0.5", value), "world.player_height");
    }
}

TEST_F(WorldConfigTest, RejectsIntegersNotRepresentableAsRealValues) {
    expect_error(config_text("2", "9223372036854775807"), "world.player_speed");
}

TEST_F(WorldConfigTest, KeepsCapacityValidationInWorld) {
    const auto config = load_world_config(write_config(config_text("0")));

    EXPECT_THROW((World{make_map(), {0.5, 0.5}, config}), std::invalid_argument);
}

TEST_F(WorldConfigTest, KeepsRealValueValidationInWorld) {
    const std::string values[] = {"0", "-1", "inf", "nan"};
    for (const auto& value: values) {
        SCOPED_TRACE(value);
        auto config = load_world_config(write_config(config_text("2", value)));
        EXPECT_THROW((World{make_map(), {0.5, 0.5}, config}), std::invalid_argument);
        config = load_world_config(write_config(config_text("2", "1.5", value)));
        EXPECT_THROW((World{make_map(), {0.5, 0.5}, config}), std::invalid_argument);
        config = load_world_config(write_config(config_text("2", "1.5", "0.5", value)));
        EXPECT_THROW((World{make_map(), {0.5, 0.5}, config}), std::invalid_argument);
    }
}

TEST_F(WorldConfigTest, LoadedCapacityControlsAdmission) {
    const auto config = load_world_config(write_config(config_text("1")));
    World world{make_map(), {0.5, 0.5}, config};
    ASSERT_TRUE(std::holds_alternative<PlayerId>(world.add_player("Jose")));

    const auto result = world.add_player("Mateo");

    ASSERT_TRUE(std::holds_alternative<AddPlayerError>(result));
    EXPECT_EQ(std::get<AddPlayerError>(result), AddPlayerError::GameFull);
}

TEST_F(WorldConfigTest, LoadedSpeedControlsDisplacement) {
    const auto config = load_world_config(write_config(config_text("2", "2.5")));
    World world{make_map(), {0.5, 0.5}, config};
    const auto id = std::get<PlayerId>(world.add_player("Jose"));
    ASSERT_EQ(world.set_direction(id, {1, 0}), SetDirectionResult::Accepted);

    world.update_world(std::chrono::duration<double>{0.4});

    EXPECT_DOUBLE_EQ(world.get_state().players.at(0).position.x, 1.5);
}

TEST_F(WorldConfigTest, LoadedDimensionsControlWallContact) {
    const auto config = load_world_config(write_config(config_text("2", "1.0", "0.6", "0.6")));
    World horizontal{Map{{{TileType::Walkable, TileType::Blocked}}}, {0.5, 0.5}, config};
    World vertical{Map{{{TileType::Walkable}, {TileType::Blocked}}}, {0.5, 0.5}, config};
    const auto first = std::get<PlayerId>(horizontal.add_player("Jose"));
    const auto second = std::get<PlayerId>(vertical.add_player("Mateo"));
    ASSERT_EQ(horizontal.set_direction(first, {1, 0}), SetDirectionResult::Accepted);
    ASSERT_EQ(vertical.set_direction(second, {0, 1}), SetDirectionResult::Accepted);

    horizontal.update_world(std::chrono::duration<double>{1.0});
    vertical.update_world(std::chrono::duration<double>{1.0});

    EXPECT_DOUBLE_EQ(horizontal.get_state().players.at(0).position.x, 0.7);
    EXPECT_DOUBLE_EQ(vertical.get_state().players.at(0).position.y, 0.7);
}

TEST(WorldConfigExampleTest, RepositoryExampleCanCreateWorld) {
    const auto config = load_world_config(TEST_WORLD_CONFIG_PATH);
    World world{Map{{{TileType::Walkable}}}, {0.5, 0.5}, config};

    EXPECT_TRUE(std::holds_alternative<PlayerId>(world.add_player("Jose")));
}

}
