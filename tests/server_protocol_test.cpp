#include "server_protocol.h"

#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <variant>

#include <sys/socket.h>

#include "gtest/gtest.h"

#include "client_protocol.h"
#include "client_receiver.h"
#include "client_sender.h"
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

TEST(ServerProtocolTest, SendsAcceptedJoinWithPlayerIdInNetworkOrder) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    ServerProtocol protocol(connection);
    protocol.send_join_response(JoinResponse{game::PlayerId{0x01020304}});
    connection.shutdown(SHUT_WR);

    std::array<std::uint8_t, 6> received{};
    ASSERT_EQ(client.recvall(received.data(), received.size()), 6);

    const std::array<std::uint8_t, 6> expected{0x02, 0x00, 0x01, 0x02, 0x03, 0x04};
    EXPECT_EQ(received, expected);

    std::uint8_t extra{};
    EXPECT_EQ(client.recvall(&extra, sizeof(extra)), 0);
}

TEST(ServerProtocolTest, SendsAcceptedJoinWithPlayerIdZero) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    ServerProtocol protocol(connection);
    protocol.send_join_response(JoinResponse{game::PlayerId{0}});
    connection.shutdown(SHUT_WR);

    std::array<std::uint8_t, 6> received{};
    ASSERT_EQ(client.recvall(received.data(), received.size()), 6);

    const std::array<std::uint8_t, 6> expected{0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
    EXPECT_EQ(received, expected);
}

TEST(ServerProtocolTest, SendsGameFullWithoutPlayerId) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    ServerProtocol protocol(connection);
    protocol.send_join_response(JoinResponse{game::AddPlayerError::GameFull});
    connection.shutdown(SHUT_WR);

    std::array<std::uint8_t, 2> received{};
    ASSERT_EQ(client.recvall(received.data(), received.size()), 2);

    const std::array<std::uint8_t, 2> expected{0x02, 0x01};
    EXPECT_EQ(received, expected);

    std::uint8_t extra{};
    EXPECT_EQ(client.recvall(&extra, sizeof(extra)), 0);
}

TEST(ServerProtocolTest, SendsEmptyGameState) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    ServerProtocol protocol(connection);
    protocol.send_game_state(game::GameState{});
    connection.shutdown(SHUT_WR);

    std::array<std::uint8_t, 5> received{};
    ASSERT_EQ(client.recvall(received.data(), received.size()), 5);

    const std::array<std::uint8_t, 5> expected{0x03, 0x00, 0x00, 0x00, 0x00};
    EXPECT_EQ(received, expected);

    std::uint8_t extra{};
    EXPECT_EQ(client.recvall(&extra, sizeof(extra)), 0);
}

TEST(ServerProtocolTest, SendsPlayerNameDecimalPositionFacingAndMovement) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    game::GameState state;
    state.players.push_back({7, "Ana", {2.5, 3.25}, {-1, 1}, true});

    ServerProtocol protocol(connection);
    protocol.send_game_state(state);
    connection.shutdown(SHUT_WR);

    std::array<std::uint8_t, 35> received{};
    ASSERT_EQ(client.recvall(received.data(), received.size()), 35);

    const std::array<std::uint8_t, 35> expected{
            0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
            0x03, 0x41, 0x6E, 0x61, 0x40, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x40, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x01, 0x01};

    EXPECT_EQ(received, expected);

    std::uint8_t extra{};
    EXPECT_EQ(client.recvall(&extra, sizeof(extra)), 0);
}

TEST(ServerProtocolTest, RejectsNonFinitePositionBeforeSendingAnyBytes) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    game::GameState state;
    state.players.push_back({7, "Ana", {2.5, 3.25}, {0, 1}, false});
    state.players.push_back(
            {8, "Jose", {std::numeric_limits<double>::infinity(), 1.0}, {0, 1}, false});

    ServerProtocol protocol(connection);

    EXPECT_THROW(protocol.send_game_state(state), std::invalid_argument);
    connection.shutdown(SHUT_WR);

    std::uint8_t byte{};
    EXPECT_EQ(client.recvall(&byte, sizeof(byte)), 0);
}

TEST(ServerProtocolTest, RejectsInvalidFacingBeforeSendingAnyBytes) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    game::GameState state;
    state.players.push_back({7, "Ana", {2.5, 3.25}, {0, 1}, false});
    state.players.push_back({8, "Jose", {1.0, 1.0}, {2, 0}, false});

    ServerProtocol protocol(connection);

    EXPECT_THROW(protocol.send_game_state(state), std::invalid_argument);
    connection.shutdown(SHUT_WR);

    std::uint8_t byte{};
    EXPECT_EQ(client.recvall(&byte, sizeof(byte)), 0);
}

