#include "server_protocol.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
#include <variant>

#include <sys/socket.h>

#include "gtest/gtest.h"

#include "client_handler.h"
#include "client_protocol.h"
#include "client_receiver.h"
#include "client_sender.h"
#include "client_session.h"
#include "liberror.h"
#include "match_manager.h"
#include "network_client.h"
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

TEST_F(ClientServerProtocolTest, HandlerStopsWithoutReceivingALobbyRequest) {
    MatchManager manager(game::Map{{{game::TileType::Walkable}}}, {0.5, 0.5}, {2, 1.0, 0.5, 0.5},
                         16, std::chrono::milliseconds{20});

    ClientHandler handler(std::move(connection), manager, 7, 256);

    handler.start();
    handler.stop();
    handler.join();

    EXPECT_FALSE(handler.is_alive());
    EXPECT_NO_THROW(handler.rethrow_if_failed());
}

TEST_F(ClientServerProtocolTest, TwoHandlersShareMatchAndPublishMovement) {
    MatchManager manager(game::Map{{{game::TileType::Walkable, game::TileType::Walkable}}},
                         {0.5, 0.5}, {2, 1.0, 0.5, 0.5}, 256, std::chrono::milliseconds{20});

    ClientHandler first_handler(std::move(connection), manager, 7, 256);
    first_handler.start();

    client_protocol.send_create_match(CreateMatchRequest{"Granja", "Mateo"});

    const auto selection_message = client_protocol.receive_message();
    ASSERT_TRUE(selection_message.has_value());

    const auto* selection = std::get_if<MatchResponse>(&*selection_message);
    ASSERT_NE(selection, nullptr);
    ASSERT_TRUE(selection->invitation_code.has_value());

    const auto first_join_message = client_protocol.receive_message();
    ASSERT_TRUE(first_join_message.has_value());

    const auto* first_join = std::get_if<JoinResponse>(&*first_join_message);
    ASSERT_NE(first_join, nullptr);
    ASSERT_TRUE(std::holds_alternative<game::PlayerId>(first_join->result));

    const auto first_id = std::get<game::PlayerId>(first_join->result);

    const auto first_info_message = client_protocol.receive_message();
    ASSERT_TRUE(first_info_message.has_value());

    const auto* first_info = std::get_if<game::GameInfo>(&*first_info_message);
    ASSERT_NE(first_info, nullptr);

    EXPECT_EQ(first_info->map.width, 2u);
    EXPECT_EQ(first_info->map.height, 1u);
    ASSERT_EQ(first_info->map.tiles.size(), 2u);
    EXPECT_EQ(first_info->map.tiles[0], game::TileType::Walkable);
    EXPECT_EQ(first_info->map.tiles[1], game::TileType::Walkable);
    EXPECT_DOUBLE_EQ(first_info->player_width, 0.5);
    EXPECT_DOUBLE_EQ(first_info->player_height, 0.5);

    Socket second_client("127.0.0.1", "18080");
    Socket second_connection = listener.accept();
    ClientProtocol second_protocol(second_client);
    ClientHandler second_handler(std::move(second_connection), manager, 8, 256);

    second_handler.start();

    second_protocol.send_join_match(JoinMatchRequest{*selection->invitation_code, "Jose"});

    const auto second_selection_message = second_protocol.receive_message();
    ASSERT_TRUE(second_selection_message.has_value());

    const auto* second_selection = std::get_if<MatchResponse>(&*second_selection_message);
    ASSERT_NE(second_selection, nullptr);
    EXPECT_EQ(second_selection->invitation_code, selection->invitation_code);

    const auto second_join_message = second_protocol.receive_message();
    ASSERT_TRUE(second_join_message.has_value());

    const auto* second_join = std::get_if<JoinResponse>(&*second_join_message);
    ASSERT_NE(second_join, nullptr);
    ASSERT_TRUE(std::holds_alternative<game::PlayerId>(second_join->result));

    const auto second_id = std::get<game::PlayerId>(second_join->result);
    ASSERT_NE(first_id, second_id);

    const auto second_info_message = second_protocol.receive_message();
    ASSERT_TRUE(second_info_message.has_value());

    const auto* second_info = std::get_if<game::GameInfo>(&*second_info_message);
    ASSERT_NE(second_info, nullptr);

    EXPECT_EQ(second_info->map.width, first_info->map.width);
    EXPECT_EQ(second_info->map.height, first_info->map.height);
    EXPECT_EQ(second_info->map.tiles, first_info->map.tiles);
    EXPECT_DOUBLE_EQ(second_info->player_width, first_info->player_width);
    EXPECT_DOUBLE_EQ(second_info->player_height, first_info->player_height);

    second_protocol.send_move(game::MoveRequest{game::MovementDirection{1, 0}});

    bool observed_movement = false;

    for (std::size_t attempt = 0; attempt < 128 && !observed_movement; ++attempt) {
        const auto message = client_protocol.receive_message();
        ASSERT_TRUE(message.has_value());

        const auto* state = std::get_if<game::GameState>(&*message);
        ASSERT_NE(state, nullptr);

        if (state->players.size() != 2) {
            continue;
        }

        for (const auto& player: state->players) {
            if (player.id == first_id) {
                EXPECT_EQ(player.name, "Mateo");
                EXPECT_DOUBLE_EQ(player.position.x, 0.5);
                EXPECT_DOUBLE_EQ(player.position.y, 0.5);
            }

            if (player.id == second_id && player.position.x > 0.5) {
                EXPECT_EQ(player.name, "Jose");
                EXPECT_DOUBLE_EQ(player.position.y, 0.5);
                EXPECT_EQ(player.facing.horizontal, 1);
                EXPECT_EQ(player.facing.vertical, 0);
                EXPECT_TRUE(player.moving);
                observed_movement = true;
            }
        }
    }

    EXPECT_TRUE(observed_movement);

    first_handler.stop();
    second_handler.stop();

    first_handler.join();
    second_handler.join();

    EXPECT_NO_THROW(first_handler.rethrow_if_failed());
    EXPECT_NO_THROW(second_handler.rethrow_if_failed());
}

