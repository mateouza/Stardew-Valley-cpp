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

TEST_F(MapTest, ReachesFreeDestination) {
    const auto result = map.resolve_movement({0.5, 0.5}, {2.5, 0.5}, 0.5, 0.5);

    EXPECT_DOUBLE_EQ(result.x, 2.5);
    EXPECT_DOUBLE_EQ(result.y, 0.5);
}

TEST_F(MapTest, UnchangedDestinationPreservesPosition) {
    const auto result = map.resolve_movement({0.5, 0.5}, {0.5, 0.5}, 0.5, 0.5);

    EXPECT_DOUBLE_EQ(result.x, 0.5);
    EXPECT_DOUBLE_EQ(result.y, 0.5);
}

TEST_F(MapTest, StopsAtWallWhenMovingRight) {
    const auto result = map.resolve_movement({0.5, 1.5}, {2.5, 1.5}, 0.5, 0.5);

    EXPECT_DOUBLE_EQ(result.x, 0.75);
    EXPECT_DOUBLE_EQ(result.y, 1.5);
}

TEST_F(MapTest, StopsAtWallWhenMovingLeft) {
    const auto result = map.resolve_movement({2.5, 1.5}, {0.5, 1.5}, 0.5, 0.5);

    EXPECT_DOUBLE_EQ(result.x, 2.25);
    EXPECT_DOUBLE_EQ(result.y, 1.5);
}

TEST_F(MapTest, StopsAtWallWhenMovingDown) {
    const auto result = map.resolve_movement({1.5, 0.5}, {1.5, 2.5}, 0.5, 0.5);

    EXPECT_DOUBLE_EQ(result.x, 1.5);
    EXPECT_DOUBLE_EQ(result.y, 0.75);
}

TEST_F(MapTest, StopsAtWallWhenMovingUp) {
    const auto result = map.resolve_movement({1.5, 2.5}, {1.5, 0.5}, 0.5, 0.5);

    EXPECT_DOUBLE_EQ(result.x, 1.5);
    EXPECT_DOUBLE_EQ(result.y, 2.25);
}

TEST_F(MapTest, ClampsMovementToMapEdges) {
    const auto upper_left = map.resolve_movement({0.5, 0.5}, {-100.0, -100.0}, 0.5, 0.5);
    const auto lower_right = map.resolve_movement({2.5, 2.5}, {100.0, 100.0}, 0.5, 0.5);

    EXPECT_DOUBLE_EQ(upper_left.x, 0.25);
    EXPECT_DOUBLE_EQ(upper_left.y, 0.25);
    EXPECT_DOUBLE_EQ(lower_right.x, 2.75);
    EXPECT_DOUBLE_EQ(lower_right.y, 2.75);
}

TEST_F(MapTest, SlidesVerticallyWhenHorizontalMovementIsBlocked) {
    const auto result = map.resolve_movement({0.5, 1.5}, {2.5, 2.5}, 0.5, 0.5);

    EXPECT_DOUBLE_EQ(result.x, 0.75);
    EXPECT_DOUBLE_EQ(result.y, 2.5);
}

TEST_F(MapTest, ResolvesHorizontalMovementBeforeVerticalMovement) {
    const auto result = map.resolve_movement({0.5, 0.5}, {1.5, 2.5}, 0.5, 0.5);

    EXPECT_DOUBLE_EQ(result.x, 1.5);
    EXPECT_DOUBLE_EQ(result.y, 0.75);
}

TEST_F(MapTest, CanMoveAwayFromWallContact) {
    const auto result = map.resolve_movement({0.75, 1.5}, {0.25, 1.5}, 0.5, 0.5);

    EXPECT_DOUBLE_EQ(result.x, 0.25);
    EXPECT_DOUBLE_EQ(result.y, 1.5);
}

TEST_F(MapTest, ChecksFullBodyHeightDuringHorizontalMovement) {
    const auto result = map.resolve_movement({0.5, 0.75}, {2.5, 0.75}, 0.5, 1.0);

    EXPECT_DOUBLE_EQ(result.x, 0.75);
    EXPECT_DOUBLE_EQ(result.y, 0.75);
}

TEST_F(MapTest, ChecksFullBodyWidthDuringVerticalMovement) {
    const auto result = map.resolve_movement({0.75, 0.5}, {0.75, 2.5}, 1.0, 0.5);

    EXPECT_DOUBLE_EQ(result.x, 0.75);
    EXPECT_DOUBLE_EQ(result.y, 0.75);
}

TEST_F(MapTest, FiniteExtremeDestinationDoesNotSkipWall) {
    const auto result =
            map.resolve_movement({0.5, 1.5}, {std::numeric_limits<double>::max(), 1.5}, 0.5, 0.5);

    EXPECT_DOUBLE_EQ(result.x, 0.75);
    EXPECT_DOUBLE_EQ(result.y, 1.5);
}

