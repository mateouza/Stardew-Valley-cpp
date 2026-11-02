#include "lobby/network_lobby_client.h"

#include <chrono>
#include <optional>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include <sys/socket.h>

#include "gtest/gtest.h"

#include "acceptor.h"
#include "game_state.h"
#include "map.h"
#include "match_manager.h"
#include "socket.h"
#include "world_config.h"

namespace lobby {

namespace {

constexpr const char* HOST = "127.0.0.1";
constexpr const char* PORT = "18082";

CharacterDraft make_character(const std::string& name) {
    return CharacterDraft{name, CharacterAppearance{1, 1, 1, 1, 1}};
}

}

TEST(NetworkLobbyClientTest, ConnectFailsWhenNoServerListens) {
    NetworkLobbyClient client;

    EXPECT_EQ(client.connect(HOST, PORT, "Juanceto01"), LobbyError::ServerUnavailable);
    EXPECT_EQ(std::get<LobbyError>(client.list_worlds()), LobbyError::ServerUnavailable);
    EXPECT_EQ(std::get<LobbyError>(client.find_game("12")), LobbyError::ServerUnavailable);
}

TEST(NetworkLobbyClientTest, RejectsInvalidUsernameBeforeConnecting) {
    NetworkLobbyClient client;

    EXPECT_EQ(client.connect(HOST, PORT, "Juan ceto"), LobbyError::InvalidUsername);
}

// Alcanza con un socket escuchando: el sistema acepta la conexión aunque nadie llame a accept().
class ConnectedNetworkLobbyClientTest: public ::testing::Test {
protected:
    Socket listener{PORT};
    NetworkLobbyClient client;

    void SetUp() override { ASSERT_EQ(client.connect(HOST, PORT, "Juanceto01"), std::nullopt); }
};

TEST_F(ConnectedNetworkLobbyClientTest, CanReconnect) {
    EXPECT_EQ(client.connect(HOST, PORT, "Juanceto03"), std::nullopt);
}

TEST_F(ConnectedNetworkLobbyClientTest, OffersTheServerMapAndNoSavedGames) {
    const auto worlds = std::get<std::vector<WorldInfo>>(client.list_worlds());

    ASSERT_EQ(worlds.size(), 1u);
    EXPECT_EQ(worlds[0].id, NetworkLobbyClient::WORLD_ID);
    EXPECT_TRUE(std::get<std::vector<SavedGameInfo>>(client.list_saved_games()).empty());
    EXPECT_EQ(std::get<LobbyError>(client.resume_game("partida-1")), LobbyError::UnknownGame);
}

TEST_F(ConnectedNetworkLobbyClientTest, FindGameOnlyChecksThatThereIsACode) {
    const auto game = std::get<JoinableGameInfo>(client.find_game("  12 "));

    EXPECT_EQ(game.game_id, "12");
    EXPECT_TRUE(game.game_name.empty());
    EXPECT_EQ(std::get<LobbyError>(client.find_game("   ")), LobbyError::InvalidInviteCode);
}

// Nadie contesta: sin interrupt(), create_game() esperaría para siempre.
TEST_F(ConnectedNetworkLobbyClientTest, InterruptWakesUpAGameCreationWaitingForTheServer) {
    std::thread closing([this] {
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        client.interrupt();
    });

    const auto result =
            client.create_game("Granja", NetworkLobbyClient::WORLD_ID, make_character("Juana"));
    closing.join();

    EXPECT_EQ(std::get<LobbyError>(result), LobbyError::ServerUnavailable);
    EXPECT_FALSE(client.take_game_connection().has_value());
}

TEST_F(ConnectedNetworkLobbyClientTest, InterruptIsFinal) {
    client.interrupt();

    EXPECT_EQ(std::get<LobbyError>(client.list_worlds()), LobbyError::ServerUnavailable);
    EXPECT_EQ(client.connect(HOST, PORT, "Juanceto01"), LobbyError::ServerUnavailable);
}

// Si el servidor corta, las respuestas locales también dejan de estar disponibles.
TEST(NetworkLobbyClientTest, AnswersNothingAfterTheServerCloses) {
    Socket listener{PORT};
    NetworkLobbyClient client;
    ASSERT_EQ(client.connect(HOST, PORT, "Juanceto01"), std::nullopt);

    listener.accept().shutdown(SHUT_RDWR);

    // El hilo receptor de NetworkClient se entera de forma asincrónica.
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
    while (std::holds_alternative<std::vector<WorldInfo>>(client.list_worlds()) &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }

    EXPECT_EQ(std::get<LobbyError>(client.list_worlds()), LobbyError::ServerUnavailable);
}

// El servidor de verdad, en el mismo proceso, con un mapa de 2 x 2 casillas transitables.
class ServerNetworkLobbyClientTest: public ::testing::Test {
protected:
    MatchManager matches{game::Map{{{game::TileType::Walkable, game::TileType::Walkable},
                                    {game::TileType::Walkable, game::TileType::Walkable}}},
                         game::Position{0.5, 0.5}, game::WorldConfig{2, 1.0, 0.5, 0.5}, 64,
                         std::chrono::milliseconds{20}};
    Acceptor acceptor{PORT, matches, 64};
    NetworkLobbyClient client;

    void SetUp() override {
        acceptor.start();
        ASSERT_EQ(client.connect(HOST, PORT, "Juanceto01"), std::nullopt);
    }

    void TearDown() override {
        acceptor.stop();
        acceptor.join();
        matches.stop();
    }
};

TEST_F(ServerNetworkLobbyClientTest, CreatesAGameAndHandsOverTheConnection) {
    const auto ticket = std::get<GameTicket>(
            client.create_game(" Granja ", NetworkLobbyClient::WORLD_ID, make_character("Juana")));

    EXPECT_FALSE(ticket.game_id.empty());
    EXPECT_EQ(ticket.game_name, "Granja");

    // Es lo que hace LobbyWorker al cerrar el launcher: no tiene que cortar la conexión del juego.
    client.interrupt();
    auto connection = client.take_game_connection();

    ASSERT_TRUE(connection.has_value());
    EXPECT_FALSE(client.take_game_connection().has_value());
    EXPECT_EQ(connection->invite_code, ticket.game_id);
    EXPECT_EQ(connection->game_info.map.width, 2u);

    const auto message = connection->network->receive();
    ASSERT_TRUE(message.has_value());
    const auto* state = std::get_if<game::GameState>(&*message);
    ASSERT_NE(state, nullptr);
    ASSERT_EQ(state->players.size(), 1u);
    EXPECT_EQ(state->players[0].id, connection->player_id);
    EXPECT_EQ(state->players[0].name, "Juana");
}

}
