#ifndef CLIENT_LOBBY_LOBBY_WORKER_H_
#define CLIENT_LOBBY_LOBBY_WORKER_H_

#include "lobby_client.h"
#include "lobby_requests.h"
#include "queue.h"
#include "thread.h"

namespace lobby {

class LobbyWorker final: public Thread {
private:
    LobbyClient& client;
    Queue<LobbyRequest> requests;
    Queue<LobbyResponse> responses;

    void run() override;

public:
    static constexpr unsigned int MAX_PENDING_REQUESTS = 8;

    explicit LobbyWorker(LobbyClient& client);

    void submit(const LobbyRequest& request);

    bool try_pop_response(LobbyResponse& response);

    void stop() override;

    ~LobbyWorker() override;
};

}

#endif
