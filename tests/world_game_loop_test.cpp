#include <chrono>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <utility>
#include <variant>

#include "gtest/gtest.h"

#include "game_loop.h"
#include "world.h"

namespace game {

class TestWorld: public World {
private:
    std::function<void()> after_update;

public:
    using World::World;

    void set_after_update(std::function<void()> action) { after_update = std::move(action); }

    void update_world(std::chrono::duration<double> elapsed) override {
        World::update_world(elapsed);
        if (after_update) {
            after_update();
        }
    }
};

class WorldGameLoopTest: public ::testing::Test {
protected:
    TestWorld world{Map{{{TileType::Walkable, TileType::Walkable},
                         {TileType::Walkable, TileType::Walkable}}},
                    {0.5, 0.5},
                    {2, 2.0, 0.5, 0.5}};
    Queue<PlayerCommand> commands{16};

    void send_direction(PlayerId player_id, MovementDirection direction) {
        const PlayerCommand command{player_id, MoveRequest{direction}};
        if (!commands.try_push(command)) {
            throw std::runtime_error("La cola de comandos de la prueba esta llena");
        }
    }

    void run_steps(std::size_t steps, std::function<void(std::size_t)> after_step = {}) {
        GameLoop loop(world, commands, std::chrono::milliseconds{10});
        std::size_t updates = 0;
        world.set_after_update([&] {
            ++updates;
            if (after_step) {
                after_step(updates);
            }
            if (updates == steps) {
                loop.stop();
            }
        });

        loop.start();
        loop.join();
        world.set_after_update({});
        loop.rethrow_if_failed();

        EXPECT_EQ(updates, steps);
    }

    PlayerState get_player(PlayerId player_id) const {
        const auto state = world.get_state();
        for (const auto& player: state.players) {
            if (player.id == player_id) {
                return player;
            }
        }
        throw std::runtime_error("No se encontro al jugador de la prueba");
    }
};

TEST_F(WorldGameLoopTest, MovesOnEveryTickWithoutNewCommands) {
    const auto id = std::get<PlayerId>(world.add_player("Jose"));
    send_direction(id, {1, 0});

    run_steps(5);

    const auto player = get_player(id);
    EXPECT_NEAR(player.position.x, 0.6, 1e-12);
    EXPECT_DOUBLE_EQ(player.position.y, 0.5);
    EXPECT_TRUE(player.moving);
    EXPECT_EQ(player.facing.horizontal, 1);
    EXPECT_EQ(player.facing.vertical, 0);
}

TEST_F(WorldGameLoopTest, StopRequestStopsFollowingTicks) {
    const auto id = std::get<PlayerId>(world.add_player("Jose"));
    send_direction(id, {1, 0});

    run_steps(5, [&](std::size_t step) {
        if (step == 2) {
            send_direction(id, {0, 0});
        }
    });

    const auto player = get_player(id);
    EXPECT_NEAR(player.position.x, 0.54, 1e-12);
    EXPECT_DOUBLE_EQ(player.position.y, 0.5);
    EXPECT_FALSE(player.moving);
    EXPECT_EQ(player.facing.horizontal, 1);
    EXPECT_EQ(player.facing.vertical, 0);
}

TEST_F(WorldGameLoopTest, RoutesDirectionsToTheirPlayers) {
    const auto first = std::get<PlayerId>(world.add_player("Jose"));
    const auto second = std::get<PlayerId>(world.add_player("Mateo"));
    send_direction(first, {1, 0});
    send_direction(second, {0, 1});

    run_steps(5);

    const auto first_player = get_player(first);
    const auto second_player = get_player(second);
    EXPECT_NEAR(first_player.position.x, 0.6, 1e-12);
    EXPECT_DOUBLE_EQ(first_player.position.y, 0.5);
    EXPECT_DOUBLE_EQ(second_player.position.x, 0.5);
    EXPECT_NEAR(second_player.position.y, 0.6, 1e-12);
    EXPECT_TRUE(first_player.moving);
    EXPECT_TRUE(second_player.moving);
}

}
