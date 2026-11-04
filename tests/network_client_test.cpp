#include "network_client.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <thread>
#include <variant>
#include <vector>

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

class NetworkClientCloseTest: public NetworkClientReceiveTest {};

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

TEST_F(NetworkClientCloseTest, SenderFailureAlsoStopsTheReceiver) {
    ASSERT_TRUE(network.try_send(JoinMatchRequest{"", "Juanceto"}));

    EXPECT_FALSE(network.receive().has_value());
    network.join();

    char byte = 0;
    EXPECT_EQ(connection.recvall(&byte, sizeof(byte)), 0);
    EXPECT_TRUE(network.has_finished());
    EXPECT_THROW(network.rethrow_if_failed(), std::invalid_argument);
}

TEST_F(NetworkClientCloseTest, ReceiverFailureAlsoStopsTheSender) {
    const std::uint8_t unknown_message = 0x7F;
    ASSERT_EQ(connection.sendall(&unknown_message, sizeof(unknown_message)), 1);

    EXPECT_FALSE(network.receive().has_value());
    network.join();

    EXPECT_TRUE(network.has_finished());
    EXPECT_THROW(network.try_send(game::MoveRequest{game::MovementDirection{1, 0}}),
                 std::logic_error);
    EXPECT_THROW(network.rethrow_if_failed(), std::runtime_error);
}

TEST_F(NetworkClientCloseTest, StopCanBeCalledFromSeveralThreadsAtOnce) {
    std::vector<std::thread> stoppers;
    for (int i = 0; i < 8; ++i) {
        stoppers.emplace_back([this] { network.stop(); });
    }
    for (std::thread& stopper: stoppers) {
        stopper.join();
    }
    network.join();

    EXPECT_TRUE(network.has_finished());
    EXPECT_NO_THROW(network.rethrow_if_failed());
}
