#include "demo_map.h"

#include <chrono>
#include <utility>
#include <variant>

#include "gtest/gtest.h"

#include "world.h"

namespace game {

TEST(DemoMapTest, HorizontalMovementStopsAtVisibleWall) {
    const auto demo = make_demo_map();
    const auto data = demo.map.get_data();

    ASSERT_EQ(data.width, 10u);
    ASSERT_EQ(data.height, 10u);
    ASSERT_EQ(data.tiles.size(), 100u);
    EXPECT_EQ(data.tiles.at(2 * data.width + 5), TileType::Blocked);

    const auto position = demo.map.resolve_movement(demo.spawn_position, {8.5, 2.5}, 0.6, 0.6);

    EXPECT_DOUBLE_EQ(position.x, 4.7);
    EXPECT_DOUBLE_EQ(position.y, 2.5);
    EXPECT_TRUE(demo.map.can_occupy(position, 0.6, 0.6));
}

TEST(DemoMapTest, VerticalMovementStopsAtVisibleWall) {
    const auto demo = make_demo_map();
    const auto data = demo.map.get_data();

    EXPECT_EQ(data.tiles.at(7 * data.width + 6), TileType::Blocked);

    const auto position = demo.map.resolve_movement({6.5, 4.5}, {6.5, 9.5}, 0.6, 0.6);

    EXPECT_DOUBLE_EQ(position.x, 6.5);
    EXPECT_DOUBLE_EQ(position.y, 6.7);
    EXPECT_TRUE(demo.map.can_occupy(position, 0.6, 0.6));
}

TEST(DemoMapTest, SupportsTwoPlayersMovingIndependently) {
    auto demo = make_demo_map();
    World world(std::move(demo.map), demo.spawn_position, {2, 2.0, 0.6, 0.6});
    const auto first_id = std::get<PlayerId>(world.add_player("Jose"));
    const auto second_id = std::get<PlayerId>(world.add_player("Mateo"));

    ASSERT_EQ(world.set_direction(first_id, {1, 0}), SetDirectionResult::Accepted);
    ASSERT_EQ(world.set_direction(second_id, {0, 1}), SetDirectionResult::Accepted);
    world.update_world(std::chrono::duration<double>{0.5});

    const auto state = world.get_state();
    ASSERT_EQ(state.players.size(), 2u);
    EXPECT_EQ(state.players.at(0).id, first_id);
    EXPECT_DOUBLE_EQ(state.players.at(0).position.x, 3.5);
    EXPECT_DOUBLE_EQ(state.players.at(0).position.y, 2.5);
    EXPECT_EQ(state.players.at(1).id, second_id);
    EXPECT_DOUBLE_EQ(state.players.at(1).position.x, 2.5);
    EXPECT_DOUBLE_EQ(state.players.at(1).position.y, 3.5);
}

}
