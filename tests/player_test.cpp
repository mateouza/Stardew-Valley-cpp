#include "player.h"

#include <array>
#include <cstddef>
#include <cstdint>

#include "gtest/gtest.h"

namespace game {

class PlayerTest: public ::testing::Test {
protected:
    Player player{7, "Jose", {2.5, 3.5}};
};

TEST_F(PlayerTest, StartsConnectedAndStillFacingDown) {
    const auto state = player.get_state();
    const auto direction = player.get_direction();

    EXPECT_EQ(state.id, 7u);
    EXPECT_EQ(state.name, "Jose");
    EXPECT_DOUBLE_EQ(state.position.x, 2.5);
    EXPECT_DOUBLE_EQ(state.position.y, 3.5);
    EXPECT_TRUE(player.is_connected());
    EXPECT_FALSE(state.moving);
    EXPECT_EQ(state.facing.horizontal, 0);
    EXPECT_EQ(state.facing.vertical, 1);
    EXPECT_EQ(direction.horizontal, 0);
    EXPECT_EQ(direction.vertical, 0);
}

TEST_F(PlayerTest, AcceptsAllNineDirections) {
    for (int horizontal = -1; horizontal <= 1; ++horizontal) {
        for (int vertical = -1; vertical <= 1; ++vertical) {
            const MovementDirection requested{static_cast<std::int8_t>(horizontal),
                                              static_cast<std::int8_t>(vertical)};

            ASSERT_EQ(player.set_direction(requested), SetDirectionResult::Accepted);
            EXPECT_EQ(player.get_direction().horizontal, horizontal);
            EXPECT_EQ(player.get_direction().vertical, vertical);
        }
    }
}

TEST_F(PlayerTest, DirectionChangesFacingWithoutMovingPosition) {
    ASSERT_EQ(player.set_direction({-1, 1}), SetDirectionResult::Accepted);

    const auto state = player.get_state();
    EXPECT_EQ(state.facing.horizontal, -1);
    EXPECT_EQ(state.facing.vertical, 1);
    EXPECT_DOUBLE_EQ(state.position.x, 2.5);
    EXPECT_DOUBLE_EQ(state.position.y, 3.5);
    EXPECT_FALSE(state.moving);
}

TEST_F(PlayerTest, StoppingPreservesFacing) {
    ASSERT_EQ(player.set_direction({-1, 0}), SetDirectionResult::Accepted);
    ASSERT_EQ(player.set_direction({0, 0}), SetDirectionResult::Accepted);

    EXPECT_EQ(player.get_direction().horizontal, 0);
    EXPECT_EQ(player.get_direction().vertical, 0);
    EXPECT_EQ(player.get_state().facing.horizontal, -1);
    EXPECT_EQ(player.get_state().facing.vertical, 0);
}

TEST_F(PlayerTest, InvalidDirectionPreservesIntentionAndFacing) {
    ASSERT_EQ(player.set_direction({1, -1}), SetDirectionResult::Accepted);

    const MovementDirection invalid_directions[] = {{-2, 0}, {2, 0}, {0, -2}, {0, 2}};
    for (const auto direction: invalid_directions) {
        EXPECT_EQ(player.set_direction(direction), SetDirectionResult::InvalidDirection);
        EXPECT_EQ(player.get_direction().horizontal, 1);
        EXPECT_EQ(player.get_direction().vertical, -1);
        EXPECT_EQ(player.get_state().facing.horizontal, 1);
        EXPECT_EQ(player.get_state().facing.vertical, -1);
    }
}

TEST_F(PlayerTest, PositionChangeMarksPlayerAsMoving) {
    player.set_position({3.0, 3.5});

    const auto state = player.get_state();
    EXPECT_DOUBLE_EQ(state.position.x, 3.0);
    EXPECT_DOUBLE_EQ(state.position.y, 3.5);
    EXPECT_TRUE(state.moving);
}

TEST_F(PlayerTest, VerticalPositionChangeAlsoMarksPlayerAsMoving) {
    player.set_position({2.5, 4.0});

    EXPECT_DOUBLE_EQ(player.get_state().position.y, 4.0);
    EXPECT_TRUE(player.get_state().moving);
}

TEST_F(PlayerTest, UnchangedPositionClearsPreviousMovement) {
    player.set_position({3.0, 3.5});
    ASSERT_TRUE(player.get_state().moving);

    player.set_position({3.0, 3.5});

    EXPECT_FALSE(player.get_state().moving);
}

TEST_F(PlayerTest, BlockedMovementPreservesFacingButDoesNotMarkMovement) {
    ASSERT_EQ(player.set_direction({1, 0}), SetDirectionResult::Accepted);

    player.set_position({2.5, 3.5});

    EXPECT_FALSE(player.get_state().moving);
    EXPECT_EQ(player.get_state().facing.horizontal, 1);
    EXPECT_EQ(player.get_state().facing.vertical, 0);
    EXPECT_EQ(player.get_direction().horizontal, 1);
}

TEST_F(PlayerTest, DisconnectClearsIntentionAndPreservesCharacter) {
    ASSERT_EQ(player.set_direction({-1, 0}), SetDirectionResult::Accepted);
    player.set_position({1.5, 3.5});

    player.disconnect();

    const auto state = player.get_state();
    EXPECT_FALSE(player.is_connected());
    EXPECT_FALSE(state.moving);
    EXPECT_EQ(player.get_direction().horizontal, 0);
    EXPECT_EQ(player.get_direction().vertical, 0);
    EXPECT_EQ(state.id, 7u);
    EXPECT_EQ(state.name, "Jose");
    EXPECT_DOUBLE_EQ(state.position.x, 1.5);
    EXPECT_DOUBLE_EQ(state.position.y, 3.5);
    EXPECT_EQ(state.facing.horizontal, -1);
    EXPECT_EQ(state.facing.vertical, 0);
}

TEST_F(PlayerTest, RepeatedDisconnectKeepsPlayerDisconnectedAndStill) {
    ASSERT_EQ(player.set_direction({0, -1}), SetDirectionResult::Accepted);

    player.disconnect();
    player.disconnect();

    EXPECT_FALSE(player.is_connected());
    EXPECT_EQ(player.get_direction().horizontal, 0);
    EXPECT_EQ(player.get_direction().vertical, 0);
}

TEST_F(PlayerTest, DisconnectedPlayerRejectsDirectionWithoutChangingFacing) {
    player.disconnect();

    EXPECT_EQ(player.set_direction({1, 0}), SetDirectionResult::DisconnectedPlayer);
    EXPECT_EQ(player.get_direction().horizontal, 0);
    EXPECT_EQ(player.get_direction().vertical, 0);
    EXPECT_EQ(player.get_state().facing.horizontal, 0);
    EXPECT_EQ(player.get_state().facing.vertical, 1);
}

TEST_F(PlayerTest, ReconnectPreservesCharacterAndStartsWithoutIntention) {
    ASSERT_EQ(player.set_direction({0, -1}), SetDirectionResult::Accepted);
    player.set_position({2.5, 2.0});
    player.disconnect();

    ASSERT_EQ(player.reconnect(), ReconnectPlayerResult::Accepted);

    const auto state = player.get_state();
    EXPECT_TRUE(player.is_connected());
    EXPECT_FALSE(state.moving);
    EXPECT_EQ(state.id, 7u);
    EXPECT_EQ(state.name, "Jose");
    EXPECT_DOUBLE_EQ(state.position.x, 2.5);
    EXPECT_DOUBLE_EQ(state.position.y, 2.0);
    EXPECT_EQ(state.facing.horizontal, 0);
    EXPECT_EQ(state.facing.vertical, -1);
    EXPECT_EQ(player.get_direction().horizontal, 0);
    EXPECT_EQ(player.get_direction().vertical, 0);
    EXPECT_EQ(player.set_direction({1, 0}), SetDirectionResult::Accepted);
}

TEST_F(PlayerTest, ReconnectingConnectedPlayerPreservesIntention) {
    ASSERT_EQ(player.set_direction({1, -1}), SetDirectionResult::Accepted);
    player.set_position({3.0, 3.5});

    EXPECT_EQ(player.reconnect(), ReconnectPlayerResult::AlreadyConnected);
    EXPECT_EQ(player.get_direction().horizontal, 1);
    EXPECT_EQ(player.get_direction().vertical, -1);
    EXPECT_EQ(player.get_state().facing.horizontal, 1);
    EXPECT_EQ(player.get_state().facing.vertical, -1);
    EXPECT_TRUE(player.get_state().moving);
}

TEST_F(PlayerTest, StoppingDoesNotRewriteLastAppliedMovement) {
    player.set_position({3.0, 3.5});
    ASSERT_EQ(player.set_direction({0, 0}), SetDirectionResult::Accepted);
    EXPECT_TRUE(player.get_state().moving);

    player.set_position({3.0, 3.5});
    EXPECT_FALSE(player.get_state().moving);
}

TEST_F(PlayerTest, StateIsAnIndependentSnapshot) {
    auto state = player.get_state();
    state.name = "Otro";
    state.position.x = 99.0;
    state.facing.horizontal = -1;
    state.moving = true;

    const auto current = player.get_state();
    EXPECT_EQ(current.name, "Jose");
    EXPECT_DOUBLE_EQ(current.position.x, 2.5);
    EXPECT_EQ(current.facing.horizontal, 0);
    EXPECT_FALSE(current.moving);

    player.set_position({4.0, 3.5});
    EXPECT_DOUBLE_EQ(state.position.x, 99.0);
}

TEST_F(PlayerTest, KeepsInventoriesIndependentBetweenPlayers) {
    Player other(8, "Mateo", {0.5, 0.5});

    ASSERT_EQ(player.add_item({ItemType::StrawberrySeed, ItemQuality::Base, 12}),
              InventoryResult::Accepted);

    ASSERT_EQ(player.select_slot(4), InventoryResult::Accepted);
    ASSERT_EQ(other.select_slot(4), InventoryResult::Accepted);

    const auto item = player.get_selected_item();
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->type, ItemType::StrawberrySeed);
    EXPECT_EQ(item->quantity, 12u);