class NetworkClientTest: public ClientServerProtocolTest {
protected:
    std::optional<ServerMessage> wait_message(NetworkClient& network) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};

        while (std::chrono::steady_clock::now() < deadline) {
            auto message = network.try_receive();

            if (message) {
                return message;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }

        return std::nullopt;
    }

    bool wait_finished(NetworkClient& network) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};

        while (std::chrono::steady_clock::now() < deadline) {
            if (network.has_finished()) {
                return true;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }

        return network.has_finished();
    }
};

TEST_F(NetworkClientTest, SendsQueuedRequestsInOrder) {
    NetworkClient network(std::move(client), 16, 16);
    network.start();

    ASSERT_TRUE(network.try_send(CreateMatchRequest{"Granja", "Mateo"}));
    ASSERT_TRUE(network.try_send(game::MoveRequest{game::MovementDirection{-1, 1}}));

    const auto request = server_protocol.receive_lobby_request();
    ASSERT_TRUE(request.has_value());

    const auto* create = std::get_if<CreateMatchRequest>(&*request);
    ASSERT_NE(create, nullptr);
    EXPECT_EQ(create->match_name, "Granja");
    EXPECT_EQ(create->player_name, "Mateo");

    const auto movement = server_protocol.receive_move();
    ASSERT_TRUE(movement.has_value());
    EXPECT_EQ(movement->direction.horizontal, -1);
    EXPECT_EQ(movement->direction.vertical, 1);

    network.stop();
    network.join();

    EXPECT_NO_THROW(network.rethrow_if_failed());
}

TEST_F(NetworkClientTest, ReceivesResponsesAndStatesInOrder) {
    NetworkClient network(std::move(client), 16, 16);
    network.start();

    server_protocol.send_match_response(MatchResponse{std::string{"ABC123"}});
    server_protocol.send_join_response(JoinResponse{game::PlayerId{7}});

    game::GameState state;
    state.players.push_back(game::PlayerState{7, "Mateo", {2.5, 3.25}, {1, 0}, true});

    server_protocol.send_game_state(state);

    const auto first = wait_message(network);
    ASSERT_TRUE(first.has_value());

    const auto* selection = std::get_if<MatchResponse>(&*first);
    ASSERT_NE(selection, nullptr);
    ASSERT_TRUE(selection->invitation_code.has_value());
    EXPECT_EQ(*selection->invitation_code, "ABC123");

    const auto second = wait_message(network);
    ASSERT_TRUE(second.has_value());

    const auto* join = std::get_if<JoinResponse>(&*second);
    ASSERT_NE(join, nullptr);

    const auto* id = std::get_if<game::PlayerId>(&join->result);
    ASSERT_NE(id, nullptr);
    EXPECT_EQ(*id, 7u);

    const auto third = wait_message(network);
    ASSERT_TRUE(third.has_value());

    const auto* received = std::get_if<game::GameState>(&*third);
    ASSERT_NE(received, nullptr);
    ASSERT_EQ(received->players.size(), 1u);

    const auto& player = received->players.front();
    EXPECT_EQ(player.name, "Mateo");
    EXPECT_DOUBLE_EQ(player.position.x, 2.5);
    EXPECT_DOUBLE_EQ(player.position.y, 3.25);
    EXPECT_EQ(player.facing.horizontal, 1);
    EXPECT_EQ(player.facing.vertical, 0);
    EXPECT_TRUE(player.moving);

    network.stop();
    network.join();

    EXPECT_NO_THROW(network.rethrow_if_failed());
}

