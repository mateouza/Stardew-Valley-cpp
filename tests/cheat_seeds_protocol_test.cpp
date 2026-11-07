#include <array>
#include <cstdint>
#include <stdexcept>
#include <utility>

#include <sys/socket.h>

#include "gtest/gtest.h"

#include "client_protocol.h"
#include "client_session.h"
#include "server_protocol.h"
#include "socket.h"

namespace {

class CheatSeedsProtocolTest: public ::testing::Test {
protected:
    Socket listener{"18080"};
    Socket client{"127.0.0.1", "18080"};
    Socket connection{listener.accept()};
    ClientProtocol client_protocol{client};
    ServerProtocol server_protocol{connection};
};

TEST_F(CheatSeedsProtocolTest, SendsOnlyTheExpectedRequestByte) {
    client_protocol.send_cheat_seeds();
    client.shutdown(SHUT_WR);

    std::uint8_t opcode{};
    ASSERT_EQ(connection.recvall(&opcode, sizeof(opcode)), 1);
    EXPECT_EQ(opcode, 0x0E);
    EXPECT_EQ(connection.recvall(&opcode, sizeof(opcode)), 0);
}

TEST_F(CheatSeedsProtocolTest, ReceiverAttachesItsOwnSessionIdToTheCheat) {
    Queue<MatchCommand> commands{16};
    auto channel = std::make_shared<SessionChannel>(7, 16);
    ClientSession session(std::move(connection));
    session.start(commands, "Mateo", channel);

    client_protocol.send_cheat_seeds();
    const auto admission = commands.pop();
    const auto command = commands.pop();

    session.stop();
    session.join();
    EXPECT_NO_THROW(session.rethrow_if_failed());

    EXPECT_TRUE(std::holds_alternative<AddPlayerCommand>(admission));
    const auto* cheat = std::get_if<CheatSeedsCommand>(&command);
    ASSERT_NE(cheat, nullptr);
    EXPECT_EQ(cheat->session_id, 7u);
}

TEST_F(CheatSeedsProtocolTest, RejectsInvalidResultsBeforeWritingAnyBytes) {
    for (const auto result:
         {game::InventoryResult::UnknownPlayer, game::InventoryResult::InvalidItem,
          static_cast<game::InventoryResult>(256), static_cast<game::InventoryResult>(-256)}) {
        EXPECT_THROW(server_protocol.send_cheat_seeds_response(CheatSeedsResponse{result}),
                     std::invalid_argument);
    }

    connection.shutdown(SHUT_WR);
    EXPECT_FALSE(client_protocol.receive_message().has_value());
}

TEST_F(CheatSeedsProtocolTest, RejectsUnknownResponseStatus) {
    const std::array<std::uint8_t, 2> message{0x0F, 2};
    ASSERT_EQ(connection.sendall(message.data(), message.size()), 2);
    connection.shutdown(SHUT_WR);

    EXPECT_THROW(client_protocol.receive_message(), std::runtime_error);
}

TEST_F(CheatSeedsProtocolTest, RejectsResponseWithoutItsStatusByte) {
    const std::uint8_t opcode = 0x0F;
    ASSERT_EQ(connection.sendall(&opcode, sizeof(opcode)), 1);
    connection.shutdown(SHUT_WR);

    EXPECT_THROW(client_protocol.receive_message(), std::runtime_error);
}

}