    EXPECT_FALSE(other.get_selected_item().has_value());

    ASSERT_EQ(player.select_slot(2), InventoryResult::Accepted);
    EXPECT_EQ(player.get_selected_slot(), 2u);
    EXPECT_EQ(other.get_selected_slot(), 4u);
}

TEST_F(PlayerTest, DelegatesRemovalAndPreservesStateOnRejection) {
    ASSERT_EQ(player.add_item({ItemType::StrawberrySeed, ItemQuality::Base, 2}),
              InventoryResult::Accepted);
    ASSERT_EQ(player.select_slot(4), InventoryResult::Accepted);

    EXPECT_EQ(player.remove_item(4, 3), InventoryResult::NotEnoughItems);

    const auto item = player.get_selected_item();
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->quantity, 2u);

    EXPECT_EQ(player.select_slot(9), InventoryResult::InvalidSlot);
    EXPECT_EQ(player.get_selected_slot(), 4u);

    ASSERT_EQ(player.remove_item(4, 2), InventoryResult::Accepted);
    EXPECT_FALSE(player.get_selected_item().has_value());
}

TEST_F(PlayerTest, StartsWithFourToolsAndFiveEmptySlots) {
    const std::array<ItemType, 4> expected{ItemType::Pickaxe, ItemType::WateringCan,
                                           ItemType::Shovel, ItemType::Sword};

    EXPECT_EQ(player.get_selected_slot(), 0u);

    for (std::size_t slot = 0; slot < expected.size(); ++slot) {
        ASSERT_EQ(player.select_slot(slot), InventoryResult::Accepted);

        const auto item = player.get_selected_item();
        ASSERT_TRUE(item.has_value());
        EXPECT_EQ(item->type, expected[slot]);
        EXPECT_EQ(item->quality, ItemQuality::Base);
        EXPECT_EQ(item->quantity, 1u);
    }

    for (std::size_t slot = expected.size(); slot < 9; ++slot) {
        ASSERT_EQ(player.select_slot(slot), InventoryResult::Accepted);
        EXPECT_FALSE(player.get_selected_item().has_value());
    }
}

