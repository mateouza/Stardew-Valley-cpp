#ifndef CLIENT_LOBBY_NETWORK_LOBBY_CLIENT_H_
#define CLIENT_LOBBY_NETWORK_LOBBY_CLIENT_H_

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "lobby_client.h"
#include "lobby_types.h"
#include "network_client.h"

namespace lobby {

// LobbyClient sobre el protocolo del servidor. El protocolo todavía solo tiene crear y unirse: el
// único mundo es el mapa que carga el servidor, y no hay partidas guardadas.
class NetworkLobbyClient: public LobbyClient {
private:
    // Hasta que el juego empieza a leer, los GameState se acumulan en la cola de mensajes.
    static constexpr unsigned int REQUEST_CAPACITY = 256;
    static constexpr unsigned int MESSAGE_CAPACITY = 256;

    // Protege network e interrupted, que interrupt() usa desde otro hilo.
    std::mutex mutex;
    std::unique_ptr<NetworkClient> network;
    bool interrupted = false;

public:
    void interrupt() override;

    // interrupt() no corta un intento de conexión en curso: al cerrar el launcher, LobbyWorker
    // espera a que termine.
    std::optional<LobbyError> connect(const std::string& host, const std::string& port,
                                      const std::string& username) override;

    LobbyResult<std::vector<WorldInfo>> list_worlds() override;

    LobbyResult<std::vector<SavedGameInfo>> list_saved_games() override;

    LobbyResult<GameTicket> create_game(const std::string& game_name, const std::string& world_id,
                                        const CharacterDraft& character) override;

    LobbyResult<JoinableGameInfo> find_game(const std::string& invite_code) override;

    LobbyResult<GameTicket> join_game(const std::string& invite_code,
                                      const CharacterDraft& character) override;

    LobbyResult<GameTicket> resume_game(const std::string& game_id) override;
};

}

#endif
