#include "lobby_worker.h"

#include <stdexcept>
#include <variant>

namespace lobby {

namespace {

// Llama a la operación de LobbyClient que corresponde a cada pedido y arma su respuesta.
class RequestRunner {
private:
    LobbyClient& client;

public:
    explicit RequestRunner(LobbyClient& client): client(client) {}

    LobbyResponse operator()(const ConnectRequest& request) const {
        return ConnectResponse{client.connect(request.host, request.port, request.username)};
    }

    LobbyResponse operator()(const ListWorldsRequest&) const {
        return ListWorldsResponse{client.list_worlds()};
    }

    LobbyResponse operator()(const ListSavedGamesRequest&) const {
        return ListSavedGamesResponse{client.list_saved_games()};
    }

    LobbyResponse operator()(const CreateGameRequest& request) const {
        return CreateGameResponse{
                client.create_game(request.game_name, request.world_id, request.character)};
    }

    LobbyResponse operator()(const FindGameRequest& request) const {
        return FindGameResponse{client.find_game(request.invite_code)};
    }

    LobbyResponse operator()(const JoinGameRequest& request) const {
        return JoinGameResponse{client.join_game(request.invite_code, request.character)};
    }

    LobbyResponse operator()(const ResumeGameRequest& request) const {
        return ResumeGameResponse{client.resume_game(request.game_id)};
    }
};

}

LobbyWorker::LobbyWorker(LobbyClient& client): client(client), requests(MAX_PENDING_REQUESTS) {
    start();
}

void LobbyWorker::run() {
    try {
        const RequestRunner runner(client);
        while (should_keep_running()) {
            const LobbyRequest request = requests.pop();
            responses.push(std::visit(runner, request));
        }
    } catch (const ClosedQueue&) {}
}

void LobbyWorker::submit(const LobbyRequest& request) {
    if (!requests.try_push(request)) {
        throw std::runtime_error("La cola de pedidos al lobby está llena");
    }
}

bool LobbyWorker::try_pop_response(LobbyResponse& response) { return responses.try_pop(response); }

void LobbyWorker::stop() {
    if (!should_keep_running()) {
        return;
    }

    Thread::stop();
    requests.close();
    client.interrupt();
}

LobbyWorker::~LobbyWorker() {
    stop();
    join();
}

}
