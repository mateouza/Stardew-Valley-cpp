#include "soil.h"

#include <limits>
#include <stdexcept>
#include <vector>

#include "gtest/gtest.h"

namespace game {

class SoilTest: public ::testing::Test {
protected:
    Soil soil{{{SoilState::Unprepared, SoilState::NotCultivable, SoilState::Prepared},
               {SoilState::Unprepared, SoilState::Unprepared, SoilState::NotCultivable}}};
};

TEST(SoilConstructionTest, RejectsEmptySoil) {
    const std::vector<std::vector<SoilState>> cells;

    EXPECT_THROW((Soil{cells}), std::invalid_argument);
}

TEST(SoilConstructionTest, RejectsEmptyRow) {
    const std::vector<std::vector<SoilState>> cells(1);

    EXPECT_THROW((Soil{cells}), std::invalid_argument);
}

TEST(SoilConstructionTest, RejectsRowsWithDifferentWidths) {
    const std::vector<std::vector<SoilState>> cells = {
            {SoilState::Unprepared, SoilState::Unprepared}, {SoilState::Unprepared}};

    EXPECT_THROW((Soil{cells}), std::invalid_argument);
}

TEST(SoilConstructionTest, RejectsUnknownState) {
    EXPECT_THROW((Soil{{{static_cast<SoilState>(99)}}}), std::invalid_argument);
}

TEST(SoilConstructionTest, OwnsItsStatesIndependentlyOfTheInput) {
    std::vector<std::vector<SoilState>> cells{{SoilState::Unprepared}};
    Soil soil{cells};
    cells.at(0).at(0) = SoilState::NotCultivable;

    EXPECT_EQ(soil.prepare(0, 0), PrepareSoilResult::Accepted);
    EXPECT_EQ(soil.get_state(0, 0), SoilState::Prepared);
}

TEST(SoilConstructionTest, AcceptsInitiallyPreparedSoil) {
    const Soil soil{{{SoilState::Prepared}}};

    EXPECT_EQ(soil.get_state(0, 0), SoilState::Prepared);
}

TEST_F(SoilTest, ReadsStatesUsingRowAndColumnOnRectangularSoil) {
    EXPECT_EQ(soil.get_state(0, 1), SoilState::NotCultivable);
    EXPECT_EQ(soil.get_state(1, 0), SoilState::Unprepared);
    EXPECT_EQ(soil.get_state(0, 2), SoilState::Prepared);
    EXPECT_EQ(soil.get_state(1, 2), SoilState::NotCultivable);
}

TEST_F(SoilTest, PreparingChangesOnlyTheSelectedCell) {
    ASSERT_EQ(soil.prepare(1, 1), PrepareSoilResult::Accepted);

    EXPECT_EQ(soil.get_state(1, 1), SoilState::Prepared);
    EXPECT_EQ(soil.get_state(0, 0), SoilState::Unprepared);
    EXPECT_EQ(soil.get_state(0, 1), SoilState::NotCultivable);
    EXPECT_EQ(soil.get_state(0, 2), SoilState::Prepared);
    EXPECT_EQ(soil.get_state(1, 0), SoilState::Unprepared);
    EXPECT_EQ(soil.get_state(1, 2), SoilState::NotCultivable);
}

TEST_F(SoilTest, RepeatedPreparationPreservesThePreparedCellAndItsNeighbors) {
    ASSERT_EQ(soil.prepare(0, 0), PrepareSoilResult::Accepted);

    EXPECT_EQ(soil.prepare(0, 0), PrepareSoilResult::AlreadyPrepared);
    EXPECT_EQ(soil.get_state(0, 0), SoilState::Prepared);
    EXPECT_EQ(soil.get_state(0, 1), SoilState::NotCultivable);
    EXPECT_EQ(soil.get_state(1, 0), SoilState::Unprepared);
}

TEST_F(SoilTest, RejectsNonCultivableCellWithoutChangingIt) {
    EXPECT_EQ(soil.prepare(0, 1), PrepareSoilResult::NotCultivable);

    EXPECT_EQ(soil.get_state(0, 1), SoilState::NotCultivable);
    EXPECT_EQ(soil.get_state(0, 0), SoilState::Unprepared);
}

TEST_F(SoilTest, RejectsPreparationOutsideEitherDimensionWithoutChangingTheSoil) {
    const auto largest = std::numeric_limits<std::size_t>::max();

    EXPECT_EQ(soil.prepare(2, 0), PrepareSoilResult::OutOfBounds);
    EXPECT_EQ(soil.prepare(0, 3), PrepareSoilResult::OutOfBounds);
    EXPECT_EQ(soil.prepare(largest, 0), PrepareSoilResult::OutOfBounds);
    EXPECT_EQ(soil.prepare(0, largest), PrepareSoilResult::OutOfBounds);
    EXPECT_EQ(soil.get_state(0, 0), SoilState::Unprepared);
    EXPECT_EQ(soil.get_state(1, 1), SoilState::Unprepared);
}

TEST_F(SoilTest, RejectsQueriesOutsideEitherDimension) {
    EXPECT_THROW(soil.get_state(2, 0), std::out_of_range);
    EXPECT_THROW(soil.get_state(0, 3), std::out_of_range);
}

TEST_F(SoilTest, PlantsOnlyOnPreparedCultivableSoil) {
    EXPECT_EQ(soil.plant(0, 0, CropType::Strawberry), PlantCropResult::SoilNotPrepared);
    EXPECT_FALSE(soil.get_crop_state(0, 0).has_value());
    EXPECT_EQ(soil.plant(0, 1, CropType::Strawberry), PlantCropResult::NotCultivable);
    EXPECT_FALSE(soil.get_crop_state(0, 1).has_value());

    ASSERT_EQ(soil.prepare(0, 0), PrepareSoilResult::Accepted);
    ASSERT_EQ(soil.plant(0, 0, CropType::Strawberry), PlantCropResult::Accepted);
    const auto crop = soil.get_crop_state(0, 0);
    ASSERT_TRUE(crop.has_value());
    EXPECT_EQ(crop->type, CropType::Strawberry);
    EXPECT_TRUE(crop->needs_water);
    EXPECT_EQ(soil.get_state(0, 0), SoilState::Prepared);
    EXPECT_FALSE(soil.get_crop_state(1, 0).has_value());
}

TEST_F(SoilTest, RejectedPlantingPreservesTheExistingWateredCrop) {
    ASSERT_EQ(soil.plant(0, 2, CropType::Strawberry), PlantCropResult::Accepted);
    ASSERT_EQ(soil.water(0, 2), WaterCropResult::Accepted);

    EXPECT_EQ(soil.plant(0, 2, CropType::Strawberry), PlantCropResult::AlreadyPlanted);
    const auto crop = soil.get_crop_state(0, 2);
    ASSERT_TRUE(crop.has_value());
    EXPECT_FALSE(crop->needs_water);
}

TEST_F(SoilTest, WateringRequiresACropAndClearsItsCurrentNeed) {
    EXPECT_EQ(soil.water(0, 2), WaterCropResult::NoCrop);
    ASSERT_EQ(soil.plant(0, 2, CropType::Strawberry), PlantCropResult::Accepted);

    EXPECT_EQ(soil.water(0, 2), WaterCropResult::Accepted);
    EXPECT_EQ(soil.water(0, 2), WaterCropResult::AlreadyWatered);
    EXPECT_EQ(soil.get_state(0, 2), SoilState::Prepared);
}

TEST_F(SoilTest, RejectsInvalidCropAndOutOfBoundsActions) {
    const auto largest = std::numeric_limits<std::size_t>::max();
    EXPECT_EQ(soil.plant(0, 2, static_cast<CropType>(99)), PlantCropResult::InvalidCrop);
    EXPECT_EQ(soil.plant(2, 0, CropType::Strawberry), PlantCropResult::OutOfBounds);
    EXPECT_EQ(soil.plant(0, 3, CropType::Strawberry), PlantCropResult::OutOfBounds);
    EXPECT_EQ(soil.plant(largest, largest, CropType::Strawberry), PlantCropResult::OutOfBounds);
    EXPECT_EQ(soil.water(2, 0), WaterCropResult::OutOfBounds);
    EXPECT_EQ(soil.water(0, 3), WaterCropResult::OutOfBounds);
    EXPECT_EQ(soil.water(largest, largest), WaterCropResult::OutOfBounds);
    EXPECT_FALSE(soil.get_crop_state(0, 2).has_value());
    EXPECT_EQ(soil.plant(0, 2, CropType::Strawberry), PlantCropResult::Accepted);
}

TEST_F(SoilTest, CropStateIsAnIndependentCopy) {
    ASSERT_EQ(soil.plant(0, 2, CropType::Strawberry), PlantCropResult::Accepted);
    auto copy = soil.get_crop_state(0, 2);
    ASSERT_TRUE(copy.has_value());
    copy->needs_water = false;

    EXPECT_EQ(soil.water(0, 2), WaterCropResult::Accepted);
}

TEST_F(SoilTest, RejectsCropQueriesOutsideEitherDimension) {
    EXPECT_THROW(soil.get_crop_state(2, 0), std::out_of_range);
    EXPECT_THROW(soil.get_crop_state(0, 3), std::out_of_range);
}

}
