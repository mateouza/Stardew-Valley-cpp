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

}
