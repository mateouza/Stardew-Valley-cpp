#include "game_loop.h"

#include <chrono>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

#include "gtest/gtest.h"

#include "game_info.h"
#include "tick_scheduler.h"
#include "world.h"

TEST(GameLoopTest, RejectsNonPositiveTickInterval) {
    game::Map map({{game::TileType::Walkable}});
    game::WorldConfig config{1, 1.0, 0.5, 0.5};
    const game::GameInfo info{map.get_data(), config.player_width, config.player_height};
    game::World world(std::move(map), {0.5, 0.5}, config);
    Queue<MatchCommand> commands;

    EXPECT_THROW((GameLoop{world, commands, std::chrono::milliseconds{0}, info}),
                 std::invalid_argument);

    EXPECT_THROW((GameLoop{world, commands, std::chrono::milliseconds{-1}, info}),
                 std::invalid_argument);
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
    Queue<MatchCommand> commands;
    const game::GameInfo info{{1, 1, {game::TileType::Walkable}}, 0.5, 0.5};
    GameLoop game_loop(world, commands, std::chrono::milliseconds{1}, info);

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
    Queue<MatchCommand> commands;
    const game::GameInfo info{{1, 1, {game::TileType::Walkable}}, 0.5, 0.5};
    GameLoop game_loop(world, commands, std::chrono::milliseconds{10}, info);

    game_loop.start();
    game_loop.join();

    EXPECT_EQ(world.processed_commands, 0u);
    EXPECT_EQ(world.updates, 1u);
    EXPECT_DOUBLE_EQ(world.received_elapsed.count(), 0.01);
    EXPECT_THROW(game_loop.rethrow_if_failed(), std::runtime_error);
}

