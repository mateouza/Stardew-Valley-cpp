#include "lobby/network_lobby_client.h"

#include <optional>

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

TEST_F(ConnectedNetworkLobbyClientTest, InterruptIsFinal) {
    client.interrupt();

    EXPECT_EQ(client.connect(HOST, PORT, "Juanceto01"), LobbyError::ServerUnavailable);
}

}
