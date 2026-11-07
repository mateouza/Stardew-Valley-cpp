#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "gtest/gtest.h"

#include "game_loop.h"
#include "world.h"

namespace {

class CheatSeedsWorld: public game::World {
private:
    std::uint32_t initial_seeds;

public:
    explicit CheatSeedsWorld(std::uint32_t initial_seeds = 0):
            game::World(game::Map{{{game::TileType::Walkable, game::TileType::Walkable}}},
                        {0.5, 0.5}, {2, 1.0, 0.5, 0.5},
                        game::Soil{{{game::SoilState::Unprepared, game::SoilState::Unprepared}}}),
            initial_seeds(initial_seeds) {}

    game::AddPlayerResult add_player(const std::string& name) override {
        const auto result = game::World::add_player(name);
        const auto* id = std::get_if<game::PlayerId>(&result);
        if (id && initial_seeds != 0 &&
            add_item(*id, {game::ItemType::StrawberrySeed, game::ItemQuality::Base,
                           initial_seeds}) != game::InventoryResult::Accepted) {
            throw std::logic_error("No se pudo preparar el inventario de prueba");
        }
        return result;
    }

    void update_world(std::chrono::duration<double>) override {
        throw std::runtime_error("Fin del tick de prueba del cheat");
    }
};

class CheatSeedsGameLoopTest: public ::testing::Test {
protected:
    Queue<MatchCommand> commands{32};
    std::shared_ptr<SessionChannel> owner = std::make_shared<SessionChannel>(7, 64);
    std::shared_ptr<SessionChannel> other = std::make_shared<SessionChannel>(8, 64);
    const game::GameInfo info{
            {2, 1, {game::TileType::Walkable, game::TileType::Walkable}}, 0.5, 0.5};

    void run_tick(CheatSeedsWorld& world) {
        GameLoop loop(world, commands, std::chrono::milliseconds{1}, info);
        loop.start();
        loop.join();
        try {
            loop.rethrow_if_failed();
            FAIL() << "El modelo debe terminar la prueba al completar los comandos";
        } catch (const std::runtime_error& error) {
            EXPECT_STREQ(error.what(), "Fin del tick de prueba del cheat");
        }
    }

    std::vector<ServerMessage> read_messages(SessionChannel& channel) {
        std::vector<ServerMessage> messages;
        ServerMessage message;
        try {
            while (channel.get_outgoing_queue().try_pop(message)) {
                messages.push_back(message);
            }
        } catch (const ClosedQueue&) {}
        return messages;
    }
};

TEST_F(CheatSeedsGameLoopTest, GrantsTenSeedsThenPlantingLeavesNineOnlyForTheOwner) {
    CheatSeedsWorld world;
    ASSERT_TRUE(commands.try_push(AddPlayerCommand{"Mateo", owner}));
    ASSERT_TRUE(commands.try_push(AddPlayerCommand{"Jose", other}));
    ASSERT_TRUE(commands.try_push(CheatSeedsCommand{7}));
    ASSERT_TRUE(commands.try_push(SelectSlotCommand{7, game::SelectSlotRequest{2}}));
    ASSERT_TRUE(commands.try_push(UseCellCommand{7, game::UseCellRequest{{0, 1}}}));
    ASSERT_TRUE(commands.try_push(SelectSlotCommand{7, game::SelectSlotRequest{4}}));
    ASSERT_TRUE(commands.try_push(UseCellCommand{7, game::UseCellRequest{{0, 1}}}));

    run_tick(world);

    std::vector<std::uint32_t> seed_quantities;
    std::size_t responses = 0;
    bool confirmed = false;
    for (const auto& message: read_messages(*owner)) {
        if (const auto* response = std::get_if<CheatSeedsResponse>(&message)) {
            EXPECT_EQ(response->result, game::InventoryResult::Accepted);
            confirmed = true;
            ++responses;
        }
        if (const auto* inventory = std::get_if<game::InventoryState>(&message)) {
            const auto& item = inventory->items[4];
            if (item) {
                EXPECT_TRUE(confirmed);
                EXPECT_EQ(item->type, game::ItemType::StrawberrySeed);
                EXPECT_EQ(item->quality, game::ItemQuality::Base);
            }
            seed_quantities.push_back(item ? item->quantity : 0);
        }
    }
    EXPECT_EQ(responses, 1u);
    EXPECT_EQ(seed_quantities, (std::vector<std::uint32_t>{0, 10, 10, 10, 9}));

    std::size_t other_inventories = 0;
    for (const auto& message: read_messages(*other)) {
        EXPECT_FALSE(std::holds_alternative<CheatSeedsResponse>(message));
        if (const auto* inventory = std::get_if<game::InventoryState>(&message)) {
            ++other_inventories;
            EXPECT_FALSE(inventory->items[4].has_value());
        }
    }
    EXPECT_EQ(other_inventories, 1u);
}

TEST_F(CheatSeedsGameLoopTest, RejectsWithoutPartiallyFillingTheLastStack) {
    CheatSeedsWorld world{245};
    ASSERT_TRUE(commands.try_push(AddPlayerCommand{"Mateo", owner}));
    ASSERT_TRUE(commands.try_push(CheatSeedsCommand{7}));

    run_tick(world);

    std::size_t responses = 0;
    std::size_t inventories = 0;
    for (const auto& message: read_messages(*owner)) {
        if (const auto* response = std::get_if<CheatSeedsResponse>(&message)) {
            ++responses;
            EXPECT_EQ(response->result, game::InventoryResult::NotEnoughSpace);
        }
        inventories += std::holds_alternative<game::InventoryState>(message);
    }
    EXPECT_EQ(responses, 1u);
    EXPECT_EQ(inventories, 1u);

    const auto inventory = world.get_inventory_state(0);
    ASSERT_TRUE(inventory.has_value());
    EXPECT_EQ(inventory->selected_slot, 0u);
    for (std::size_t slot = 4; slot < inventory->items.size(); ++slot) {
        ASSERT_TRUE(inventory->items[slot].has_value());
        EXPECT_EQ(inventory->items[slot]->quantity, slot == 8 ? 45u : 50u);
    }
}

TEST_F(CheatSeedsGameLoopTest, IgnoresUnknownSessionsWithoutGrantingSeeds) {
    CheatSeedsWorld world;
    ASSERT_TRUE(commands.try_push(AddPlayerCommand{"Mateo", owner}));
    ASSERT_TRUE(commands.try_push(CheatSeedsCommand{999}));

    run_tick(world);

    for (const auto& message: read_messages(*owner)) {
        EXPECT_FALSE(std::holds_alternative<CheatSeedsResponse>(message));
    }
    const auto inventory = world.get_inventory_state(0);
    ASSERT_TRUE(inventory.has_value());
    EXPECT_FALSE(inventory->items[4].has_value());
}

}
