#ifndef TESTS_BLOCKING_LOBBY_CLIENT_H_
#define TESTS_BLOCKING_LOBBY_CLIENT_H_

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>

#include "lobby/fake_lobby_client.h"

namespace lobby {

// Lobby falso cuyo connect() se queda bloqueado, como esperando a un servidor lento, hasta que el
// test lo libera o alguien lo interrumpe. Sirve para probar respuestas demoradas y cierres.
class BlockingLobbyClient: public FakeLobbyClient {
private:
    static constexpr std::chrono::seconds MAX_WAIT{2};

    std::mutex mutex;
    std::condition_variable changed;
    bool started = false;
    bool released = false;
    bool interrupted = false;

public:
    std::optional<LobbyError> connect(const std::string& host, const std::string& port,
                                      const std::string& username) override {
        {
            std::unique_lock<std::mutex> lock(mutex);
            started = true;
            changed.notify_all();
            changed.wait(lock, [this] { return released || interrupted; });
            if (interrupted) {
                return LobbyError::ServerUnavailable;
            }
        }
        return FakeLobbyClient::connect(host, port, username);
    }

    void interrupt() override {
        const std::lock_guard<std::mutex> lock(mutex);
        interrupted = true;
        changed.notify_all();
    }

    // Espera, con un plazo máximo, a que connect() haya empezado.
    bool wait_until_started() {
        std::unique_lock<std::mutex> lock(mutex);
        return changed.wait_for(lock, MAX_WAIT, [this] { return started; });
    }

    void release() {
        const std::lock_guard<std::mutex> lock(mutex);
        released = true;
        changed.notify_all();
    }

    bool was_interrupted() {
        const std::lock_guard<std::mutex> lock(mutex);
        return interrupted;
    }
};

}

#endif