TEST_F(MapTest, DecimalBodySizesKeepWallContactsValidAndStable) {
    const double widths[] = {0.2, 0.3, 0.6, 0.7, 0.9};
    for (const double width: widths) {
        const auto left = map.resolve_movement({0.5, 1.5}, {2.5, 1.5}, width, 0.5);
        const auto right = map.resolve_movement({2.5, 1.5}, {0.5, 1.5}, width, 0.5);

        EXPECT_DOUBLE_EQ(left.x, 1.0 - width / 2.0);
        EXPECT_DOUBLE_EQ(right.x, 2.0 + width / 2.0);
        EXPECT_TRUE(map.can_occupy(left, width, 0.5));
        EXPECT_TRUE(map.can_occupy(right, width, 0.5));
        EXPECT_DOUBLE_EQ(map.resolve_movement(left, {2.5, 1.5}, width, 0.5).x, left.x);
        EXPECT_DOUBLE_EQ(map.resolve_movement(right, {0.5, 1.5}, width, 0.5).x, right.x);

        const auto top = map.resolve_movement({1.5, 0.5}, {1.5, 2.5}, 0.5, width);
        const auto bottom = map.resolve_movement({1.5, 2.5}, {1.5, 0.5}, 0.5, width);

        EXPECT_DOUBLE_EQ(top.y, 1.0 - width / 2.0);
        EXPECT_DOUBLE_EQ(bottom.y, 2.0 + width / 2.0);
        EXPECT_TRUE(map.can_occupy(top, 0.5, width));
        EXPECT_TRUE(map.can_occupy(bottom, 0.5, width));
    }
}

TEST_F(MapTest, RejectsInvalidInitialPosition) {
    EXPECT_THROW(map.resolve_movement({1.5, 1.5}, {0.5, 0.5}, 0.5, 0.5), std::invalid_argument);
    EXPECT_THROW(map.resolve_movement({-1.0, 0.5}, {0.5, 0.5}, 0.5, 0.5), std::invalid_argument);
}

TEST_F(MapTest, RejectsInvalidBodyDimensionsForMovement) {
    EXPECT_THROW(map.resolve_movement({0.5, 0.5}, {0.75, 0.5}, 0.0, 0.5), std::invalid_argument);
    EXPECT_THROW(map.resolve_movement({0.5, 0.5}, {0.75, 0.5}, 0.5, -1.0), std::invalid_argument);
}

TEST_F(MapTest, RejectsNonFiniteMovementDestination) {
    const double invalid_values[] = {std::numeric_limits<double>::quiet_NaN(),
                                     std::numeric_limits<double>::infinity(),
                                     -std::numeric_limits<double>::infinity()};
    for (const double value: invalid_values) {
        EXPECT_THROW(map.resolve_movement({0.5, 0.5}, {value, 0.5}, 0.5, 0.5),
                     std::invalid_argument);
        EXPECT_THROW(map.resolve_movement({0.5, 0.5}, {0.5, value}, 0.5, 0.5),
                     std::invalid_argument);
    }
}

TEST(MapMovementTest, StopsAtNearestWallInEitherDirection) {
    const Map map{{{TileType::Walkable, TileType::Blocked, TileType::Walkable, TileType::Blocked,
                    TileType::Walkable}}};

    const auto right = map.resolve_movement({0.5, 0.5}, {4.5, 0.5}, 0.5, 0.5);
    const auto left = map.resolve_movement({4.5, 0.5}, {0.5, 0.5}, 0.5, 0.5);

    EXPECT_DOUBLE_EQ(right.x, 0.75);
    EXPECT_DOUBLE_EQ(left.x, 4.25);
}

TEST(MapMovementTest, CannotPassBetweenTwoBlockedCornerTiles) {
    const Map map{
            {{TileType::Walkable, TileType::Blocked}, {TileType::Blocked, TileType::Walkable}}};

    const auto result = map.resolve_movement({0.5, 0.5}, {1.5, 1.5}, 0.5, 0.5);

    EXPECT_DOUBLE_EQ(result.x, 0.75);
    EXPECT_DOUBLE_EQ(result.y, 0.75);
    EXPECT_TRUE(map.can_occupy(result, 0.5, 0.5));
}

TEST(MapMovementTest, RespectsDifferentMapWidthAndHeight) {
    const Map map{{{TileType::Walkable, TileType::Walkable, TileType::Walkable}}};

    const auto result = map.resolve_movement({0.5, 0.5}, {100.0, 100.0}, 0.5, 0.5);

    EXPECT_DOUBLE_EQ(result.x, 2.75);
    EXPECT_DOUBLE_EQ(result.y, 0.75);
}