TEST(GameLoopTest, ProcessesAtMost128CommandsBeforeUpdating) {
    RecordingWorld world;
    Queue<MatchCommand> commands(256);
    auto channel = std::make_shared<SessionChannel>(7, 16);
    const game::GameInfo info{{1, 1, {game::TileType::Walkable}}, 0.5, 0.5};

    ASSERT_TRUE(commands.try_push(AddPlayerCommand{"Mateo", channel}));

    const PlayerCommand command{7, game::MoveRequest{game::MovementDirection{1, 0}}};

    for (std::size_t index = 0; index < 128; ++index) {
        ASSERT_TRUE(commands.try_push(command));
    }

    GameLoop game_loop(world, commands, std::chrono::milliseconds{1}, info);

    game_loop.start();
    game_loop.join();

    EXPECT_EQ(world.processed_commands, 127u);
    EXPECT_EQ(world.updates, 1u);

    MatchCommand remaining;
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

TEST(GameLoopTest, RepliesWithPlayerIdWhenAdmissionSucceeds) {
    RecordingWorld world;
    Queue<MatchCommand> commands(16);
    auto channel = std::make_shared<SessionChannel>(7, 16);
    const game::GameInfo info{{1, 1, {game::TileType::Walkable}}, 0.5, 0.5};

    ASSERT_TRUE(commands.try_push(AddPlayerCommand{"Mateo", channel}));

    GameLoop game_loop(world, commands, std::chrono::milliseconds{1}, info);

    game_loop.start();
    game_loop.join();

    ServerMessage message;
    ASSERT_TRUE(channel->get_outgoing_queue().try_pop(message));

    const auto* response = std::get_if<JoinResponse>(&message);
    ASSERT_NE(response, nullptr);

    const auto* player_id = std::get_if<game::PlayerId>(&response->result);
    ASSERT_NE(player_id, nullptr);
    EXPECT_EQ(*player_id, 0u);

    ASSERT_TRUE(channel->get_outgoing_queue().try_pop(message));

    const auto* received_info = std::get_if<game::GameInfo>(&message);
    ASSERT_NE(received_info, nullptr);
    EXPECT_EQ(received_info->map.width, info.map.width);
    EXPECT_EQ(received_info->map.height, info.map.height);
    EXPECT_EQ(received_info->map.tiles, info.map.tiles);
    EXPECT_DOUBLE_EQ(received_info->player_width, info.player_width);
    EXPECT_DOUBLE_EQ(received_info->player_height, info.player_height);

    EXPECT_THROW(channel->get_outgoing_queue().try_pop(message), ClosedQueue);
    EXPECT_TRUE(channel->is_closing());
    EXPECT_TRUE(world.get_state().players.empty());
    EXPECT_THROW(game_loop.rethrow_if_failed(), std::runtime_error);
}

TEST(GameLoopTest, RepliesWithGameFullWhenThereIsNoCapacity) {
    RecordingWorld world;
    Queue<MatchCommand> commands(16);
    auto first_channel = std::make_shared<SessionChannel>(7, 16);
    auto second_channel = std::make_shared<SessionChannel>(8, 16);
    const game::GameInfo info{{1, 1, {game::TileType::Walkable}}, 0.5, 0.5};

    ASSERT_TRUE(commands.try_push(AddPlayerCommand{"Mateo", first_channel}));
    ASSERT_TRUE(commands.try_push(AddPlayerCommand{"Jose", second_channel}));

    GameLoop game_loop(world, commands, std::chrono::milliseconds{1}, info);

    game_loop.start();
    game_loop.join();

    ServerMessage message;
    ASSERT_TRUE(second_channel->get_outgoing_queue().try_pop(message));

    const auto* response = std::get_if<JoinResponse>(&message);
    ASSERT_NE(response, nullptr);

    const auto* error = std::get_if<game::AddPlayerError>(&response->result);
    ASSERT_NE(error, nullptr);
    EXPECT_EQ(*error, game::AddPlayerError::GameFull);

    EXPECT_FALSE(second_channel->get_outgoing_queue().try_pop(message));
    EXPECT_THROW(game_loop.rethrow_if_failed(), std::runtime_error);
}

class PublishingWorld: public game::World {
private:
    std::size_t updates = 0;

public:
    PublishingWorld():
            game::World(game::Map{{{game::TileType::Walkable, game::TileType::Walkable}}},
                        {0.5, 0.5}, game::WorldConfig{1, 1.0, 0.5, 0.5}) {}

    void update_world(std::chrono::duration<double> elapsed) override {
        ++updates;

        if (updates == 2) {
            throw std::runtime_error("Fin de la prueba de publicacion");
        }

        game::World::update_world(elapsed);
    }
};

TEST(GameLoopTest, PublishesStateAfterApplyingMovement) {
    PublishingWorld world;
    Queue<MatchCommand> commands(16);
    auto channel = std::make_shared<SessionChannel>(7, 16);
    const game::GameInfo info{
            {2, 1, {game::TileType::Walkable, game::TileType::Walkable}}, 0.5, 0.5};

    ASSERT_TRUE(commands.try_push(AddPlayerCommand{"Mateo", channel}));
    ASSERT_TRUE(
            commands.try_push(PlayerCommand{7, game::MoveRequest{game::MovementDirection{1, 0}}}));

    GameLoop game_loop(world, commands, std::chrono::milliseconds{20}, info);

    game_loop.start();
    game_loop.join();

    ServerMessage message;
    ASSERT_TRUE(channel->get_outgoing_queue().try_pop(message));
    ASSERT_TRUE(std::holds_alternative<JoinResponse>(message));

    ASSERT_TRUE(channel->get_outgoing_queue().try_pop(message));

    const auto* received_info = std::get_if<game::GameInfo>(&message);
    ASSERT_NE(received_info, nullptr);
    EXPECT_EQ(received_info->map.width, info.map.width);
    EXPECT_EQ(received_info->map.height, info.map.height);
    EXPECT_EQ(received_info->map.tiles, info.map.tiles);
    EXPECT_DOUBLE_EQ(received_info->player_width, info.player_width);
    EXPECT_DOUBLE_EQ(received_info->player_height, info.player_height);

    ASSERT_TRUE(channel->get_outgoing_queue().try_pop(message));

    const auto* state = std::get_if<game::GameState>(&message);
    ASSERT_NE(state, nullptr);
    ASSERT_EQ(state->players.size(), 1u);

    const auto& player = state->players.front();

    EXPECT_EQ(player.id, 0u);
    EXPECT_EQ(player.name, "Mateo");
    EXPECT_DOUBLE_EQ(player.position.x, 0.52);
    EXPECT_DOUBLE_EQ(player.position.y, 0.5);
    EXPECT_EQ(player.facing.horizontal, 1);
    EXPECT_EQ(player.facing.vertical, 0);
    EXPECT_TRUE(player.moving);

    EXPECT_THROW(channel->get_outgoing_queue().try_pop(message), ClosedQueue);
    EXPECT_THROW(game_loop.rethrow_if_failed(), std::runtime_error);
}

class PendingJoinFailureWorld: public game::World {
public:
    PendingJoinFailureWorld():
            game::World(game::Map{{{game::TileType::Walkable}}}, {0.5, 0.5},
                        game::WorldConfig{1, 1.0, 0.5, 0.5}) {}

    void update_world(std::chrono::duration<double>) override {
        throw std::runtime_error("Fallo de prueba con ingreso pendiente");
    }
};

TEST(GameLoopTest, ClosesPendingJoinChannelWhenUpdateFails) {
    PendingJoinFailureWorld world;
    Queue<MatchCommand> commands(256);
    auto channel = std::make_shared<SessionChannel>(7, 16);
    const game::GameInfo info{{1, 1, {game::TileType::Walkable}}, 0.5, 0.5};

    for (std::size_t index = 0; index < 128; ++index) {
        ASSERT_TRUE(commands.try_push(
                PlayerCommand{7, game::MoveRequest{game::MovementDirection{1, 0}}}));
    }

    ASSERT_TRUE(commands.try_push(AddPlayerCommand{"Mateo", channel}));

    GameLoop game_loop(world, commands, std::chrono::milliseconds{1}, info);

    game_loop.start();
    game_loop.join();

    EXPECT_TRUE(channel->is_closing());

    ServerMessage message;
    EXPECT_THROW(channel->get_outgoing_queue().try_pop(message), ClosedQueue);

    MatchCommand pending;
    EXPECT_THROW(commands.try_pop(pending), ClosedQueue);

    try {
        game_loop.rethrow_if_failed();
        ADD_FAILURE() << "No se propago el fallo del modelo";
    } catch (const std::runtime_error& error) {
        EXPECT_STREQ(error.what(), "Fallo de prueba con ingreso pendiente");
    }
}

class FailingAdmissionWorld: public game::World {
public:
    FailingAdmissionWorld():
            game::World(game::Map{{{game::TileType::Walkable}}}, {0.5, 0.5},
                        game::WorldConfig{2, 1.0, 0.5, 0.5}) {}

    game::AddPlayerResult add_player(const std::string&) override {
        throw std::runtime_error("Fallo de ingreso de prueba");
    }
};

TEST(GameLoopTest, ClosesFailedAndPendingJoinChannelsWhenAdmissionThrows) {
    FailingAdmissionWorld world;
    Queue<MatchCommand> commands(16);
    auto failed_channel = std::make_shared<SessionChannel>(7, 16);
    auto pending_channel = std::make_shared<SessionChannel>(8, 16);
    const game::GameInfo info{{1, 1, {game::TileType::Walkable}}, 0.5, 0.5};

    ASSERT_TRUE(commands.try_push(AddPlayerCommand{"Mateo", failed_channel}));
    ASSERT_TRUE(commands.try_push(AddPlayerCommand{"Jose", pending_channel}));

    GameLoop game_loop(world, commands, std::chrono::milliseconds{1}, info);

    game_loop.start();
    game_loop.join();

    EXPECT_TRUE(failed_channel->is_closing());
    EXPECT_TRUE(pending_channel->is_closing());

    ServerMessage message;
    EXPECT_THROW(failed_channel->get_outgoing_queue().try_pop(message), ClosedQueue);
    EXPECT_THROW(pending_channel->get_outgoing_queue().try_pop(message), ClosedQueue);

    MatchCommand remaining;
    EXPECT_THROW(commands.try_pop(remaining), ClosedQueue);

    try {
        game_loop.rethrow_if_failed();
        FAIL() << "No se propago el fallo de ingreso";
    } catch (const std::runtime_error& error) {
        EXPECT_STREQ(error.what(), "Fallo de ingreso de prueba");
    }
}

TEST(GameLoopTest, ClosesSessionWhenGameInfoDoesNotFitInOutgoingQueue) {
    RecordingWorld world;
    Queue<MatchCommand> commands(16);
    auto channel = std::make_shared<SessionChannel>(7, 1);
    const game::GameInfo info{{1, 1, {game::TileType::Walkable}}, 0.5, 0.5};

    ASSERT_TRUE(commands.try_push(AddPlayerCommand{"Mateo", channel}));

    GameLoop game_loop(world, commands, std::chrono::milliseconds{1}, info);

    game_loop.start();
    game_loop.join();

    EXPECT_TRUE(channel->is_closing());
    EXPECT_TRUE(world.get_state().players.empty());

    ServerMessage message;
    ASSERT_TRUE(channel->get_outgoing_queue().try_pop(message));
    ASSERT_TRUE(std::holds_alternative<JoinResponse>(message));

    EXPECT_THROW(channel->get_outgoing_queue().try_pop(message), ClosedQueue);
    EXPECT_THROW(game_loop.rethrow_if_failed(), std::runtime_error);
}
