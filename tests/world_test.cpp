#include "world.h"

#include <chrono>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "gtest/gtest.h"

namespace game {

class WorldTest: public ::testing::Test {
protected:
    static Map make_map() {
        return Map{std::vector<std::vector<TileType>>(
                5, std::vector<TileType>(5, TileType::Walkable))};
    }

    World world{make_map(), {2.5, 2.5}, {2, 1.0, 0.5, 0.5}};

    PlayerId add_player(const std::string& name = "Jose") {
        return std::get<PlayerId>(world.add_player(name));
    }
};

TEST_F(WorldTest, StartsWithoutPlayers) { EXPECT_TRUE(world.get_state().players.empty()); }

TEST_F(WorldTest, RejectsZeroPlayerCapacity) {
    EXPECT_THROW((World{make_map(), {2.5, 2.5}, {0, 1.0, 0.5, 0.5}}), std::invalid_argument);
}

TEST_F(WorldTest, RejectsInvalidSpeed) {
    const double speeds[] = {0.0, -1.0, std::numeric_limits<double>::infinity(),
                             std::numeric_limits<double>::quiet_NaN()};
    for (const double speed: speeds) {
        EXPECT_THROW((World{make_map(), {2.5, 2.5}, {2, speed, 0.5, 0.5}}), std::invalid_argument);
    }
}

TEST_F(WorldTest, RejectsInvalidBodyDimensions) {
    const double sizes[] = {0.0, -1.0, std::numeric_limits<double>::infinity(),
                            std::numeric_limits<double>::quiet_NaN()};
    for (const double size: sizes) {
        EXPECT_THROW((World{make_map(), {2.5, 2.5}, {2, 1.0, size, 0.5}}), std::invalid_argument);
        EXPECT_THROW((World{make_map(), {2.5, 2.5}, {2, 1.0, 0.5, size}}), std::invalid_argument);
    }
}

TEST_F(WorldTest, RejectsSpawnOutsideMapOrPartiallyOutsideMap) {
    EXPECT_THROW((World{make_map(), {-1.0, 2.5}, {2, 1.0, 0.5, 0.5}}), std::invalid_argument);
    EXPECT_THROW((World{make_map(), {0.1, 2.5}, {2, 1.0, 0.5, 0.5}}), std::invalid_argument);
}

TEST_F(WorldTest, RejectsSpawnInsideWall) {
    EXPECT_THROW((World{Map{{{TileType::Blocked}}}, {0.5, 0.5}, {2, 1.0, 0.5, 0.5}}),
                 std::invalid_argument);
}

TEST_F(WorldTest, AddsPlayerAtSpawnWithIndependentIdentity) {
    const auto first = add_player("Jose");
    const auto second = add_player("Mateo");
    const auto state = world.get_state();

    ASSERT_EQ(state.players.size(), 2u);
    EXPECT_NE(first, second);
    EXPECT_EQ(state.players.at(0).id, first);
    EXPECT_EQ(state.players.at(0).name, "Jose");
    EXPECT_DOUBLE_EQ(state.players.at(0).position.x, 2.5);
    EXPECT_DOUBLE_EQ(state.players.at(0).position.y, 2.5);
    EXPECT_FALSE(state.players.at(0).moving);
    EXPECT_EQ(state.players.at(1).id, second);
    EXPECT_EQ(state.players.at(1).name, "Mateo");
}

TEST_F(WorldTest, RejectsNewPlayerWhenConnectedCapacityIsFull) {
    add_player("Jose");
    add_player("Mateo");

    const auto result = world.add_player("Juana");

    ASSERT_TRUE(std::holds_alternative<AddPlayerError>(result));
    EXPECT_EQ(std::get<AddPlayerError>(result), AddPlayerError::GameFull);
    EXPECT_EQ(world.get_state().players.size(), 2u);
}

TEST_F(WorldTest, DisconnectFreesCapacityWithoutReusingId) {
    const auto first = add_player("Jose");
    const auto second = add_player("Mateo");
    world.disconnect_player(first);

    const auto third = add_player("Juana");

    EXPECT_GT(third, second);
    EXPECT_NE(third, first);
    EXPECT_EQ(world.get_state().players.size(), 2u);
}

TEST_F(WorldTest, RejectedAdditionDoesNotConsumeId) {
    const auto first = add_player("Jose");
    const auto second = add_player("Mateo");
    ASSERT_TRUE(std::holds_alternative<AddPlayerError>(world.add_player("Juana")));
    world.disconnect_player(first);

    EXPECT_EQ(add_player("Kika"), second + 1);
}

TEST_F(WorldTest, DisconnectRemovesPlayerFromSnapshot) {
    const auto first = add_player("Jose");
    const auto second = add_player("Mateo");

    world.disconnect_player(first);

    const auto state = world.get_state();
    ASSERT_EQ(state.players.size(), 1u);
    EXPECT_EQ(state.players.at(0).id, second);
}

TEST_F(WorldTest, UnknownAndRepeatedDisconnectDoNotFreeExtraCapacity) {
    const auto first = add_player("Jose");
    add_player("Mateo");
    world.disconnect_player(std::numeric_limits<PlayerId>::max());
    ASSERT_TRUE(std::holds_alternative<AddPlayerError>(world.add_player("Juana")));

    world.disconnect_player(first);
    world.disconnect_player(first);
    add_player("Juana");

    EXPECT_TRUE(std::holds_alternative<AddPlayerError>(world.add_player("Kika")));
}

TEST_F(WorldTest, UnknownReconnectIsRejectedEvenWhenFull) {
    add_player("Jose");
    add_player("Mateo");

    EXPECT_EQ(world.reconnect_player(std::numeric_limits<PlayerId>::max()),
              ReconnectPlayerResult::UnknownPlayer);
}

TEST_F(WorldTest, ConnectedReconnectDoesNotResetIntentionEvenWhenFull) {
    const auto first = add_player("Jose");
    add_player("Mateo");
    ASSERT_EQ(world.set_direction(first, {1, 0}), SetDirectionResult::Accepted);

    EXPECT_EQ(world.reconnect_player(first), ReconnectPlayerResult::AlreadyConnected);
    world.update_world(std::chrono::duration<double>{1.0});

    EXPECT_DOUBLE_EQ(world.get_state().players.at(0).position.x, 3.5);
}

TEST_F(WorldTest, ReconnectChecksConnectedCapacityAndRetainsCharacter) {
    const auto first = add_player("Jose");
    add_player("Mateo");
    world.disconnect_player(first);
    const auto third = add_player("Juana");

    EXPECT_EQ(world.reconnect_player(first), ReconnectPlayerResult::GameFull);
    EXPECT_EQ(world.set_direction(first, {1, 0}), SetDirectionResult::DisconnectedPlayer);
    world.disconnect_player(third);

    EXPECT_EQ(world.reconnect_player(first), ReconnectPlayerResult::Accepted);
    EXPECT_EQ(world.get_state().players.at(0).id, first);
    EXPECT_EQ(world.get_state().players.at(0).name, "Jose");
}

TEST_F(WorldTest, ReconnectPreservesPositionAndFacingWithoutResumingMovement) {
    const auto id = add_player();
    ASSERT_EQ(world.set_direction(id, {1, 0}), SetDirectionResult::Accepted);
    world.update_world(std::chrono::duration<double>{1.0});
    world.disconnect_player(id);

    ASSERT_EQ(world.reconnect_player(id), ReconnectPlayerResult::Accepted);
    world.update_world(std::chrono::duration<double>{1.0});

    const auto state = world.get_state().players.at(0);
    EXPECT_EQ(state.id, id);
    EXPECT_DOUBLE_EQ(state.position.x, 3.5);
    EXPECT_EQ(state.facing.horizontal, 1);
    EXPECT_EQ(state.facing.vertical, 0);
    EXPECT_FALSE(state.moving);
}

TEST_F(WorldTest, DisconnectedPlayerStaysStillAcrossUpdatesAndReconnect) {
    const auto id = add_player();
    ASSERT_EQ(world.set_direction(id, {1, 0}), SetDirectionResult::Accepted);
    world.update_world(std::chrono::duration<double>{1.0});
    world.disconnect_player(id);

    world.update_world(std::chrono::duration<double>{1.0});
    ASSERT_EQ(world.reconnect_player(id), ReconnectPlayerResult::Accepted);

    EXPECT_DOUBLE_EQ(world.get_state().players.at(0).position.x, 3.5);
    EXPECT_FALSE(world.get_state().players.at(0).moving);
}

TEST_F(WorldTest, ImmediateReconnectDoesNotExposePreviousMovement) {
    const auto id = add_player();
    ASSERT_EQ(world.set_direction(id, {1, 0}), SetDirectionResult::Accepted);
    world.update_world(std::chrono::duration<double>{1.0});
    ASSERT_TRUE(world.get_state().players.at(0).moving);

    world.disconnect_player(id);
    ASSERT_EQ(world.reconnect_player(id), ReconnectPlayerResult::Accepted);

    const auto state = world.get_state().players.at(0);
    EXPECT_FALSE(state.moving);
    EXPECT_DOUBLE_EQ(state.position.x, 3.5);
    EXPECT_DOUBLE_EQ(state.position.y, 2.5);
    EXPECT_EQ(state.facing.horizontal, 1);
    EXPECT_EQ(state.facing.vertical, 0);
}

TEST_F(WorldTest, UnknownDirectionRequestDoesNotAffectExistingPlayer) {
    const auto id = add_player();
    ASSERT_EQ(world.set_direction(id, {1, 0}), SetDirectionResult::Accepted);

    EXPECT_EQ(world.set_direction(std::numeric_limits<PlayerId>::max(), {0, 0}),
              SetDirectionResult::UnknownPlayer);
    world.update_world(std::chrono::duration<double>{1.0});

    EXPECT_DOUBLE_EQ(world.get_state().players.at(0).position.x, 3.5);
}

TEST_F(WorldTest, DisconnectedPlayerCannotReceiveMovementIntention) {
    const auto id = add_player();
    world.disconnect_player(id);

    EXPECT_EQ(world.set_direction(id, {1, 0}), SetDirectionResult::DisconnectedPlayer);
}

TEST_F(WorldTest, DirectionRequestDoesNotMoveUntilWorldUpdate) {
    const auto id = add_player();

    ASSERT_EQ(world.set_direction(id, {1, 0}), SetDirectionResult::Accepted);

    const auto state = world.get_state().players.at(0);
    EXPECT_DOUBLE_EQ(state.position.x, 2.5);
    EXPECT_EQ(state.facing.horizontal, 1);
    EXPECT_FALSE(state.moving);
}

TEST_F(WorldTest, InvalidDirectionPreservesPreviousMovementIntention) {
    const auto id = add_player();
    ASSERT_EQ(world.set_direction(id, {1, 0}), SetDirectionResult::Accepted);

    EXPECT_EQ(world.set_direction(id, {0, 2}), SetDirectionResult::InvalidDirection);
    world.update_world(std::chrono::duration<double>{1.0});

    EXPECT_DOUBLE_EQ(world.get_state().players.at(0).position.x, 3.5);
    EXPECT_DOUBLE_EQ(world.get_state().players.at(0).position.y, 2.5);
}

TEST_F(WorldTest, MovementUsesElapsedSecondsAndPersistsAcrossUpdates) {
    const auto id = add_player();
    ASSERT_EQ(world.set_direction(id, {1, 0}), SetDirectionResult::Accepted);

    world.update_world(std::chrono::duration<double>{0.25});
    EXPECT_DOUBLE_EQ(world.get_state().players.at(0).position.x, 2.75);
    world.update_world(std::chrono::duration<double>{0.5});

    EXPECT_DOUBLE_EQ(world.get_state().players.at(0).position.x, 3.25);
    EXPECT_TRUE(world.get_state().players.at(0).moving);
}

TEST_F(WorldTest, DiagonalMovementHasSameSpeedAsStraightMovement) {
    const auto id = add_player();
    ASSERT_EQ(world.set_direction(id, {1, 1}), SetDirectionResult::Accepted);

    world.update_world(std::chrono::duration<double>{1.0});

    const auto state = world.get_state().players.at(0);
    EXPECT_NEAR(std::hypot(state.position.x - 2.5, state.position.y - 2.5), 1.0, 1e-12);
    EXPECT_DOUBLE_EQ(state.position.x, state.position.y);
    EXPECT_EQ(state.facing.horizontal, 1);
    EXPECT_EQ(state.facing.vertical, 1);
}

TEST_F(WorldTest, DifferentPlayersKeepIndependentDirections) {
    const auto first = add_player("Jose");
    const auto second = add_player("Mateo");
    ASSERT_EQ(world.set_direction(first, {1, 0}), SetDirectionResult::Accepted);
    ASSERT_EQ(world.set_direction(second, {0, -1}), SetDirectionResult::Accepted);

    world.update_world(std::chrono::duration<double>{1.0});

    const auto state = world.get_state();
    EXPECT_DOUBLE_EQ(state.players.at(0).position.x, 3.5);
    EXPECT_DOUBLE_EQ(state.players.at(0).position.y, 2.5);
    EXPECT_DOUBLE_EQ(state.players.at(1).position.x, 2.5);
    EXPECT_DOUBLE_EQ(state.players.at(1).position.y, 1.5);
}

TEST_F(WorldTest, StopPreservesFacingAndClearsMovementOnNextUpdate) {
    const auto id = add_player();
    ASSERT_EQ(world.set_direction(id, {1, 0}), SetDirectionResult::Accepted);
    world.update_world(std::chrono::duration<double>{1.0});

    ASSERT_EQ(world.set_direction(id, {0, 0}), SetDirectionResult::Accepted);
    world.update_world(std::chrono::duration<double>{1.0});

    const auto state = world.get_state().players.at(0);
    EXPECT_DOUBLE_EQ(state.position.x, 3.5);
    EXPECT_EQ(state.facing.horizontal, 1);
    EXPECT_EQ(state.facing.vertical, 0);
    EXPECT_FALSE(state.moving);
}

TEST_F(WorldTest, MapEdgeStopsMovementWithoutChangingFacing) {
    const auto id = add_player();
    ASSERT_EQ(world.set_direction(id, {1, 0}), SetDirectionResult::Accepted);
    world.update_world(std::chrono::duration<double>{100.0});
    ASSERT_DOUBLE_EQ(world.get_state().players.at(0).position.x, 4.75);

    world.update_world(std::chrono::duration<double>{1.0});

    const auto state = world.get_state().players.at(0);
    EXPECT_DOUBLE_EQ(state.position.x, 4.75);
    EXPECT_EQ(state.facing.horizontal, 1);
    EXPECT_FALSE(state.moving);
}

TEST_F(WorldTest, ZeroElapsedClearsPreviousMovementWithoutLosingIntention) {
    const auto id = add_player();
    ASSERT_EQ(world.set_direction(id, {1, 0}), SetDirectionResult::Accepted);
    world.update_world(std::chrono::duration<double>{1.0});

    world.update_world(std::chrono::duration<double>{0.0});
    EXPECT_FALSE(world.get_state().players.at(0).moving);
    EXPECT_DOUBLE_EQ(world.get_state().players.at(0).position.x, 3.5);

    world.update_world(std::chrono::duration<double>{0.5});
    EXPECT_DOUBLE_EQ(world.get_state().players.at(0).position.x, 4.0);
}

TEST_F(WorldTest, InvalidElapsedDoesNotChangeWorldState) {
    const auto id = add_player();
    ASSERT_EQ(world.set_direction(id, {1, 0}), SetDirectionResult::Accepted);
    const double intervals[] = {-1.0, std::numeric_limits<double>::infinity(),
                                std::numeric_limits<double>::quiet_NaN()};

    for (const double interval: intervals) {
        EXPECT_THROW(world.update_world(std::chrono::duration<double>{interval}),
                     std::invalid_argument);
        EXPECT_DOUBLE_EQ(world.get_state().players.at(0).position.x, 2.5);
        EXPECT_FALSE(world.get_state().players.at(0).moving);
    }
}

TEST_F(WorldTest, StateIsIndependentFromWorldAndLaterSnapshots) {
    const auto id = add_player();
    auto snapshot = world.get_state();
    snapshot.players.at(0).name = "Otro";
    snapshot.players.at(0).position.x = 99.0;
    ASSERT_EQ(world.set_direction(id, {1, 0}), SetDirectionResult::Accepted);

    world.update_world(std::chrono::duration<double>{1.0});
    add_player("Mateo");

    const auto current = world.get_state();
    EXPECT_EQ(current.players.at(0).name, "Jose");
    EXPECT_DOUBLE_EQ(current.players.at(0).position.x, 3.5);
    EXPECT_DOUBLE_EQ(snapshot.players.at(0).position.x, 99.0);
    EXPECT_EQ(snapshot.players.size(), 1u);
    EXPECT_EQ(current.players.size(), 2u);
}

TEST_F(WorldTest, DifferentWorldsDoNotSharePlayerState) {
    World other{make_map(), {2.5, 2.5}, {2, 1.0, 0.5, 0.5}};
    const auto id = add_player("Jose");
    const auto other_id = std::get<PlayerId>(other.add_player("Mateo"));
    ASSERT_EQ(world.set_direction(id, {1, 0}), SetDirectionResult::Accepted);

    world.update_world(std::chrono::duration<double>{1.0});
    other.update_world(std::chrono::duration<double>{1.0});

    EXPECT_DOUBLE_EQ(world.get_state().players.at(0).position.x, 3.5);
    EXPECT_EQ(other.get_state().players.at(0).id, other_id);
    EXPECT_DOUBLE_EQ(other.get_state().players.at(0).position.x, 2.5);
    EXPECT_FALSE(other.get_state().players.at(0).moving);
}

TEST(WorldMovementTest, AppliesConfiguredSpeed) {
    World world{Map{{{TileType::Walkable, TileType::Walkable, TileType::Walkable}}},
                {0.5, 0.5},
                {1, 2.0, 0.5, 0.5}};
    const auto id = std::get<PlayerId>(world.add_player("Jose"));
    ASSERT_EQ(world.set_direction(id, {1, 0}), SetDirectionResult::Accepted);

    world.update_world(std::chrono::duration<double>{0.5});

    EXPECT_DOUBLE_EQ(world.get_state().players.at(0).position.x, 1.5);
}

TEST(WorldMovementTest, CannotCrossIntermediateWallWithLargeElapsed) {
    World world{Map{{{TileType::Walkable, TileType::Blocked, TileType::Walkable}}},
                {0.5, 0.5},
                {1, 1.0, 0.5, 0.5}};
    const auto id = std::get<PlayerId>(world.add_player("Jose"));
    ASSERT_EQ(world.set_direction(id, {1, 0}), SetDirectionResult::Accepted);

    world.update_world(std::chrono::duration<double>{100.0});

    EXPECT_DOUBLE_EQ(world.get_state().players.at(0).position.x, 0.75);
    EXPECT_TRUE(world.get_state().players.at(0).moving);
}

TEST(WorldMovementTest, SlidesWithoutAcceleratingTheUnblockedAxis) {
    World world{Map{{{TileType::Walkable, TileType::Walkable, TileType::Walkable},
                     {TileType::Walkable, TileType::Blocked, TileType::Walkable},
                     {TileType::Walkable, TileType::Walkable, TileType::Walkable}}},
                {0.5, 1.5},
                {1, 1.0, 0.5, 0.5}};
    const auto id = std::get<PlayerId>(world.add_player("Jose"));
    ASSERT_EQ(world.set_direction(id, {1, 1}), SetDirectionResult::Accepted);

    world.update_world(std::chrono::duration<double>{1.0});

    const auto state = world.get_state().players.at(0);
    EXPECT_DOUBLE_EQ(state.position.x, 0.75);
    EXPECT_DOUBLE_EQ(state.position.y, 1.5 + std::sqrt(0.5));
    EXPECT_TRUE(state.moving);
}

TEST(WorldMovementTest, RejectsUnrepresentableDistanceBeforeUpdatingAnyPlayer) {
    World world{Map{{{TileType::Walkable}}},
                {0.5, 0.5},
                {2, std::numeric_limits<double>::max(), 0.5, 0.5}};
    const auto first = std::get<PlayerId>(world.add_player("Jose"));
    const auto second = std::get<PlayerId>(world.add_player("Mateo"));
    ASSERT_EQ(world.set_direction(first, {1, 0}), SetDirectionResult::Accepted);
    ASSERT_EQ(world.set_direction(second, {-1, 0}), SetDirectionResult::Accepted);

    EXPECT_THROW(world.update_world(std::chrono::duration<double>{2.0}), std::overflow_error);

    const auto state = world.get_state();
    EXPECT_DOUBLE_EQ(state.players.at(0).position.x, 0.5);
    EXPECT_DOUBLE_EQ(state.players.at(1).position.x, 0.5);
    EXPECT_FALSE(state.players.at(0).moving);
    EXPECT_FALSE(state.players.at(1).moving);
}

}
