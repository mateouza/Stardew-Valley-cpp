#include "match.h"

#include <chrono>
#include <memory>
#include <stdexcept>
#include <variant>

#include "gtest/gtest.h"

#include "match_manager.h"
#include "player_command.h"
#include "session_channel.h"

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

class MatchManagerTest: public ::testing::Test {
protected:
    MatchManager manager{game::Map{{{game::TileType::Walkable, game::TileType::Walkable}}},
                         {0.5, 0.5},
                         {1, 1.0, 0.5, 0.5},
                         16,
                         std::chrono::milliseconds{20}};
};

TEST_F(MatchManagerTest, FindsCreatedMatchByInvitationCode) {
    const auto created = manager.create_match("Granja de Mateo");
    const auto found = manager.find_match(created.invitation_code);

    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "Granja de Mateo");
    EXPECT_EQ(found->invitation_code, created.invitation_code);
    EXPECT_EQ(found->match, created.match);
}

TEST_F(MatchManagerTest, ReturnsNoMatchForUnknownCode) {
    EXPECT_FALSE(manager.find_match("inexistente").has_value());
}

TEST_F(MatchManagerTest, CreatesDifferentMatchesEvenWithTheSameName) {
    const auto first = manager.create_match("Granja");
    const auto second = manager.create_match("Granja");

    EXPECT_NE(first.invitation_code, second.invitation_code);
    EXPECT_NE(first.match, second.match);
    EXPECT_NE(&first.match->get_commands(), &second.match->get_commands());
}

TEST_F(MatchManagerTest, KeepsPlayerCapacityIndependentBetweenMatches) {
    const auto first = manager.create_match("Primera");
    const auto second = manager.create_match("Segunda");

    auto first_channel = std::make_shared<SessionChannel>(7, 256);
    auto second_channel = std::make_shared<SessionChannel>(8, 256);

    ASSERT_TRUE(first.match->get_commands().try_push(AddPlayerCommand{"Mateo", first_channel}));

    ASSERT_TRUE(second.match->get_commands().try_push(AddPlayerCommand{"Jose", second_channel}));

    const auto first_message = first_channel->get_outgoing_queue().pop();
    const auto second_message = second_channel->get_outgoing_queue().pop();

    const auto* first_response = std::get_if<JoinResponse>(&first_message);
    const auto* second_response = std::get_if<JoinResponse>(&second_message);

    ASSERT_NE(first_response, nullptr);
    ASSERT_NE(second_response, nullptr);

    ASSERT_TRUE(std::holds_alternative<game::PlayerId>(first_response->result));
    ASSERT_TRUE(std::holds_alternative<game::PlayerId>(second_response->result));

    EXPECT_EQ(std::get<game::PlayerId>(first_response->result), 0u);
    EXPECT_EQ(std::get<game::PlayerId>(second_response->result), 0u);
}

TEST_F(MatchManagerTest, StopsMatchesAndRejectsFurtherCreation) {
    const auto created = manager.create_match("Granja");

    manager.stop();

    const MatchCommand command = PlayerCommand{7, game::MoveRequest{game::MovementDirection{1, 0}}};

    EXPECT_THROW(created.match->get_commands().try_push(command), ClosedQueue);
    EXPECT_FALSE(manager.find_match(created.invitation_code).has_value());
    EXPECT_THROW(manager.create_match("Otra"), std::logic_error);

    EXPECT_NO_THROW(manager.stop());
    EXPECT_NO_THROW(manager.rethrow_if_failed());
}

TEST_F(MatchManagerTest, RejectsEmptyNameWithoutPreventingLaterCreation) {
    EXPECT_THROW(manager.create_match(""), std::invalid_argument);

    EXPECT_NO_THROW(manager.create_match("Granja"));
}
