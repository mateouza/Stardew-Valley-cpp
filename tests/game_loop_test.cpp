#include "game_loop.h"

#include <chrono>
#include <cstddef>
#include <stdexcept>
#include <utility>

#include "gtest/gtest.h"

#include "tick_scheduler.h"
#include "world.h"

TEST(GameLoopTest, RejectsNonPositiveTickInterval) {
    game::Map map({{game::TileType::Walkable}});
    game::WorldConfig config{1, 1.0, 0.5, 0.5};
    game::World world(std::move(map), {0.5, 0.5}, config);
    Queue<PlayerCommand> commands;

    EXPECT_THROW((GameLoop{world, commands, std::chrono::milliseconds{0}}), std::invalid_argument);

    EXPECT_THROW((GameLoop{world, commands, std::chrono::milliseconds{-1}}), std::invalid_argument);
}

class FailingWorld: public game::World {
public:
    FailingWorld():
            game::World(game::Map{{{game::TileType::Walkable}}}, {0.5, 0.5},
                        game::WorldConfig{1, 1.0, 0.5, 0.5}) {}

    void update_world(std::chrono::duration<double>) override {
        throw std::runtime_error("Fallo de actualizacion de prueba");
    }
};

TEST(GameLoopTest, ClosesCommandsAndPreservesFailureWhenUpdateThrows) {
    FailingWorld world;
    Queue<PlayerCommand> commands;
    GameLoop game_loop(world, commands, std::chrono::milliseconds{1});

    game_loop.start();
    game_loop.join();

    const PlayerCommand command{0, game::MoveRequest{game::MovementDirection{1, 0}}};

    EXPECT_THROW(commands.try_push(command), ClosedQueue);
    EXPECT_THROW(game_loop.rethrow_if_failed(), std::runtime_error);

    EXPECT_NO_THROW(game_loop.stop());
    EXPECT_NO_THROW(game_loop.stop());
}

class RecordingWorld: public game::World {
public:
    std::size_t processed_commands = 0;
    std::size_t updates = 0;
    std::chrono::duration<double> received_elapsed{0.0};

    RecordingWorld():
            game::World(game::Map{{{game::TileType::Walkable}}}, {0.5, 0.5},
                        game::WorldConfig{1, 1.0, 0.5, 0.5}) {}

    game::SetDirectionResult set_direction(game::PlayerId, game::MovementDirection) override {
        ++processed_commands;
        return game::SetDirectionResult::Accepted;
    }

    void update_world(std::chrono::duration<double> elapsed) override {
        ++updates;
        received_elapsed = elapsed;
        throw std::runtime_error("Fin de la primera actualizacion de prueba");
    }
};

TEST(GameLoopTest, UpdatesWithoutCommandsUsingConfiguredInterval) {
    RecordingWorld world;
    Queue<PlayerCommand> commands;
    GameLoop game_loop(world, commands, std::chrono::milliseconds{10});

    game_loop.start();
    game_loop.join();

    EXPECT_EQ(world.processed_commands, 0u);
    EXPECT_EQ(world.updates, 1u);
    EXPECT_DOUBLE_EQ(world.received_elapsed.count(), 0.01);
    EXPECT_THROW(game_loop.rethrow_if_failed(), std::runtime_error);
}

TEST(GameLoopTest, ProcessesAtMost128CommandsBeforeUpdating) {
    RecordingWorld world;
    Queue<PlayerCommand> commands(256);
    const PlayerCommand command{0, game::MoveRequest{game::MovementDirection{1, 0}}};

    for (std::size_t index = 0; index < 129; ++index) {
        ASSERT_TRUE(commands.try_push(command));
    }

    GameLoop game_loop(world, commands, std::chrono::milliseconds{1});

    game_loop.start();
    game_loop.join();

    EXPECT_EQ(world.processed_commands, 128u);
    EXPECT_EQ(world.updates, 1u);

    PlayerCommand remaining{};
    ASSERT_TRUE(commands.try_pop(remaining));
    EXPECT_EQ(remaining.player_id, command.player_id);
    EXPECT_EQ(remaining.request.direction.horizontal, 1);
    EXPECT_EQ(remaining.request.direction.vertical, 0);
    EXPECT_THROW(commands.try_pop(remaining), ClosedQueue);
    EXPECT_THROW(game_loop.rethrow_if_failed(), std::runtime_error);
}

TEST(TickSchedulerTest, RejectsNonPositiveInterval) {
    EXPECT_THROW((TickScheduler{std::chrono::milliseconds{0}}), std::invalid_argument);
    EXPECT_THROW((TickScheduler{std::chrono::milliseconds{-1}}), std::invalid_argument);
}

