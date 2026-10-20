#include "server_protocol.h"

#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>
#include <variant>

#include <sys/socket.h>

#include "gtest/gtest.h"

#include "client_receiver.h"
#include "client_session.h"
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

TEST(ServerProtocolTest, CoordinatesReceiverStopShutdownAndJoin) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    ServerProtocol protocol(connection);
    Queue<MatchCommand> commands(16);
    auto channel = std::make_shared<SessionChannel>(7, 16);
    ClientReceiver receiver(protocol, commands, channel);

    const std::array<std::uint8_t, 3> message{0x01, 0x01, 0x00};
    ASSERT_EQ(client.sendall(message.data(), message.size()), 3);

    receiver.start();

    const auto command = commands.pop();

    receiver.stop();
    EXPECT_NO_THROW(connection.shutdown(SHUT_RDWR));
    receiver.join();

    EXPECT_FALSE(receiver.is_alive());
    EXPECT_TRUE(channel->is_closing());
    EXPECT_NO_THROW(receiver.rethrow_if_failed());

    const auto* movement = std::get_if<PlayerCommand>(&command);
    ASSERT_NE(movement, nullptr);
    EXPECT_EQ(movement->session_id, 7u);
    EXPECT_EQ(movement->request.direction.horizontal, 1);
    EXPECT_EQ(movement->request.direction.vertical, 0);

    EXPECT_TRUE(connection.is_stream_send_closed());
    EXPECT_TRUE(connection.is_stream_recv_closed());
}

TEST(ServerProtocolTest, SessionEnqueuesAdmissionAndCoordinatesReceiverCleanup) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    Queue<MatchCommand> commands(16);
    auto channel = std::make_shared<SessionChannel>(7, 16);
    ClientSession session(std::move(connection));

    const std::array<std::uint8_t, 3> message{0x01, 0x01, 0x00};
    ASSERT_EQ(client.sendall(message.data(), message.size()), 3);

    session.start(commands, "Mateo", channel);

    const auto first = commands.pop();
    const auto second = commands.pop();

    session.stop();
    session.join();

    const auto* admission = std::get_if<AddPlayerCommand>(&first);
    ASSERT_NE(admission, nullptr);
    EXPECT_EQ(admission->name, "Mateo");
    EXPECT_EQ(admission->channel, channel);

    const auto* movement = std::get_if<PlayerCommand>(&second);
    ASSERT_NE(movement, nullptr);
    EXPECT_EQ(movement->session_id, 7u);
    EXPECT_EQ(movement->request.direction.horizontal, 1);
    EXPECT_EQ(movement->request.direction.vertical, 0);

    EXPECT_TRUE(channel->is_closing());
    EXPECT_TRUE(session.has_finished());
    EXPECT_NO_THROW(session.rethrow_if_failed());
    EXPECT_NO_THROW(session.stop());
    EXPECT_NO_THROW(session.join());
}

TEST(ServerProtocolTest, SessionDestructorStopsReceiverAndShutsDownConnection) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    Queue<MatchCommand> commands(16);
    auto channel = std::make_shared<SessionChannel>(7, 16);

    {
        ClientSession session(std::move(connection));
        session.start(commands, "Mateo", channel);
    }

    EXPECT_TRUE(channel->is_closing());

    std::uint8_t byte{};
    EXPECT_EQ(client.recvall(&byte, sizeof(byte)), 0);
}

TEST(ServerProtocolTest, ReceiverRequestsClosureOnEndOfStream) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    ServerProtocol protocol(connection);
    Queue<MatchCommand> commands(16);
    auto channel = std::make_shared<SessionChannel>(7, 16);
    ClientReceiver receiver(protocol, commands, channel);

    client.shutdown(SHUT_WR);

    receiver.start();
    receiver.join();

    EXPECT_TRUE(channel->is_closing());
    EXPECT_FALSE(receiver.is_alive());
    EXPECT_NO_THROW(receiver.rethrow_if_failed());

    MatchCommand command;
    EXPECT_FALSE(commands.try_pop(command));
}

TEST(ServerProtocolTest, ReceiverRequestsClosureWhenCommandsQueueIsFull) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    ServerProtocol protocol(connection);
    Queue<MatchCommand> commands(1);
    auto channel = std::make_shared<SessionChannel>(7, 16);
    ClientReceiver receiver(protocol, commands, channel);

    const PlayerCommand previous{8, game::MoveRequest{game::MovementDirection{0, 1}}};
    ASSERT_TRUE(commands.try_push(previous));

    const std::array<std::uint8_t, 3> message{0x01, 0x01, 0x00};
    ASSERT_EQ(client.sendall(message.data(), message.size()), 3);

    receiver.start();
    receiver.join();

    EXPECT_TRUE(channel->is_closing());
    EXPECT_FALSE(receiver.is_alive());
    EXPECT_THROW(receiver.rethrow_if_failed(), std::runtime_error);

    MatchCommand remaining;
    ASSERT_TRUE(commands.try_pop(remaining));

    const auto* movement = std::get_if<PlayerCommand>(&remaining);
    ASSERT_NE(movement, nullptr);
    EXPECT_EQ(movement->session_id, 8u);
    EXPECT_EQ(movement->request.direction.horizontal, 0);
    EXPECT_EQ(movement->request.direction.vertical, 1);

    EXPECT_FALSE(commands.try_pop(remaining));
}
