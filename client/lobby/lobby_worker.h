#ifndef CLIENT_LOBBY_LOBBY_WORKER_H_
#define CLIENT_LOBBY_LOBBY_WORKER_H_

#include "lobby_client.h"
#include "lobby_requests.h"
#include "queue.h"
#include "thread.h"

namespace lobby {

// Hilo que ejecuta los pedidos al lobby, de a uno y en orden. Quien pide (la ventana de Qt) no se
// bloquea nunca: encola el pedido y después consulta si llegó la respuesta. Arranca al construirse;
// al destruirse interrumpe el pedido en curso y espera a que el hilo termine.
class LobbyWorker final: public Thread {
private:
    LobbyClient& client;
    Queue<LobbyRequest> requests;
    Queue<LobbyResponse> responses;

    void run() override;

public:
    // La ventana manda de a un pedido, porque deshabilita las pantallas mientras espera.
    static constexpr unsigned int MAX_PENDING_REQUESTS = 8;

    explicit LobbyWorker(LobbyClient& client);

    // No espera nunca. Si ya hay MAX_PENDING_REQUESTS sin atender, lanza std::runtime_error.
    void submit(const LobbyRequest& request);

    // Devuelve false si todavía no hay ninguna respuesta.
    bool try_pop_response(LobbyResponse& response);

    // Cierra la cola de pedidos e interrumpe el que esté en curso. Llamarlo de nuevo no hace nada.
    void stop() override;

    ~LobbyWorker() override;
};

}

#endif
