#include "inventory.h"

#include <cstddef>
#include <cstdint>

#include "gtest/gtest.h"

TEST(InventoryTest, StartsEmptyWithFirstSlotSelected) {
    game::Inventory inventory;

    EXPECT_EQ(inventory.get_slots().size(), 9u);
    EXPECT_EQ(inventory.get_selected_slot(), 0u);
    EXPECT_FALSE(inventory.get_selected_item().has_value());

    for (const auto& slot: inventory.get_slots()) {
        EXPECT_FALSE(slot.has_value());
    }
}

TEST(InventoryTest, SelectsEmptySlotAndPreservesSelectionOnInvalidIndex) {
    game::Inventory inventory;

    EXPECT_EQ(inventory.select(8), game::InventoryResult::Accepted);
    EXPECT_EQ(inventory.get_selected_slot(), 8u);
    EXPECT_FALSE(inventory.get_selected_item().has_value());

    EXPECT_EQ(inventory.select(9), game::InventoryResult::InvalidSlot);
    EXPECT_EQ(inventory.get_selected_slot(), 8u);
}

TEST(InventoryTest, FillsExistingStackBeforeUsingEmptySlot) {
    game::Inventory inventory;

    ASSERT_EQ(inventory.add({game::ItemType::StrawberrySeed, game::ItemQuality::Base, 40}),
              game::InventoryResult::Accepted);
    ASSERT_EQ(inventory.add({game::ItemType::StrawberrySeed, game::ItemQuality::Base, 15}),
              game::InventoryResult::Accepted);

    const auto& slots = inventory.get_slots();

    ASSERT_TRUE(slots[0].has_value());
    ASSERT_TRUE(slots[1].has_value());
    EXPECT_EQ(slots[0]->quantity, 50u);
    EXPECT_EQ(slots[1]->quantity, 5u);
    EXPECT_FALSE(slots[2].has_value());
}

TEST(InventoryTest, KeepsDifferentQualitiesInSeparateSlots) {
    game::Inventory inventory;

    ASSERT_EQ(inventory.add({game::ItemType::StrawberrySeed, game::ItemQuality::Base, 3}),
              game::InventoryResult::Accepted);
    ASSERT_EQ(inventory.add({game::ItemType::StrawberrySeed, game::ItemQuality::Good, 4}),
              game::InventoryResult::Accepted);

    const auto& slots = inventory.get_slots();

    ASSERT_TRUE(slots[0].has_value());
    ASSERT_TRUE(slots[1].has_value());
    EXPECT_EQ(slots[0]->quality, game::ItemQuality::Base);
    EXPECT_EQ(slots[0]->quantity, 3u);
    EXPECT_EQ(slots[1]->quality, game::ItemQuality::Good);
    EXPECT_EQ(slots[1]->quantity, 4u);
}

TEST(InventoryTest, PlacesEachToolInItsOwnSlot) {
    game::Inventory inventory;

    ASSERT_EQ(inventory.add({game::ItemType::Shovel, game::ItemQuality::Base, 2}),
              game::InventoryResult::Accepted);

    const auto& slots = inventory.get_slots();

    for (std::size_t index = 0; index < 2; ++index) {
        ASSERT_TRUE(slots[index].has_value());
        EXPECT_EQ(slots[index]->type, game::ItemType::Shovel);
        EXPECT_EQ(slots[index]->quantity, 1u);
    }

    EXPECT_FALSE(slots[2].has_value());
}

TEST(InventoryTest, RejectsAdditionWithoutPartiallyFillingExistingStack) {
    game::Inventory inventory;

    ASSERT_EQ(inventory.add({game::ItemType::StrawberrySeed, game::ItemQuality::Base, 48}),
              game::InventoryResult::Accepted);
    ASSERT_EQ(inventory.add({game::ItemType::Shovel, game::ItemQuality::Base, 8}),
              game::InventoryResult::Accepted);
    ASSERT_EQ(inventory.select(4), game::InventoryResult::Accepted);

    EXPECT_EQ(inventory.add({game::ItemType::StrawberrySeed, game::ItemQuality::Base, 3}),
              game::InventoryResult::NotEnoughSpace);

    const auto& slots = inventory.get_slots();

    ASSERT_TRUE(slots[0].has_value());
    EXPECT_EQ(slots[0]->quantity, 48u);

    for (std::size_t index = 1; index < slots.size(); ++index) {
        ASSERT_TRUE(slots[index].has_value());
        EXPECT_EQ(slots[index]->type, game::ItemType::Shovel);
        EXPECT_EQ(slots[index]->quantity, 1u);
    }

    EXPECT_EQ(inventory.get_selected_slot(), 4u);
}

TEST(InventoryTest, RejectsInvalidItemsAndQuantitiesWithoutAddingObjects) {
    game::Inventory inventory;

    EXPECT_EQ(inventory.add({game::ItemType::StrawberrySeed, game::ItemQuality::Base, 0}),
              game::InventoryResult::InvalidQuantity);

    EXPECT_EQ(inventory.add({static_cast<game::ItemType>(255), game::ItemQuality::Base, 1}),
              game::InventoryResult::InvalidItem);

    EXPECT_EQ(
            inventory.add({game::ItemType::StrawberrySeed, static_cast<game::ItemQuality>(255), 1}),
            game::InventoryResult::InvalidItem);

    for (const auto& slot: inventory.get_slots()) {
        EXPECT_FALSE(slot.has_value());
    }
}

TEST(InventoryTest, RemovesItemsAndEmptiesSlotWhenLastUnitIsConsumed) {
    game::Inventory inventory;

    ASSERT_EQ(inventory.add({game::ItemType::StrawberrySeed, game::ItemQuality::Base, 5}),
              game::InventoryResult::Accepted);

    ASSERT_EQ(inventory.remove(0, 2), game::InventoryResult::Accepted);

    const auto selected = inventory.get_selected_item();
    ASSERT_TRUE(selected.has_value());
    EXPECT_EQ(selected->quantity, 3u);

    ASSERT_EQ(inventory.remove(0, 3), game::InventoryResult::Accepted);

    EXPECT_FALSE(inventory.get_selected_item().has_value());
    EXPECT_EQ(inventory.get_selected_slot(), 0u);
}

TEST(InventoryTest, RejectsInvalidRemovalWithoutChangingContents) {
    game::Inventory inventory;

    ASSERT_EQ(inventory.add({game::ItemType::StrawberrySeed, game::ItemQuality::Base, 2}),
              game::InventoryResult::Accepted);

    EXPECT_EQ(inventory.remove(9, 1), game::InventoryResult::InvalidSlot);
    EXPECT_EQ(inventory.remove(0, 0), game::InventoryResult::InvalidQuantity);
    EXPECT_EQ(inventory.remove(1, 1), game::InventoryResult::EmptySlot);
    EXPECT_EQ(inventory.remove(0, 3), game::InventoryResult::NotEnoughItems);

    const auto selected = inventory.get_selected_item();
    ASSERT_TRUE(selected.has_value());
    EXPECT_EQ(selected->quantity, 2u);
}

TEST(InventoryTest, SelectedItemIsAnIndependentCopy) {
    game::Inventory inventory;

    ASSERT_EQ(inventory.add({game::ItemType::StrawberrySeed, game::ItemQuality::Base, 12}),
              game::InventoryResult::Accepted);

    auto copy = inventory.get_selected_item();
    ASSERT_TRUE(copy.has_value());
    copy->quantity = 1;

    const auto original = inventory.get_selected_item();
    ASSERT_TRUE(original.has_value());
    EXPECT_EQ(original->quantity, 12u);
}