TEST(TickSchedulerTest, WaitsUntilACompleteTickIsAccumulated) {
    TickScheduler scheduler(std::chrono::milliseconds{20});

    EXPECT_EQ(scheduler.get_wait_time(), std::chrono::milliseconds{20});

    EXPECT_EQ(scheduler.advance(std::chrono::milliseconds{15}), 0u);
    EXPECT_EQ(scheduler.get_wait_time(), std::chrono::milliseconds{5});

    EXPECT_EQ(scheduler.advance(std::chrono::milliseconds{5}), 1u);
    EXPECT_EQ(scheduler.get_wait_time(), std::chrono::milliseconds{20});
}

TEST(TickSchedulerTest, RecoversSeveralStepsAndPreservesRemainder) {
    TickScheduler scheduler(std::chrono::milliseconds{20});

    EXPECT_EQ(scheduler.advance(std::chrono::milliseconds{45}), 2u);
    EXPECT_EQ(scheduler.get_wait_time(), std::chrono::milliseconds{15});

    EXPECT_EQ(scheduler.advance(std::chrono::milliseconds{15}), 1u);
    EXPECT_EQ(scheduler.get_wait_time(), std::chrono::milliseconds{20});
}

TEST(TickSchedulerTest, LimitsRecoveryAndDiscardsOnlyCompleteExcessTicks) {
    TickScheduler scheduler(std::chrono::milliseconds{20});

    EXPECT_EQ(scheduler.advance(std::chrono::milliseconds{147}), 5u);
    EXPECT_EQ(scheduler.get_wait_time(), std::chrono::milliseconds{13});

    EXPECT_EQ(scheduler.advance(std::chrono::milliseconds{0}), 0u);
    EXPECT_EQ(scheduler.get_wait_time(), std::chrono::milliseconds{13});

    EXPECT_EQ(scheduler.advance(std::chrono::milliseconds{13}), 1u);
    EXPECT_EQ(scheduler.get_wait_time(), std::chrono::milliseconds{20});
}

TEST(TickSchedulerTest, PreservesFractionsSmallerThanOneMillisecond) {
    TickScheduler scheduler(std::chrono::milliseconds{20});

    EXPECT_EQ(scheduler.advance(std::chrono::microseconds{19900}), 0u);
    EXPECT_EQ(scheduler.get_wait_time(), std::chrono::microseconds{100});

    EXPECT_EQ(scheduler.advance(std::chrono::microseconds{100}), 1u);
    EXPECT_EQ(scheduler.get_wait_time(), std::chrono::milliseconds{20});
}

TEST(TickSchedulerTest, RejectsNegativeElapsedWithoutChangingPendingTime) {
    TickScheduler scheduler(std::chrono::milliseconds{20});

    EXPECT_EQ(scheduler.advance(std::chrono::milliseconds{15}), 0u);

    EXPECT_THROW(scheduler.advance(std::chrono::milliseconds{-1}), std::invalid_argument);

    EXPECT_EQ(scheduler.get_wait_time(), std::chrono::milliseconds{5});
    EXPECT_EQ(scheduler.advance(std::chrono::milliseconds{5}), 1u);
}

TEST(TickSchedulerTest, RejectsIntervalOutsideClockRange) {
    EXPECT_THROW((TickScheduler{std::chrono::milliseconds::max()}), std::invalid_argument);
}

TEST(TickSchedulerTest, AcceptsLargestWholeMillisecondIntervalWithinClockRange) {
    const auto max_interval = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::duration::max());

    TickScheduler scheduler(max_interval);

    EXPECT_EQ(scheduler.advance(std::chrono::milliseconds{0}), 0u);
    EXPECT_EQ(scheduler.get_wait_time(),
              std::chrono::duration_cast<std::chrono::steady_clock::duration>(max_interval));
}

TEST(TickSchedulerTest, RejectsAccumulationOverflowWithoutChangingPendingTime) {
    TickScheduler scheduler(std::chrono::milliseconds{20});

    EXPECT_EQ(scheduler.advance(std::chrono::milliseconds{15}), 0u);

    EXPECT_THROW(scheduler.advance(std::chrono::steady_clock::duration::max()),
                 std::overflow_error);

    EXPECT_EQ(scheduler.get_wait_time(), std::chrono::milliseconds{5});
    EXPECT_EQ(scheduler.advance(std::chrono::milliseconds{5}), 1u);
}
