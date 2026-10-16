#include "game_loop.h"

#include <chrono>
#include <stdexcept>
#include <utility>

#include "gtest/gtest.h"

#include "world.h"

TEST(GameLoopTest, RejectsNonPositiveTickInterval) {
    game::Map map({{game::TileType::Walkable}});
    game::WorldConfig config{1, 1.0, 0.5, 0.5};
    game::World world(std::move(map), {0.5, 0.5}, config);
    Queue<PlayerCommand> commands;

    EXPECT_THROW((GameLoop{world, commands, std::chrono::milliseconds{0}}), std::invalid_argument);

    EXPECT_THROW((GameLoop{world, commands, std::chrono::milliseconds{-1}}), std::invalid_argument);
}