TEST(ServerProtocolTest, SenderSendsJoinResponseAndGameStateInOrder) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    ServerProtocol protocol(connection);
    auto channel = std::make_shared<SessionChannel>(7, 16);
    ClientSender sender(protocol, channel);

    ASSERT_TRUE(channel->get_outgoing_queue().try_push(JoinResponse{game::PlayerId{3}}));
    ASSERT_TRUE(channel->get_outgoing_queue().try_push(game::GameState{}));

    sender.start();

    std::array<std::uint8_t, 11> received{};
    const auto received_size = client.recvall(received.data(), received.size());

    sender.stop();
    EXPECT_NO_THROW(connection.shutdown(SHUT_RDWR));
    sender.join();

    const std::array<std::uint8_t, 11> expected{0x02, 0x00, 0x00, 0x00, 0x00, 0x03,
                                                0x03, 0x00, 0x00, 0x00, 0x00};

    EXPECT_EQ(received_size, 11);
    EXPECT_EQ(received, expected);
    EXPECT_FALSE(sender.is_alive());
    EXPECT_TRUE(channel->is_closing());
    EXPECT_NO_THROW(sender.rethrow_if_failed());
}

TEST(ServerProtocolTest, SenderCanStopWithAnEmptyQueue) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    ServerProtocol protocol(connection);
    auto channel = std::make_shared<SessionChannel>(7, 16);
    ClientSender sender(protocol, channel);

    sender.start();
    sender.stop();
    EXPECT_NO_THROW(connection.shutdown(SHUT_RDWR));
    sender.join();

    EXPECT_FALSE(sender.is_alive());
    EXPECT_TRUE(channel->is_closing());
    EXPECT_NO_THROW(sender.rethrow_if_failed());
    EXPECT_NO_THROW(sender.stop());
}

TEST(ServerProtocolTest, SenderSendsRejectionBeforeRequestingClosure) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    ServerProtocol protocol(connection);
    auto channel = std::make_shared<SessionChannel>(7, 16);
    ClientSender sender(protocol, channel);

    ASSERT_TRUE(
            channel->get_outgoing_queue().try_push(JoinResponse{game::AddPlayerError::GameFull}));

    sender.start();
    sender.join();

    connection.shutdown(SHUT_WR);

    std::array<std::uint8_t, 2> received{};
    ASSERT_EQ(client.recvall(received.data(), received.size()), 2);

    const std::array<std::uint8_t, 2> expected{0x02, 0x01};
    EXPECT_EQ(received, expected);

    std::uint8_t extra{};
    EXPECT_EQ(client.recvall(&extra, sizeof(extra)), 0);

    EXPECT_TRUE(channel->is_closing());
    EXPECT_FALSE(sender.is_alive());
    EXPECT_NO_THROW(sender.rethrow_if_failed());
}

TEST(ServerProtocolTest, SessionSendsJoinResponseAndGameState) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    Queue<MatchCommand> commands(16);
    auto channel = std::make_shared<SessionChannel>(7, 16);
    ClientSession session(std::move(connection));

    session.start(commands, "Mateo", channel);

    const auto admission_command = commands.pop();

    ASSERT_TRUE(channel->get_outgoing_queue().try_push(JoinResponse{game::PlayerId{3}}));
    ASSERT_TRUE(channel->get_outgoing_queue().try_push(game::GameState{}));

    std::array<std::uint8_t, 11> received{};
    const auto received_size = client.recvall(received.data(), received.size());

    session.stop();
    session.join();

    const auto* admission = std::get_if<AddPlayerCommand>(&admission_command);
    ASSERT_NE(admission, nullptr);
    EXPECT_EQ(admission->name, "Mateo");
    EXPECT_EQ(admission->channel, channel);

    const std::array<std::uint8_t, 11> expected{0x02, 0x00, 0x00, 0x00, 0x00, 0x03,
                                                0x03, 0x00, 0x00, 0x00, 0x00};

    EXPECT_EQ(received_size, 11);
    EXPECT_EQ(received, expected);
    EXPECT_TRUE(channel->is_closing());
    EXPECT_TRUE(session.has_finished());
    EXPECT_NO_THROW(session.rethrow_if_failed());
    EXPECT_NO_THROW(session.stop());
    EXPECT_NO_THROW(session.join());
}

class ClientServerProtocolTest: public ::testing::Test {
protected:
    Socket listener;
    Socket client;
    Socket connection;
    ClientProtocol client_protocol;
    ServerProtocol server_protocol;

    ClientServerProtocolTest():
            listener("18080"),
            client("127.0.0.1", "18080"),
            connection(listener.accept()),
            client_protocol(client),
            server_protocol(connection) {}
};