TEST(MapMovementTest, DecimalContactDoesNotOverlapWallThroughRounding) {
    const Map map{{{TileType::Blocked, TileType::Walkable}}};

    const auto result = map.resolve_movement({1.5, 0.5}, {0.5, 0.5}, 0.8, 0.5);

    EXPECT_DOUBLE_EQ(result.x, 1.4);
    EXPECT_GE(result.x - 0.4, 1.0);
    EXPECT_TRUE(map.can_occupy(result, 0.8, 0.5));
    EXPECT_DOUBLE_EQ(map.resolve_movement(result, {0.5, 0.5}, 0.8, 0.5).x, result.x);
}

TEST(MapMovementTest, CannotCrossCornerMovingDownLeftWithDecimalBody) {
    const Map map{
            {{TileType::Blocked, TileType::Walkable}, {TileType::Walkable, TileType::Blocked}}};

    const auto result = map.resolve_movement({1.5, 0.5}, {0.5, 1.5}, 0.6, 0.6);

    EXPECT_DOUBLE_EQ(result.x, 1.3);
    EXPECT_DOUBLE_EQ(result.y, 0.7);
    EXPECT_TRUE(map.can_occupy(result, 0.6, 0.6));
}

TEST(MapMovementTest, CannotCrossCornerMovingUpLeftWithDecimalBody) {
    const Map map{
            {{TileType::Walkable, TileType::Blocked}, {TileType::Blocked, TileType::Walkable}}};

    const auto result = map.resolve_movement({1.5, 1.5}, {0.5, 0.5}, 0.6, 0.6);

    EXPECT_DOUBLE_EQ(result.x, 1.3);
    EXPECT_DOUBLE_EQ(result.y, 1.3);
    EXPECT_TRUE(map.can_occupy(result, 0.6, 0.6));
}

TEST(MapMovementTest, CannotCrossCornerMovingUpRightWithDecimalBody) {
    const Map map{
            {{TileType::Blocked, TileType::Walkable}, {TileType::Walkable, TileType::Blocked}}};

    const auto result = map.resolve_movement({0.5, 1.5}, {1.5, 0.5}, 0.6, 0.6);

    EXPECT_DOUBLE_EQ(result.x, 0.7);
    EXPECT_DOUBLE_EQ(result.y, 1.3);
    EXPECT_TRUE(map.can_occupy(result, 0.6, 0.6));
}

TEST(MapMovementTest, WallContactRemainsValidAcrossTinySlidingSteps) {
    const Map map{{{TileType::Blocked, TileType::Walkable}}};
    auto position = map.resolve_movement({1.5, 0.5}, {0.5, 0.5}, 0.8, 0.6);
    const double initial_x = position.x;

    for (int step = 0; step < 1000; ++step) {
        position = map.resolve_movement(position, {position.x - 0.00001, position.y + 0.0001}, 0.8,
                                        0.6);
        ASSERT_TRUE(map.can_occupy(position, 0.8, 0.6));
    }

    EXPECT_DOUBLE_EQ(position.x, initial_x);
    EXPECT_NEAR(position.y, 0.6, 1e-12);
}

TEST(MapMovementTest, RepeatedTinyFreeMovementsDoNotGetCancelled) {
    const Map map{{{TileType::Walkable, TileType::Walkable, TileType::Walkable}}};
    Position position{0.5, 0.5};

    for (int step = 0; step < 1000; ++step) {
        position = map.resolve_movement(position, {position.x + 0.00001, position.y}, 0.6, 0.6);
        ASSERT_TRUE(map.can_occupy(position, 0.6, 0.6));
    }

    EXPECT_NEAR(position.x, 0.51, 1e-12);
    EXPECT_DOUBLE_EQ(position.y, 0.5);
}

TEST_F(MapTest, DecimalBodyRemainsStableAtBothMapCorners) {
    auto upper_left = map.resolve_movement({0.5, 0.5}, {-100.0, -100.0}, 0.6, 0.6);
    auto lower_right = map.resolve_movement({2.5, 2.5}, {100.0, 100.0}, 0.6, 0.6);

    for (int step = 0; step < 1000; ++step) {
        upper_left = map.resolve_movement(upper_left,
                                          {upper_left.x - 0.0001, upper_left.y - 0.0001}, 0.6, 0.6);
        lower_right = map.resolve_movement(
                lower_right, {lower_right.x + 0.0001, lower_right.y + 0.0001}, 0.6, 0.6);
        ASSERT_TRUE(map.can_occupy(upper_left, 0.6, 0.6));
        ASSERT_TRUE(map.can_occupy(lower_right, 0.6, 0.6));
    }

    EXPECT_DOUBLE_EQ(upper_left.x, 0.3);
    EXPECT_DOUBLE_EQ(upper_left.y, 0.3);
    EXPECT_DOUBLE_EQ(lower_right.x, 2.7);
    EXPECT_DOUBLE_EQ(lower_right.y, 2.7);
}

}

