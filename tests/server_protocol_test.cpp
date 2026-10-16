#include "server_protocol.h"

#include <array>
#include <cstdint>
#include <stdexcept>

#include <sys/socket.h>

#include "gtest/gtest.h"

#include "liberror.h"
#include "socket.h"

TEST(ServerProtocolTest, ReceivesMoveRight) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    const std::array<std::uint8_t, 3> message{0x01, 0x01, 0x00};

    ASSERT_EQ(client.sendall(message.data(), message.size()), 3);
    client.shutdown(SHUT_WR);

    ServerProtocol protocol(connection);
    const auto request = protocol.receive_move();

    ASSERT_TRUE(request.has_value());
    EXPECT_EQ(request->direction.horizontal, 1);
    EXPECT_EQ(request->direction.vertical, 0);
}

TEST(ServerProtocolTest, ReturnsNoMoveWhenConnectionClosesBeforeMessage) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    client.shutdown(SHUT_WR);

    ServerProtocol protocol(connection);
    const auto request = protocol.receive_move();

    EXPECT_FALSE(request.has_value());
}

TEST(ServerProtocolTest, ReceivesMoveUpLeft) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    const std::array<std::uint8_t, 3> message{0x01, 0xFF, 0xFF};

    ASSERT_EQ(client.sendall(message.data(), message.size()), 3);
    client.shutdown(SHUT_WR);

    ServerProtocol protocol(connection);
    const auto request = protocol.receive_move();

    ASSERT_TRUE(request.has_value());
    EXPECT_EQ(request->direction.horizontal, -1);
    EXPECT_EQ(request->direction.vertical, -1);
}

TEST(ServerProtocolTest, ReceivesStop) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    const std::array<std::uint8_t, 3> message{0x01, 0x00, 0x00};

    ASSERT_EQ(client.sendall(message.data(), message.size()), 3);
    client.shutdown(SHUT_WR);

    ServerProtocol protocol(connection);
    const auto request = protocol.receive_move();

    ASSERT_TRUE(request.has_value());
    EXPECT_EQ(request->direction.horizontal, 0);
    EXPECT_EQ(request->direction.vertical, 0);
}

TEST(ServerProtocolTest, ReceivesConsecutiveMovesAndThenEndOfStream) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    const std::array<std::uint8_t, 6> messages{0x01, 0x01, 0x00, 0x01, 0x00, 0xFF};

    ASSERT_EQ(client.sendall(messages.data(), messages.size()), 6);
    client.shutdown(SHUT_WR);

    ServerProtocol protocol(connection);

    const auto first = protocol.receive_move();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->direction.horizontal, 1);
    EXPECT_EQ(first->direction.vertical, 0);

    const auto second = protocol.receive_move();
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(second->direction.horizontal, 0);
    EXPECT_EQ(second->direction.vertical, -1);

    EXPECT_FALSE(protocol.receive_move().has_value());
}

TEST(ServerProtocolTest, RejectsUnknownMessageType) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    const std::array<std::uint8_t, 1> message{0x7F};

    ASSERT_EQ(client.sendall(message.data(), message.size()), 1);
    client.shutdown(SHUT_WR);

    ServerProtocol protocol(connection);

    EXPECT_THROW(protocol.receive_move(), std::runtime_error);
}

TEST(ServerProtocolTest, RejectsInvalidDirectionInEitherComponent) {
    const std::array<std::array<std::uint8_t, 3>, 4> messages{
            {{0x01, 0x02, 0x00}, {0x01, 0x00, 0x02}, {0x01, 0xFE, 0x00}, {0x01, 0x00, 0xFE}}};

    for (const auto& message: messages) {
        Socket server("18080");
        Socket client("127.0.0.1", "18080");
        Socket connection = server.accept();

        ASSERT_EQ(client.sendall(message.data(), message.size()), 3);
        client.shutdown(SHUT_WR);

        ServerProtocol protocol(connection);

        EXPECT_THROW(protocol.receive_move(), std::runtime_error);
    }
}

TEST(ServerProtocolTest, RejectsMessageWithoutDirectionBytes) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    const std::array<std::uint8_t, 1> message{0x01};

    ASSERT_EQ(client.sendall(message.data(), message.size()), 1);
    client.shutdown(SHUT_WR);

    ServerProtocol protocol(connection);

    EXPECT_THROW(protocol.receive_move(), std::runtime_error);
}

TEST(ServerProtocolTest, RejectsTruncatedDirection) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    const std::array<std::uint8_t, 2> message{0x01, 0x01};

    ASSERT_EQ(client.sendall(message.data(), message.size()), 2);
    client.shutdown(SHUT_WR);

    ServerProtocol protocol(connection);

    EXPECT_THROW(protocol.receive_move(), LibError);
}