TEST_F(PlayerTest, ReconnectPreservesInventoryWithoutAddingToolsAgain) {
    ASSERT_EQ(player.add_item({ItemType::StrawberrySeed, ItemQuality::Base, 12}),
              InventoryResult::Accepted);
    ASSERT_EQ(player.select_slot(4), InventoryResult::Accepted);

    player.disconnect();
    ASSERT_EQ(player.reconnect(), ReconnectPlayerResult::Accepted);

    EXPECT_EQ(player.get_selected_slot(), 4u);

    const auto item = player.get_selected_item();
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->type, ItemType::StrawberrySeed);
    EXPECT_EQ(item->quantity, 12u);

    const std::array<ItemType, 4> expected{ItemType::Pickaxe, ItemType::WateringCan,
                                           ItemType::Shovel, ItemType::Sword};

    for (std::size_t slot = 0; slot < expected.size(); ++slot) {
        ASSERT_EQ(player.select_slot(slot), InventoryResult::Accepted);

        const auto tool = player.get_selected_item();
        ASSERT_TRUE(tool.has_value());
        EXPECT_EQ(tool->type, expected[slot]);
        EXPECT_EQ(tool->quantity, 1u);
    }

    for (std::size_t slot = 5; slot < 9; ++slot) {
        ASSERT_EQ(player.select_slot(slot), InventoryResult::Accepted);
        EXPECT_FALSE(player.get_selected_item().has_value());
    }
}

}
