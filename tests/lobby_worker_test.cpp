#include "lobby/lobby_worker.h"

#include <chrono>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>
#include <variant>
#include <vector>

#include "gtest/gtest.h"
#include "lobby/fake_lobby_client.h"

#include "blocking_lobby_client.h"

namespace lobby {

namespace {

// Consulta sin bloquear, como hace la ventana, hasta que llega una respuesta o vence el plazo.
std::optional<LobbyResponse> wait_for_response(LobbyWorker& worker) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    LobbyResponse response;
    while (std::chrono::steady_clock::now() < deadline) {
        if (worker.try_pop_response(response)) {
            return response;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return std::nullopt;
}

}

TEST(LobbyWorkerTest, RunsRequestsInOrder) {
    FakeLobbyClient client;
    LobbyWorker worker(client);

    worker.submit(ListWorldsRequest{});
    worker.submit(ConnectRequest{"localhost", "8080", "Juanceto02"});
    worker.submit(ListWorldsRequest{});

    const auto before_connecting = wait_for_response(worker);
    ASSERT_TRUE(before_connecting.has_value());
    ASSERT_TRUE(std::holds_alternative<ListWorldsResponse>(*before_connecting));
    EXPECT_TRUE(std::holds_alternative<LobbyError>(
            std::get<ListWorldsResponse>(*before_connecting).result));

    const auto connected = wait_for_response(worker);
    ASSERT_TRUE(connected.has_value());
    ASSERT_TRUE(std::holds_alternative<ConnectResponse>(*connected));
    EXPECT_EQ(std::get<ConnectResponse>(*connected).error, std::nullopt);

    const auto after_connecting = wait_for_response(worker);
    ASSERT_TRUE(after_connecting.has_value());
    ASSERT_TRUE(std::holds_alternative<ListWorldsResponse>(*after_connecting));
    const auto& worlds = std::get<ListWorldsResponse>(*after_connecting).result;
    ASSERT_TRUE(std::holds_alternative<std::vector<WorldInfo>>(worlds));
    EXPECT_EQ(std::get<std::vector<WorldInfo>>(worlds).size(), 3u);
}

TEST(LobbyWorkerTest, DoesNotBlockWhileARequestIsPending) {
    BlockingLobbyClient client;
    LobbyWorker worker(client);
    worker.submit(ConnectRequest{"localhost", "8080", "Juanceto02"});
    ASSERT_TRUE(client.wait_until_started());

    LobbyResponse response;
    EXPECT_FALSE(worker.try_pop_response(response));

    client.release();
    const auto connected = wait_for_response(worker);
    ASSERT_TRUE(connected.has_value());
    ASSERT_TRUE(std::holds_alternative<ConnectResponse>(*connected));
    EXPECT_EQ(std::get<ConnectResponse>(*connected).error, std::nullopt);
}

TEST(LobbyWorkerTest, DestroyingInterruptsThePendingRequestAndJoins) {
    BlockingLobbyClient client;
    auto worker = std::make_unique<LobbyWorker>(client);
    worker->submit(ConnectRequest{"localhost", "8080", "Juanceto02"});
    ASSERT_TRUE(client.wait_until_started());

    // Si no interrumpiera, el destructor esperaría para siempre y vencería el timeout de ctest.
    worker.reset();

    EXPECT_TRUE(client.was_interrupted());
}

TEST(LobbyWorkerTest, SubmitNeverWaitsAndFailsWhenTheQueueIsFull) {
    BlockingLobbyClient client;
    LobbyWorker worker(client);
    worker.submit(ConnectRequest{"localhost", "8080", "Juanceto02"});
    ASSERT_TRUE(client.wait_until_started());

    // El worker quedó bloqueado en el primer pedido: los siguientes se acumulan en la cola.
    for (unsigned int i = 0; i < LobbyWorker::MAX_PENDING_REQUESTS; ++i) {
        worker.submit(ListWorldsRequest{});
    }

    EXPECT_THROW(worker.submit(ListWorldsRequest{}), std::runtime_error);
}

TEST(LobbyWorkerTest, CanBeStoppedMoreThanOnce) {
    FakeLobbyClient client;
    LobbyWorker worker(client);

    worker.stop();
    worker.stop();

    LobbyResponse response;
    EXPECT_FALSE(worker.try_pop_response(response));
}

}
