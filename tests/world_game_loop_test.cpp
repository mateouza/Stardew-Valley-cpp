#include <chrono>
#include <cmath>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

#include "gtest/gtest.h"

#include "game_loop.h"
#include "match_command.h"
#include "server_message.h"
#include "session_channel.h"
#include "session_id.h"
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
                         {TileType::Walkable, TileType::Blocked}}},
                    {0.5, 0.5},
                    {2, 2.0, 0.5, 0.5}};
    static constexpr unsigned int MAX_STEPS = 70;

    Queue<MatchCommand> commands{16};
    std::map<SessionId, std::shared_ptr<SessionChannel>> channels;
    std::map<SessionId, PlayerId> player_ids;
    SessionId next_session_id = 100;
    GameState final_state;

    SessionId add_player(const std::string& name) {
        const auto session_id = next_session_id++;
        auto channel = std::make_shared<SessionChannel>(session_id, MAX_STEPS + 1);
        if (!commands.try_push(AddPlayerCommand{name, channel})) {
            throw std::runtime_error("La cola de comandos de la prueba esta llena");
        }
        channels.emplace(session_id, std::move(channel));
        return session_id;
    }

    void send_direction(SessionId session_id, MovementDirection direction) {
        const PlayerCommand command{session_id, MoveRequest{direction}};
        if (!commands.try_push(command)) {
            throw std::runtime_error("La cola de comandos de la prueba esta llena");
        }
    }

    void read_join_responses() {
        for (const auto& [session_id, channel]: channels) {
            ServerMessage message;
            if (!channel->get_outgoing_queue().try_pop(message)) {
                throw std::runtime_error("No se recibio la respuesta de ingreso");
            }
            const auto* response = std::get_if<JoinResponse>(&message);
            if (!response) {
                throw std::runtime_error("El primer mensaje debe ser la respuesta de ingreso");
            }
            const auto* player_id = std::get_if<PlayerId>(&response->result);
            if (!player_id) {
                throw std::runtime_error("Se rechazo el ingreso del jugador de la prueba");
            }
            player_ids.emplace(session_id, *player_id);
        }
    }

    void run_steps(std::size_t steps, std::function<void(std::size_t)> after_step = {}) {
        if (steps == 0 || steps > MAX_STEPS) {
            throw std::invalid_argument("La prueba debe ejecutar entre 1 y 70 pasos");
        }
        GameLoop loop(world, commands, std::chrono::milliseconds{10});
        std::size_t updates = 0;
        world.set_after_update([&] {
            ++updates;
            if (after_step) {
                after_step(updates);
            }
            if (updates == steps) {
                final_state = world.get_state();
                loop.stop();
            }
        });

        loop.start();
        loop.join();
        world.set_after_update({});
        loop.rethrow_if_failed();
        read_join_responses();

        EXPECT_EQ(updates, steps);
    }

    PlayerState get_player(SessionId session_id) const {
        const auto player_id = player_ids.at(session_id);
        for (const auto& player: final_state.players) {
            if (player.id == player_id) {
                return player;
            }
        }
        throw std::runtime_error("No se encontro al jugador de la prueba");
    }
};

TEST_F(WorldGameLoopTest, MovesOnEveryTickWithoutNewCommands) {
    const auto session_id = add_player("Jose");
    send_direction(session_id, {1, 0});

    run_steps(5);

    const auto player = get_player(session_id);
    EXPECT_NEAR(player.position.x, 0.6, 1e-12);
    EXPECT_DOUBLE_EQ(player.position.y, 0.5);
    EXPECT_TRUE(player.moving);
    EXPECT_EQ(player.facing.horizontal, 1);
    EXPECT_EQ(player.facing.vertical, 0);
}

TEST_F(WorldGameLoopTest, StopRequestStopsFollowingTicks) {
    const auto session_id = add_player("Jose");
    send_direction(session_id, {1, 0});

    run_steps(5, [&](std::size_t step) {
        if (step == 2) {
            send_direction(session_id, {0, 0});
        }
    });

    const auto player = get_player(session_id);
    EXPECT_NEAR(player.position.x, 0.54, 1e-12);
    EXPECT_DOUBLE_EQ(player.position.y, 0.5);
    EXPECT_FALSE(player.moving);
    EXPECT_EQ(player.facing.horizontal, 1);
    EXPECT_EQ(player.facing.vertical, 0);
}

