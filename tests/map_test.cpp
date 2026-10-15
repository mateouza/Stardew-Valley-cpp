#include "map.h"

#include <limits>
#include <stdexcept>
#include <vector>

#include "gtest/gtest.h"

namespace game {

class MapTest: public ::testing::Test {
protected:
    Map map{{{TileType::Walkable, TileType::Walkable, TileType::Walkable},
             {TileType::Walkable, TileType::Blocked, TileType::Walkable},
             {TileType::Walkable, TileType::Walkable, TileType::Walkable}}};
};

TEST(MapConstructionTest, RejectsEmptyMap) { EXPECT_THROW((Map{{}}), std::invalid_argument); }

TEST(MapConstructionTest, RejectsEmptyRow) {
    const std::vector<std::vector<TileType>> tiles(1);

    EXPECT_THROW((Map{tiles}), std::invalid_argument);
}

TEST(MapConstructionTest, RejectsRowsWithDifferentWidths) {
    const std::vector<std::vector<TileType>> tiles = {{TileType::Walkable, TileType::Walkable},
                                                      {TileType::Walkable}};

    EXPECT_THROW((Map{tiles}), std::invalid_argument);
}

TEST(MapConstructionTest, RejectsUnknownTileType) {
    EXPECT_THROW((Map{{{static_cast<TileType>(99)}}}), std::invalid_argument);
}

TEST(MapConstructionTest, OwnsItsTilesIndependentlyOfTheInput) {
    std::vector<std::vector<TileType>> tiles{{TileType::Walkable}};
    const Map map{tiles};

    tiles[0][0] = TileType::Blocked;

    EXPECT_TRUE(map.can_occupy({0.5, 0.5}, 1.0, 1.0));
}

TEST(MapConstructionTest, SupportsRectangularMaps) {
    const Map map{{{TileType::Walkable, TileType::Walkable}}};

    EXPECT_TRUE(map.can_occupy({1.0, 0.5}, 2.0, 1.0));
    EXPECT_FALSE(map.can_occupy({0.5, 1.5}, 1.0, 1.0));
}

TEST_F(MapTest, AcceptsBodyInsideWalkableTile) {
    EXPECT_TRUE(map.can_occupy({0.5, 0.5}, 0.5, 0.5));
}

TEST_F(MapTest, RejectsBodyInsideBlockedTile) {
    EXPECT_FALSE(map.can_occupy({1.5, 1.5}, 0.5, 0.5));
}

TEST_F(MapTest, ChecksBodyBeyondItsCenter) {
    EXPECT_FALSE(map.can_occupy({0.9, 1.5}, 0.4, 0.4));
    EXPECT_FALSE(map.can_occupy({1.5, 0.9}, 0.4, 0.4));
}

TEST_F(MapTest, AllowsContactWithBlockedTileWithoutOverlap) {
    EXPECT_TRUE(map.can_occupy({0.75, 1.5}, 0.5, 0.5));
    EXPECT_TRUE(map.can_occupy({2.25, 1.5}, 0.5, 0.5));
    EXPECT_TRUE(map.can_occupy({1.5, 0.75}, 0.5, 0.5));
    EXPECT_TRUE(map.can_occupy({1.5, 2.25}, 0.5, 0.5));
}

TEST_F(MapTest, AllowsExactContactWithMapEdges) {
    EXPECT_TRUE(map.can_occupy({0.5, 0.5}, 1.0, 1.0));
    EXPECT_TRUE(map.can_occupy({2.5, 0.5}, 1.0, 1.0));
    EXPECT_TRUE(map.can_occupy({0.5, 2.5}, 1.0, 1.0));
    EXPECT_TRUE(map.can_occupy({2.5, 2.5}, 1.0, 1.0));
}

TEST_F(MapTest, RejectsPartialExitThroughAnyMapEdge) {
    EXPECT_FALSE(map.can_occupy({0.25, 0.5}, 1.0, 1.0));
    EXPECT_FALSE(map.can_occupy({2.75, 0.5}, 1.0, 1.0));
    EXPECT_FALSE(map.can_occupy({0.5, 0.25}, 1.0, 1.0));
    EXPECT_FALSE(map.can_occupy({0.5, 2.75}, 1.0, 1.0));
}

TEST_F(MapTest, RejectsCenterOutsideMap) {
    EXPECT_FALSE(map.can_occupy({-1.0, 0.5}, 0.5, 0.5));
    EXPECT_FALSE(map.can_occupy({4.0, 0.5}, 0.5, 0.5));
    EXPECT_FALSE(map.can_occupy({0.5, -1.0}, 0.5, 0.5));
    EXPECT_FALSE(map.can_occupy({0.5, 4.0}, 0.5, 0.5));
}

TEST_F(MapTest, AcceptsBodySpanningSeveralWalkableTiles) {
    EXPECT_TRUE(map.can_occupy({1.5, 0.5}, 3.0, 1.0));
    EXPECT_TRUE(map.can_occupy({0.5, 1.5}, 1.0, 3.0));
}

TEST_F(MapTest, DetectsBlockedTileInsideBodyEvenWithWalkableCorners) {
    EXPECT_FALSE(map.can_occupy({1.5, 1.5}, 3.0, 3.0));
}

TEST_F(MapTest, RejectsBodyLargerThanMap) {
    EXPECT_FALSE(map.can_occupy({1.5, 1.5}, 4.0, 1.0));
    EXPECT_FALSE(map.can_occupy({1.5, 1.5}, 1.0, 4.0));
}

TEST_F(MapTest, RejectsNonPositiveDimensions) {
    EXPECT_FALSE(map.can_occupy({0.5, 0.5}, 0.0, 0.5));
    EXPECT_FALSE(map.can_occupy({0.5, 0.5}, 0.5, 0.0));
    EXPECT_FALSE(map.can_occupy({0.5, 0.5}, -0.5, 0.5));
    EXPECT_FALSE(map.can_occupy({0.5, 0.5}, 0.5, -0.5));
}

TEST_F(MapTest, RejectsNonFiniteCoordinatesAndDimensions) {
    const double invalid_values[] = {std::numeric_limits<double>::quiet_NaN(),
                                     std::numeric_limits<double>::infinity(),
                                     -std::numeric_limits<double>::infinity()};

    for (const double value: invalid_values) {
        EXPECT_FALSE(map.can_occupy({value, 0.5}, 0.5, 0.5));
        EXPECT_FALSE(map.can_occupy({0.5, value}, 0.5, 0.5));
        EXPECT_FALSE(map.can_occupy({0.5, 0.5}, value, 0.5));
        EXPECT_FALSE(map.can_occupy({0.5, 0.5}, 0.5, value));
    }
}

TEST_F(MapTest, RejectsExtremeFiniteValuesBeforeIndexing) {
    const double largest = std::numeric_limits<double>::max();

    EXPECT_FALSE(map.can_occupy({largest, largest}, largest, largest));
    EXPECT_FALSE(map.can_occupy({-largest, -largest}, largest, largest));
}

TEST_F(MapTest, RejectsDimensionsThatCollapseAtFloatingPointPrecision) {
    const double smallest = std::numeric_limits<double>::denorm_min();

    EXPECT_FALSE(map.can_occupy({0.5, 0.5}, smallest, 0.5));
    EXPECT_FALSE(map.can_occupy({0.5, 0.5}, 0.5, smallest));
}

}
