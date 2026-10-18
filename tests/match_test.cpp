#include "match.h"

#include <chrono>
#include <stdexcept>

#include "gtest/gtest.h"

TEST(MatchTest, StopsAndJoinsOnDestruction) {
    Match match(game::Map{{{game::TileType::Walkable}}}, {0.5, 0.5}, {1, 1.0, 0.5, 0.5}, 16,
                std::chrono::milliseconds{1});

    ASSERT_NO_THROW(match.start());
}

TEST(MatchTest, CleansUpDuringExceptionUnwinding) {
    bool started = false;

    EXPECT_THROW(
            {
                Match match(game::Map{{{game::TileType::Walkable}}}, {0.5, 0.5}, {1, 1.0, 0.5, 0.5},
                            16, std::chrono::milliseconds{1});

                match.start();
                started = true;

                throw std::runtime_error("Error de prueba");
            },
            std::runtime_error);

    EXPECT_TRUE(started);
}

TEST(MatchTest, SupportsRepeatedCleanupAndRejectsRestart) {
    Match match(game::Map{{{game::TileType::Walkable}}}, {0.5, 0.5}, {1, 1.0, 0.5, 0.5}, 16,
                std::chrono::milliseconds{1});

    ASSERT_NO_THROW(match.start());
    EXPECT_THROW(match.start(), std::logic_error);

    match.stop();
    match.join();

    EXPECT_NO_THROW(match.stop());
    EXPECT_NO_THROW(match.join());
    EXPECT_NO_THROW(match.rethrow_if_failed());
    EXPECT_THROW(match.start(), std::logic_error);
}
TEST(MatchTest, CanBeDestroyedWithoutStarting) {
    Match match(game::Map{{{game::TileType::Walkable}}}, {0.5, 0.5}, {1, 1.0, 0.5, 0.5}, 16,
                std::chrono::milliseconds{1});
}

TEST(MatchTest, RejectsFailureCheckBeforeJoin) {
    Match match(game::Map{{{game::TileType::Walkable}}}, {0.5, 0.5}, {1, 1.0, 0.5, 0.5}, 16,
                std::chrono::milliseconds{1});

    match.start();

    EXPECT_THROW(match.rethrow_if_failed(), std::logic_error);

    match.stop();
    match.join();

    EXPECT_NO_THROW(match.rethrow_if_failed());
}
TEST(MatchTest, RejectsCommandsAfterStop) {
    Match match(game::Map{{{game::TileType::Walkable}}}, {0.5, 0.5}, {1, 1.0, 0.5, 0.5}, 16,
                std::chrono::milliseconds{1});

    const PlayerCommand command{0, game::MoveRequest{game::MovementDirection{1, 0}}};

    match.start();
    match.stop();

    EXPECT_THROW(match.get_commands().try_push(command), ClosedQueue);

    match.join();

    EXPECT_THROW(match.get_commands().try_push(command), ClosedQueue);
    EXPECT_NO_THROW(match.stop());
}

TEST(MatchTest, CanStopBeforeStartingAndRejectsLaterStart) {
    Match match(game::Map{{{game::TileType::Walkable}}}, {0.5, 0.5}, {1, 1.0, 0.5, 0.5}, 16,
                std::chrono::milliseconds{1});

    const PlayerCommand command{0, game::MoveRequest{game::MovementDirection{1, 0}}};

    match.stop();

    EXPECT_THROW(match.get_commands().try_push(command), ClosedQueue);
    EXPECT_THROW(match.start(), std::logic_error);
    EXPECT_NO_THROW(match.stop());
    EXPECT_NO_THROW(match.join());
}