TEST_F(WorldGameLoopTest, RoutesDirectionsToTheirPlayers) {
    const auto first_session = add_player("Jose");
    const auto second_session = add_player("Mateo");
    send_direction(first_session, {1, 0});
    send_direction(second_session, {0, 1});

    run_steps(5);

    const auto first_player = get_player(first_session);
    const auto second_player = get_player(second_session);
    EXPECT_NEAR(first_player.position.x, 0.6, 1e-12);
    EXPECT_DOUBLE_EQ(first_player.position.y, 0.5);
    EXPECT_DOUBLE_EQ(second_player.position.x, 0.5);
    EXPECT_NEAR(second_player.position.y, 0.6, 1e-12);
    EXPECT_TRUE(first_player.moving);
    EXPECT_TRUE(second_player.moving);
}

TEST_F(WorldGameLoopTest, StopsAtLeftAndTopMapEdges) {
    const auto first_session = add_player("Jose");
    const auto second_session = add_player("Mateo");
    send_direction(first_session, {-1, 0});
    send_direction(second_session, {0, -1});

    run_steps(20);

    const auto first_player = get_player(first_session);
    const auto second_player = get_player(second_session);
    EXPECT_DOUBLE_EQ(first_player.position.x, 0.25);
    EXPECT_DOUBLE_EQ(first_player.position.y, 0.5);
    EXPECT_DOUBLE_EQ(second_player.position.x, 0.5);
    EXPECT_DOUBLE_EQ(second_player.position.y, 0.25);
    EXPECT_FALSE(first_player.moving);
    EXPECT_FALSE(second_player.moving);
}

TEST_F(WorldGameLoopTest, StopsAtRightAndBottomMapEdges) {
    const auto first_session = add_player("Jose");
    const auto second_session = add_player("Mateo");
    send_direction(first_session, {1, 0});
    send_direction(second_session, {0, 1});

    run_steps(70);

    const auto first_player = get_player(first_session);
    const auto second_player = get_player(second_session);
    EXPECT_DOUBLE_EQ(first_player.position.x, 1.75);
    EXPECT_DOUBLE_EQ(first_player.position.y, 0.5);
    EXPECT_DOUBLE_EQ(second_player.position.x, 0.5);
    EXPECT_DOUBLE_EQ(second_player.position.y, 1.75);
    EXPECT_FALSE(first_player.moving);
    EXPECT_FALSE(second_player.moving);
}

TEST_F(WorldGameLoopTest, StopsAtBlockedTileHorizontally) {
    const auto session_id = add_player("Jose");
    send_direction(session_id, {0, 1});

    run_steps(70, [&](std::size_t step) {
        if (step == 40) {
            send_direction(session_id, {1, 0});
        }
    });

    const auto player = get_player(session_id);
    EXPECT_DOUBLE_EQ(player.position.x, 0.75);
    EXPECT_NEAR(player.position.y, 1.3, 1e-12);
    EXPECT_FALSE(player.moving);
    EXPECT_EQ(player.facing.horizontal, 1);
    EXPECT_EQ(player.facing.vertical, 0);
}

TEST_F(WorldGameLoopTest, StopsAtBlockedTileVertically) {
    const auto session_id = add_player("Jose");
    send_direction(session_id, {1, 0});

    run_steps(70, [&](std::size_t step) {
        if (step == 40) {
            send_direction(session_id, {0, 1});
        }
    });

    const auto player = get_player(session_id);
    EXPECT_NEAR(player.position.x, 1.3, 1e-12);
    EXPECT_DOUBLE_EQ(player.position.y, 0.75);
    EXPECT_FALSE(player.moving);
    EXPECT_EQ(player.facing.horizontal, 0);
    EXPECT_EQ(player.facing.vertical, 1);
}

TEST_F(WorldGameLoopTest, SlidesAlongVerticalAndHorizontalWalls) {
    const auto first_session = add_player("Jose");
    const auto second_session = add_player("Mateo");
    send_direction(first_session, {0, 1});
    send_direction(second_session, {1, 0});

    run_steps(70, [&](std::size_t step) {
        if (step == 40) {
            send_direction(first_session, {1, -1});
            send_direction(second_session, {-1, 1});
        }
    });

    const double free_coordinate = 1.3 - 0.6 / std::sqrt(2.0);
    const auto first_player = get_player(first_session);
    const auto second_player = get_player(second_session);
    EXPECT_DOUBLE_EQ(first_player.position.x, 0.75);
    EXPECT_NEAR(first_player.position.y, free_coordinate, 1e-12);
    EXPECT_NEAR(second_player.position.x, free_coordinate, 1e-12);
    EXPECT_DOUBLE_EQ(second_player.position.y, 0.75);
    EXPECT_TRUE(first_player.moving);
    EXPECT_TRUE(second_player.moving);
}

}
