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

    ASSERT_TRUE(channel->get_outgoing_queue().try_pop(message));
    ASSERT_TRUE(std::holds_alternative<game::InventoryState>(message));
    EXPECT_THROW(channel->get_outgoing_queue().try_pop(message), ClosedQueue);
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
    explicit PublishingWorld(std::size_t max_players = 1,
                         std::optional<game::Soil> soil = std::nullopt):
        game::World(game::Map{{{game::TileType::Walkable, game::TileType::Walkable}}},
                    {0.5, 0.5}, game::WorldConfig{max_players, 1.0, 0.5, 0.5},
                    std::move(soil)) {}

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
    ASSERT_TRUE(std::holds_alternative<game::InventoryState>(message));
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

TEST(GameLoopTest, SendsUpdatedInventoryOnlyToSelectingSession) {
    PublishingWorld world{2};
    Queue<MatchCommand> commands(16);
    auto first = std::make_shared<SessionChannel>(7, 16);
    auto second = std::make_shared<SessionChannel>(8, 16);
    const game::GameInfo info{
            {2, 1, {game::TileType::Walkable, game::TileType::Walkable}}, 0.5, 0.5};

    ASSERT_TRUE(commands.try_push(AddPlayerCommand{"Mateo", first}));
    ASSERT_TRUE(commands.try_push(AddPlayerCommand{"Jose", second}));
    ASSERT_TRUE(commands.try_push(SelectSlotCommand{7, game::SelectSlotRequest{2}}));

    GameLoop game_loop(world, commands, std::chrono::milliseconds{20}, info);
    game_loop.start();
    game_loop.join();

    for (const auto& channel: {first, second}) {
        auto& outgoing = channel->get_outgoing_queue();
        ServerMessage message;

        ASSERT_TRUE(outgoing.try_pop(message));
        const auto* join = std::get_if<JoinResponse>(&message);
        ASSERT_NE(join, nullptr);
        ASSERT_TRUE(std::holds_alternative<game::PlayerId>(join->result));

        ASSERT_TRUE(outgoing.try_pop(message));
        ASSERT_TRUE(std::holds_alternative<game::GameInfo>(message));

        ASSERT_TRUE(outgoing.try_pop(message));
        const auto* initial_inventory = std::get_if<game::InventoryState>(&message);
        ASSERT_NE(initial_inventory, nullptr);
        EXPECT_EQ(initial_inventory->selected_slot, 0u);
    }

    ServerMessage message;
    ASSERT_TRUE(first->get_outgoing_queue().try_pop(message));
    const auto* updated_inventory = std::get_if<game::InventoryState>(&message);
    ASSERT_NE(updated_inventory, nullptr);
    EXPECT_EQ(updated_inventory->selected_slot, 2u);

    auto& second_outgoing = second->get_outgoing_queue();
    ASSERT_TRUE(second_outgoing.try_pop(message));
    ASSERT_TRUE(std::holds_alternative<game::GameState>(message));
    EXPECT_THROW(second_outgoing.try_pop(message), ClosedQueue);
}

TEST(GameLoopTest, PublishesPreparedSoilAndRepliesToRejectedUseOnlyToOwner) {
    PublishingWorld world{2, game::Soil{{{game::SoilState::Unprepared,
                                        game::SoilState::Unprepared}}}};
    Queue<MatchCommand> commands(16);
    auto first = std::make_shared<SessionChannel>(7, 16);
    auto second = std::make_shared<SessionChannel>(8, 16);
    const game::GameInfo info{
            {2, 1, {game::TileType::Walkable, game::TileType::Walkable}}, 0.5, 0.5};

    ASSERT_TRUE(commands.try_push(AddPlayerCommand{"Mateo", first}));
    ASSERT_TRUE(commands.try_push(AddPlayerCommand{"Jose", second}));
    ASSERT_TRUE(commands.try_push(SelectSlotCommand{7, game::SelectSlotRequest{2}}));
    ASSERT_TRUE(commands.try_push(UseCellCommand{7, game::UseCellRequest{{0, 1}}}));
    ASSERT_TRUE(commands.try_push(UseCellCommand{7, game::UseCellRequest{{0, 1}}}));

    GameLoop game_loop(world, commands, std::chrono::milliseconds{20}, info);
    game_loop.start();
    game_loop.join();

    for (const auto& channel: {first, second}) {
        std::size_t soil_messages = 0;
        std::size_t responses = 0;
        ServerMessage message;

        try {
            while (channel->get_outgoing_queue().try_pop(message)) {
                if (const auto* soil = std::get_if<game::SoilData>(&message)) {
                    ++soil_messages;
                    EXPECT_EQ(soil->width, 2u);
                    EXPECT_EQ(soil->height, 1u);
                    ASSERT_EQ(soil->cells.size(), 2u);
                    EXPECT_EQ(soil->cells[0], game::SoilState::Unprepared);

                    const auto expected = soil_messages == 1 ? game::SoilState::Unprepared :
                                                            game::SoilState::Prepared;
                    EXPECT_EQ(soil->cells[1], expected);
                }

                if (const auto* response = std::get_if<UseItemResponse>(&message)) {
                    ASSERT_EQ(channel->get_session_id(), 7u);
                    const auto* result =
                            std::get_if<game::PrepareSoilResult>(&response->result);
                    ASSERT_NE(result, nullptr);

                    if (responses == 0) {
                        EXPECT_EQ(*result, game::PrepareSoilResult::Accepted);
                    } else {
                        EXPECT_EQ(*result, game::PrepareSoilResult::AlreadyPrepared);
                    }

                    ++responses;
                }
            }
        } catch (const ClosedQueue&) {
        }

        EXPECT_EQ(soil_messages, 2u);
        EXPECT_EQ(responses, channel->get_session_id() == 7 ? 2u : 0u);
    }

    EXPECT_THROW(game_loop.rethrow_if_failed(), std::runtime_error);
}