TEST_F(NetworkClientTest, SupportsIdleShutdownAndRejectsRestart) {
    NetworkClient network(std::move(client), 16, 16);
    network.start();

    EXPECT_FALSE(network.try_receive().has_value());

    network.stop();
    network.join();

    EXPECT_TRUE(network.has_finished());
    EXPECT_NO_THROW(network.stop());
    EXPECT_NO_THROW(network.join());
    EXPECT_NO_THROW(network.rethrow_if_failed());

    EXPECT_THROW(network.start(), std::logic_error);
    EXPECT_THROW(network.try_send(game::MoveRequest{game::MovementDirection{1, 0}}),
                 std::logic_error);
}

TEST_F(NetworkClientTest, DetectsServerEndOfStream) {
    NetworkClient network(std::move(client), 16, 16);
    network.start();

    connection.shutdown(SHUT_WR);

    EXPECT_TRUE(wait_finished(network));

    network.stop();
    network.join();

    EXPECT_NO_THROW(network.rethrow_if_failed());
}

TEST_F(NetworkClientTest, PreservesInvalidMessageFailureAfterJoin) {
    NetworkClient network(std::move(client), 16, 16);
    network.start();

    const std::array<std::uint8_t, 1> message{0x7F};
    ASSERT_EQ(connection.sendall(message.data(), message.size()), 1);

    EXPECT_TRUE(wait_finished(network));

    network.stop();
    network.join();

    EXPECT_THROW(network.rethrow_if_failed(), std::runtime_error);
}

TEST_F(NetworkClientTest, ReportsIncomingQueueSaturation) {
    NetworkClient network(std::move(client), 16, 1);
    network.start();

    server_protocol.send_game_state(game::GameState{});
    server_protocol.send_game_state(game::GameState{});

    EXPECT_TRUE(wait_finished(network));

    network.stop();
    network.join();

    EXPECT_THROW(network.rethrow_if_failed(), std::runtime_error);

    const auto message = network.try_receive();
    ASSERT_TRUE(message.has_value());
    EXPECT_TRUE(std::holds_alternative<game::GameState>(*message));
    EXPECT_FALSE(network.try_receive().has_value());
}

TEST(ServerProtocolTest, SendsGameInfoToClientProtocol) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    ServerProtocol server_protocol(connection);
    ClientProtocol client_protocol(client);

    const game::GameInfo info{
            {2, 1, {game::TileType::Walkable, game::TileType::Blocked}}, 0.5, 0.5};

    server_protocol.send_game_info(info);
    connection.shutdown(SHUT_WR);

    const auto message = client_protocol.receive_message();
    ASSERT_TRUE(message.has_value());

    const auto* received = std::get_if<game::GameInfo>(&*message);
    ASSERT_NE(received, nullptr);

    EXPECT_EQ(received->map.width, 2u);
    EXPECT_EQ(received->map.height, 1u);
    EXPECT_EQ(received->map.tiles, info.map.tiles);
    EXPECT_DOUBLE_EQ(received->player_width, 0.5);
    EXPECT_DOUBLE_EQ(received->player_height, 0.5);

    EXPECT_FALSE(client_protocol.receive_message().has_value());
}

TEST(ServerProtocolTest, RejectsInconsistentGameInfoBeforeSendingBytes) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    ServerProtocol server_protocol(connection);
    ClientProtocol client_protocol(client);

    const game::GameInfo info{{2, 1, {game::TileType::Walkable}}, 0.5, 0.5};

    EXPECT_THROW(server_protocol.send_game_info(info), std::invalid_argument);
    connection.shutdown(SHUT_WR);

    EXPECT_FALSE(client_protocol.receive_message().has_value());
}

TEST(ServerProtocolTest, RejectsReceivedMapAboveCellLimit) {
    Socket server("18080");
    Socket client("127.0.0.1", "18080");
    Socket connection = server.accept();

    const std::array<std::uint8_t, 9> message{0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    ASSERT_EQ(connection.sendall(message.data(), message.size()), 9);
    connection.shutdown(SHUT_WR);

    ClientProtocol protocol(client);

    EXPECT_THROW(protocol.receive_message(), std::length_error);
}
