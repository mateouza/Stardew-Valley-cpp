#include "network_client.h"

#include <chrono>
#include <optional>
#include <thread>
#include <variant>

#include <sys/socket.h>

#include "gtest/gtest.h"

#include "server_protocol.h"
#include "socket.h"

namespace {

constexpr const char* PORT = "18081";

class NetworkClientReceiveTest: public ::testing::Test {
protected:
    Socket listener{PORT};
    NetworkClient network{Socket("127.0.0.1", PORT), 16, 16};
    Socket connection{listener.accept()};
    ServerProtocol server_protocol{connection};

    void SetUp() override { network.start(); }
};

}

TEST_F(NetworkClientReceiveTest, WaitsUntilAMessageArrives) {
    std::thread server([this] {
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        server_protocol.send_join_response(JoinResponse{game::PlayerId{7}});
    });

    const auto message = network.receive();
    server.join();

    ASSERT_TRUE(message.has_value());
    const auto* response = std::get_if<JoinResponse>(&*message);
    ASSERT_NE(response, nullptr);
    EXPECT_EQ(std::get<game::PlayerId>(response->result), game::PlayerId{7});
}

TEST_F(NetworkClientReceiveTest, ReturnsNothingWhenTheServerCloses) {
    connection.shutdown(SHUT_RDWR);

    EXPECT_FALSE(network.receive().has_value());
}

TEST_F(NetworkClientReceiveTest, StopWakesUpAWaitingReceive) {
    std::optional<ServerMessage> message = ServerMessage{MatchResponse{std::nullopt}};
    std::thread waiting([this, &message] { message = network.receive(); });

    std::this_thread::sleep_for(std::chrono::milliseconds{50});
    network.stop();
    waiting.join();

    EXPECT_FALSE(message.has_value());
}