TEST_F(ClientServerProtocolTest, TransfersCreateMatchRequest) {
    client_protocol.send_create_match(CreateMatchRequest{"Granja de amigos", "Mateo"});

    const auto request = server_protocol.receive_lobby_request();

    ASSERT_TRUE(request.has_value());

    const auto* create = std::get_if<CreateMatchRequest>(&*request);
    ASSERT_NE(create, nullptr);
    EXPECT_EQ(create->match_name, "Granja de amigos");
    EXPECT_EQ(create->player_name, "Mateo");
}

TEST_F(ClientServerProtocolTest, TransfersJoinMatchRequest) {
    client_protocol.send_join_match(JoinMatchRequest{"ABC123", "Jose"});

    const auto request = server_protocol.receive_lobby_request();

    ASSERT_TRUE(request.has_value());

    const auto* join = std::get_if<JoinMatchRequest>(&*request);
    ASSERT_NE(join, nullptr);
    EXPECT_EQ(join->invitation_code, "ABC123");
    EXPECT_EQ(join->player_name, "Jose");
}

TEST_F(ClientServerProtocolTest, TransfersMovement) {
    client_protocol.send_move(game::MoveRequest{game::MovementDirection{-1, 1}});

    const auto request = server_protocol.receive_move();

    ASSERT_TRUE(request.has_value());
    EXPECT_EQ(request->direction.horizontal, -1);
    EXPECT_EQ(request->direction.vertical, 1);
}

TEST_F(ClientServerProtocolTest, TransfersJoinResponseAndStateInOrder) {
    server_protocol.send_join_response(JoinResponse{game::PlayerId{0x01020304}});

    game::GameState state;
    state.players.push_back(game::PlayerState{0x01020304, "Jose", {-2.5, 3.25}, {-1, 1}, true});

    server_protocol.send_game_state(state);
    connection.shutdown(SHUT_WR);

    const auto first = client_protocol.receive_message();
    ASSERT_TRUE(first.has_value());

    const auto* response = std::get_if<JoinResponse>(&*first);
    ASSERT_NE(response, nullptr);

    const auto* player_id = std::get_if<game::PlayerId>(&response->result);
    ASSERT_NE(player_id, nullptr);
    EXPECT_EQ(*player_id, 0x01020304u);

    const auto second = client_protocol.receive_message();
    ASSERT_TRUE(second.has_value());

    const auto* received = std::get_if<game::GameState>(&*second);
    ASSERT_NE(received, nullptr);
    ASSERT_EQ(received->players.size(), 1u);

    const auto& player = received->players.front();
    EXPECT_EQ(player.id, 0x01020304u);
    EXPECT_EQ(player.name, "Jose");
    EXPECT_DOUBLE_EQ(player.position.x, -2.5);
    EXPECT_DOUBLE_EQ(player.position.y, 3.25);
    EXPECT_EQ(player.facing.horizontal, -1);
    EXPECT_EQ(player.facing.vertical, 1);
    EXPECT_TRUE(player.moving);

    EXPECT_FALSE(client_protocol.receive_message().has_value());
}

TEST_F(ClientServerProtocolTest, TransfersGameFullResponse) {
    server_protocol.send_join_response(JoinResponse{game::AddPlayerError::GameFull});

    const auto message = client_protocol.receive_message();
    ASSERT_TRUE(message.has_value());

    const auto* response = std::get_if<JoinResponse>(&*message);
    ASSERT_NE(response, nullptr);

    const auto* error = std::get_if<game::AddPlayerError>(&response->result);
    ASSERT_NE(error, nullptr);
    EXPECT_EQ(*error, game::AddPlayerError::GameFull);
}

TEST_F(ClientServerProtocolTest, ReturnsNoLobbyRequestOnEndOfStream) {
    client.shutdown(SHUT_WR);

    EXPECT_FALSE(server_protocol.receive_lobby_request().has_value());
}

TEST_F(ClientServerProtocolTest, RejectsUnknownServerMessage) {
    const std::array<std::uint8_t, 1> message{0x7F};
    ASSERT_EQ(connection.sendall(message.data(), message.size()), 1);
    connection.shutdown(SHUT_WR);

    EXPECT_THROW(client_protocol.receive_message(), std::runtime_error);
}

TEST_F(ClientServerProtocolTest, RejectsInvalidCreateBeforeSendingAnyBytes) {
    EXPECT_THROW(client_protocol.send_create_match(CreateMatchRequest{"", "Mateo"}),
                 std::invalid_argument);

    EXPECT_THROW(
            client_protocol.send_create_match(CreateMatchRequest{"Granja", std::string(257, 'a')}),
            std::length_error);

    client.shutdown(SHUT_WR);

    EXPECT_FALSE(server_protocol.receive_lobby_request().has_value());
}