class FarmingWorld: public PublishingWorld {
public:
    FarmingWorld():
            PublishingWorld{2, game::Soil{{{game::SoilState::Unprepared,
                                           game::SoilState::Unprepared}}}} {}

    game::AddPlayerResult add_player(const std::string& name) override {
        const auto result = PublishingWorld::add_player(name);
        if (const auto* id = std::get_if<game::PlayerId>(&result)) {
            if (add_item(*id, {game::ItemType::StrawberrySeed, game::ItemQuality::Base, 2}) !=
                game::InventoryResult::Accepted) {
                throw std::logic_error("No se pudieron agregar semillas para la prueba");
            }
        }
        return result;
    }
};

TEST(GameLoopTest, PlantsWatersAndSendsCurrentFarmToLateJoiningPlayer) {
    FarmingWorld world;
    Queue<MatchCommand> commands(16);
    auto first = std::make_shared<SessionChannel>(7, 32);
    auto second = std::make_shared<SessionChannel>(8, 32);
    const game::GameInfo info{
            {2, 1, {game::TileType::Walkable, game::TileType::Walkable}}, 0.5, 0.5};

    ASSERT_TRUE(commands.try_push(AddPlayerCommand{"Mateo", first}));
    ASSERT_TRUE(commands.try_push(SelectSlotCommand{7, game::SelectSlotRequest{2}}));
    ASSERT_TRUE(commands.try_push(UseCellCommand{7, game::UseCellRequest{{0, 1}}}));
    ASSERT_TRUE(commands.try_push(SelectSlotCommand{7, game::SelectSlotRequest{4}}));
    ASSERT_TRUE(commands.try_push(UseCellCommand{7, game::UseCellRequest{{0, 1}}}));
    ASSERT_TRUE(commands.try_push(SelectSlotCommand{7, game::SelectSlotRequest{1}}));
    ASSERT_TRUE(commands.try_push(UseCellCommand{7, game::UseCellRequest{{0, 1}}}));
    ASSERT_TRUE(commands.try_push(AddPlayerCommand{"Jose", second}));

    GameLoop game_loop(world, commands, std::chrono::milliseconds{20}, info);
    game_loop.start();
    game_loop.join();

    std::size_t responses = 0;
    std::size_t crop_snapshots = 0;
    game::InventoryState last_inventory;
    ServerMessage message;

    try {
        while (first->get_outgoing_queue().try_pop(message)) {
            if (const auto* inventory = std::get_if<game::InventoryState>(&message)) {
                last_inventory = *inventory;
            }

            if (const auto* response = std::get_if<UseItemResponse>(&message)) {
                if (responses == 0) {
                    const auto* result =
                            std::get_if<game::PrepareSoilResult>(&response->result);
                    ASSERT_NE(result, nullptr);
                    EXPECT_EQ(*result, game::PrepareSoilResult::Accepted);
                } else if (responses == 1) {
                    const auto* result =
                            std::get_if<game::PlantCropResult>(&response->result);
                    ASSERT_NE(result, nullptr);
                    EXPECT_EQ(*result, game::PlantCropResult::Accepted);
                } else {
                    const auto* result =
                            std::get_if<game::WaterCropResult>(&response->result);
                    ASSERT_NE(result, nullptr);
                    EXPECT_EQ(*result, game::WaterCropResult::Accepted);
                }
                ++responses;
            }

            if (const auto* crops = std::get_if<game::CropsState>(&message)) {
                if (crop_snapshots == 0) {
                    EXPECT_TRUE(crops->crops.empty());
                } else {
                    ASSERT_EQ(crops->crops.size(), 1u);
                    EXPECT_EQ(crops->crops[0].cell.row, 0u);
                    EXPECT_EQ(crops->crops[0].cell.column, 1u);
                    EXPECT_EQ(crops->crops[0].state.type, game::CropType::Strawberry);
                    EXPECT_EQ(crops->crops[0].state.needs_water, crop_snapshots == 1);
                }
                ++crop_snapshots;
            }
        }
    } catch (const ClosedQueue&) {
    }

    EXPECT_EQ(responses, 3u);
    EXPECT_EQ(crop_snapshots, 3u);
    ASSERT_TRUE(last_inventory.items[4].has_value());
    EXPECT_EQ(last_inventory.items[4]->quantity, 1u);
    ASSERT_TRUE(last_inventory.items[1].has_value());
    EXPECT_EQ(last_inventory.items[1]->quantity, 1u);

    auto& outgoing = second->get_outgoing_queue();

    ASSERT_TRUE(outgoing.try_pop(message));
    ASSERT_TRUE(std::holds_alternative<JoinResponse>(message));

    ASSERT_TRUE(outgoing.try_pop(message));
    ASSERT_TRUE(std::holds_alternative<game::GameInfo>(message));

    ASSERT_TRUE(outgoing.try_pop(message));
    const auto* inventory = std::get_if<game::InventoryState>(&message);
    ASSERT_NE(inventory, nullptr);
    ASSERT_TRUE(inventory->items[4].has_value());
    EXPECT_EQ(inventory->items[4]->quantity, 2u);
    EXPECT_EQ(inventory->selected_slot, 0u);

    ASSERT_TRUE(outgoing.try_pop(message));
    const auto* soil = std::get_if<game::SoilData>(&message);
    ASSERT_NE(soil, nullptr);
    ASSERT_EQ(soil->cells.size(), 2u);
    EXPECT_EQ(soil->cells[1], game::SoilState::Prepared);

    ASSERT_TRUE(outgoing.try_pop(message));
    const auto* crops = std::get_if<game::CropsState>(&message);
    ASSERT_NE(crops, nullptr);
    ASSERT_EQ(crops->crops.size(), 1u);
    EXPECT_EQ(crops->crops[0].cell.row, 0u);
    EXPECT_EQ(crops->crops[0].cell.column, 1u);
    EXPECT_EQ(crops->crops[0].state.type, game::CropType::Strawberry);
    EXPECT_FALSE(crops->crops[0].state.needs_water);

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

TEST(GameLoopTest, OwnsGameInfoPassedAsTemporary) {
    RecordingWorld world;
    Queue<MatchCommand> commands(16);
    auto channel = std::make_shared<SessionChannel>(7, 16);

    ASSERT_TRUE(commands.try_push(AddPlayerCommand{"Mateo", channel}));

    GameLoop game_loop(world, commands, std::chrono::milliseconds{1},
                       game::GameInfo{{1, 1, {game::TileType::Walkable}}, 0.5, 0.5});

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

    const auto* info = std::get_if<game::GameInfo>(&message);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->map.width, 1u);
    EXPECT_EQ(info->map.height, 1u);
    ASSERT_EQ(info->map.tiles.size(), 1u);
    EXPECT_EQ(info->map.tiles.front(), game::TileType::Walkable);
    EXPECT_DOUBLE_EQ(info->player_width, 0.5);
    EXPECT_DOUBLE_EQ(info->player_height, 0.5);

    EXPECT_TRUE(channel->is_closing());
    ASSERT_TRUE(channel->get_outgoing_queue().try_pop(message));
    ASSERT_TRUE(std::holds_alternative<game::InventoryState>(message));
    EXPECT_THROW(channel->get_outgoing_queue().try_pop(message), ClosedQueue);
    EXPECT_THROW(channel->get_outgoing_queue().try_pop(message), ClosedQueue);

    try {
        game_loop.rethrow_if_failed();
        FAIL() << "No se propago el fallo de actualizacion";
    } catch (const std::runtime_error& error) {
        EXPECT_STREQ(error.what(), "Fin de la primera actualizacion de prueba");
    }
}
