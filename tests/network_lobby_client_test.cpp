#include "lobby/network_lobby_client.h"

#include <chrono>
#include <optional>
#include <thread>
#include <variant>
#include <vector>

#include <sys/socket.h>

#include "gtest/gtest.h"

#include "socket.h"

namespace lobby {

namespace {

constexpr const char* HOST = "127.0.0.1";
constexpr const char* PORT = "18082";

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

}
