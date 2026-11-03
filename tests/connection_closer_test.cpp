#include "connection_closer.h"

#include <exception>
#include <stdexcept>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

namespace {

constexpr const char* PORT = "18081";

class ConnectionCloserTest: public ::testing::Test {
protected:
    Socket listener{PORT};
    Socket client{"127.0.0.1", PORT};
    Socket server{listener.accept()};
    Queue<ClientRequest> requests{4};
    Queue<ServerMessage> messages{4};
    ConnectionCloser closer{client, requests, messages};
};

}

TEST_F(ConnectionCloserTest, StartsOpen) {
    EXPECT_FALSE(closer.is_closed());
    EXPECT_NO_THROW(closer.rethrow_if_failed());
}

TEST_F(ConnectionCloserTest, ClosesBothQueuesAndTheSocket) {
    closer.close();

    EXPECT_TRUE(closer.is_closed());
    EXPECT_THROW(requests.pop(), ClosedQueue);
    EXPECT_THROW(messages.pop(), ClosedQueue);

    char byte = 0;
    EXPECT_EQ(server.recvall(&byte, sizeof(byte)), 0);
}

TEST_F(ConnectionCloserTest, ClosingAgainDoesNothing) {
    closer.close();

    EXPECT_NO_THROW(closer.close());
    EXPECT_NO_THROW(closer.rethrow_if_failed());
}

TEST_F(ConnectionCloserTest, ClosesOnceWhenSeveralThreadsCloseAtOnce) {
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([this] { closer.close(); });
    }
    for (std::thread& thread: threads) {
        thread.join();
    }

    EXPECT_TRUE(closer.is_closed());
    EXPECT_NO_THROW(closer.rethrow_if_failed());
}

TEST_F(ConnectionCloserTest, KeepsOnlyTheFirstCause) {
    closer.close(std::make_exception_ptr(std::invalid_argument("primero")));
    closer.close(std::make_exception_ptr(std::length_error("segundo")));

    EXPECT_THROW(closer.rethrow_if_failed(), std::invalid_argument);
}

TEST_F(ConnectionCloserTest, IgnoresACauseThatArrivesAfterClosing) {
    closer.close();
    closer.close(std::make_exception_ptr(std::runtime_error("efecto del shutdown")));

    EXPECT_NO_THROW(closer.rethrow_if_failed());
}
