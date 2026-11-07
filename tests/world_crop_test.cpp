#include "world.h"

#include <string>
#include <variant>

#include "gtest/gtest.h"

namespace game {

class WorldCropTest: public ::testing::Test {
protected:
    World world{Map{{{TileType::Walkable, TileType::Walkable, TileType::Walkable},
                     {TileType::Walkable, TileType::Walkable, TileType::Walkable}}},
                {0.5, 0.5},
                {2, 1.0, 0.5, 0.5},
                Soil{{{SoilState::Unprepared, SoilState::Unprepared, SoilState::Unprepared},
                      {SoilState::NotCultivable, SoilState::Prepared, SoilState::Unprepared}}}};
    const PlayerId player = std::get<PlayerId>(world.add_player("Jose"));

    void SetUp() override {
        ASSERT_EQ(world.add_item(player, {ItemType::StrawberrySeed, ItemQuality::Base, 2}),
                  InventoryResult::Accepted);
        ASSERT_EQ(world.select_slot(player, 4), SelectSlotResult::Accepted);
    }

    std::uint32_t seed_count(PlayerId id) const {
        const auto inventory = world.get_inventory_state(id);
        const auto& seeds = inventory->items.at(4);
        return seeds ? seeds->quantity : 0;
    }
};

TEST_F(WorldCropTest, DispatchesPreparationPlantingAndWateringFromTheSelectedItem) {
    GameModel& model = world;
    ASSERT_EQ(model.select_slot(player, 2), SelectSlotResult::Accepted);
    ASSERT_EQ(std::get<PrepareSoilResult>(model.use_selected_item(player, {0, 1})),
              PrepareSoilResult::Accepted);
    ASSERT_EQ(model.select_slot(player, 4), SelectSlotResult::Accepted);
    ASSERT_EQ(std::get<PlantCropResult>(model.use_selected_item(player, {0, 1})),
              PlantCropResult::Accepted);
    EXPECT_EQ(seed_count(player), 1u);

    const auto before = model.get_crops_state();
    ASSERT_EQ(before.size(), 1u);
    EXPECT_EQ(before.at(0).cell.row, 0u);
    EXPECT_EQ(before.at(0).cell.column, 1u);
    EXPECT_EQ(before.at(0).state.type, CropType::Strawberry);
    EXPECT_TRUE(before.at(0).state.needs_water);

    ASSERT_EQ(model.select_slot(player, 1), SelectSlotResult::Accepted);
    EXPECT_EQ(std::get<WaterCropResult>(model.use_selected_item(player, {0, 1})),
              WaterCropResult::Accepted);
    EXPECT_EQ(std::get<WaterCropResult>(model.use_selected_item(player, {0, 1})),
              WaterCropResult::AlreadyWatered);
    const auto after = model.get_crops_state();
    ASSERT_EQ(after.size(), 1u);
    EXPECT_FALSE(after.at(0).state.needs_water);
    EXPECT_TRUE(before.at(0).state.needs_water);
    EXPECT_EQ(model.get_inventory_state(player)->items.at(1)->quantity, 1u);
}

TEST_F(WorldCropTest, RejectedPlantingPreservesSeeds) {
    EXPECT_EQ(std::get<PlantCropResult>(world.use_selected_item(player, {0, 1})),
              PlantCropResult::SoilNotPrepared);
    EXPECT_EQ(std::get<PlantCropResult>(world.use_selected_item(player, {1, 0})),
              PlantCropResult::NotCultivable);
    EXPECT_EQ(seed_count(player), 2u);
    EXPECT_TRUE(world.get_crops_state().empty());
}

TEST_F(WorldCropTest, TwoPlayersCannotPlantTheSameCellOrLoseTheRejectedSeed) {
    const auto second = std::get<PlayerId>(world.add_player("Mateo"));
    ASSERT_EQ(world.add_item(second, {ItemType::StrawberrySeed, ItemQuality::Base, 2}),
              InventoryResult::Accepted);
    ASSERT_EQ(world.select_slot(second, 4), SelectSlotResult::Accepted);
    ASSERT_EQ(world.select_slot(player, 2), SelectSlotResult::Accepted);
    ASSERT_EQ(world.prepare_soil(player, {0, 1}), PrepareSoilResult::Accepted);
    ASSERT_EQ(world.select_slot(player, 4), SelectSlotResult::Accepted);

    EXPECT_EQ(std::get<PlantCropResult>(world.use_selected_item(player, {0, 1})),
              PlantCropResult::Accepted);
    EXPECT_EQ(std::get<PlantCropResult>(world.use_selected_item(second, {0, 1})),
              PlantCropResult::AlreadyPlanted);
    EXPECT_EQ(seed_count(player), 1u);
    EXPECT_EQ(seed_count(second), 2u);
    EXPECT_EQ(world.get_crops_state().size(), 1u);
}

TEST_F(WorldCropTest, RejectsUnknownDisconnectedAndOutOfReachActions) {
    EXPECT_EQ(std::get<UseItemError>(world.use_selected_item(player + 1, {0, 0})),
              UseItemError::UnknownPlayer);
    EXPECT_EQ(std::get<UseItemError>(world.use_selected_item(player, {2, 0})),
              UseItemError::OutOfBounds);
    EXPECT_EQ(std::get<UseItemError>(world.use_selected_item(player, {0, 3})),
              UseItemError::OutOfBounds);
    EXPECT_EQ(std::get<UseItemError>(world.use_selected_item(player, {0, 2})),
              UseItemError::OutOfReach);
    world.disconnect_player(player);
    EXPECT_EQ(std::get<UseItemError>(world.use_selected_item(player, {0, 0})),
              UseItemError::DisconnectedPlayer);
    EXPECT_EQ(seed_count(player), 2u);
    EXPECT_TRUE(world.get_crops_state().empty());
}

TEST_F(WorldCropTest, ConsumingTheLastSeedLeavesAnEmptySelectedSlot) {
    for (const CellPosition cell: {CellPosition{0, 0}, CellPosition{0, 1}}) {
        ASSERT_EQ(world.select_slot(player, 2), SelectSlotResult::Accepted);
        ASSERT_EQ(world.prepare_soil(player, cell), PrepareSoilResult::Accepted);
        ASSERT_EQ(world.select_slot(player, 4), SelectSlotResult::Accepted);
        ASSERT_EQ(std::get<PlantCropResult>(world.use_selected_item(player, cell)),
                  PlantCropResult::Accepted);
    }
    EXPECT_EQ(seed_count(player), 0u);
    const auto inventory = world.get_inventory_state(player);
    EXPECT_FALSE(inventory->items.at(4).has_value());
    EXPECT_EQ(inventory->selected_slot, 4u);
    EXPECT_EQ(std::get<UseItemError>(world.use_selected_item(player, {0, 0})),
              UseItemError::NoItemSelected);
}

TEST_F(WorldCropTest, RejectsUnsupportedItemsAndWateringWithoutACrop) {
    ASSERT_EQ(world.select_slot(player, 0), SelectSlotResult::Accepted);
    EXPECT_EQ(std::get<UseItemError>(world.use_selected_item(player, {0, 0})),
              UseItemError::UnsupportedItem);
    ASSERT_EQ(world.select_slot(player, 1), SelectSlotResult::Accepted);
    EXPECT_EQ(std::get<WaterCropResult>(world.use_selected_item(player, {0, 0})),
              WaterCropResult::NoCrop);
    EXPECT_TRUE(world.get_crops_state().empty());
    EXPECT_EQ(seed_count(player), 2u);
}

TEST_F(WorldCropTest, ServerCanAddItemsToRegisteredDisconnectedCharacters) {
    world.disconnect_player(player);
    EXPECT_EQ(world.add_item(player, {ItemType::StrawberrySeed, ItemQuality::Base, 1}),
              InventoryResult::Accepted);
    EXPECT_EQ(seed_count(player), 3u);
    EXPECT_EQ(world.add_item(player + 1, {ItemType::StrawberrySeed, ItemQuality::Base, 1}),
              InventoryResult::UnknownPlayer);
}

TEST(WorldCropWithoutSoilTest, RejectsUseAndReturnsNoCrops) {
    World world{Map{{{TileType::Walkable}}}, {0.5, 0.5}, {1, 1.0, 0.5, 0.5}};
    const auto player = std::get<PlayerId>(world.add_player("Jose"));
    EXPECT_EQ(std::get<UseItemError>(world.use_selected_item(player, {0, 0})),
              UseItemError::NotCultivable);
    EXPECT_TRUE(world.get_crops_state().empty());
}

}
