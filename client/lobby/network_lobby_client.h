#ifndef CLIENT_LOBBY_NETWORK_LOBBY_CLIENT_H_
#define CLIENT_LOBBY_NETWORK_LOBBY_CLIENT_H_

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "client_request.h"
#include "game_info.h"
#include "game_types.h"
#include "lobby_client.h"
#include "lobby_types.h"
#include "network_client.h"

namespace lobby {

// La conexión ya adentro de una partida, con lo que el lobby leyó para confirmarlo: el código, el
// id del jugador y el GameInfo. El juego sigue con la misma conexión, y lo primero que recibe es un
// GameState.
struct GameConnection {
    std::unique_ptr<NetworkClient> network;
    std::string invite_code;
    game::PlayerId player_id{};
    game::GameInfo game_info;
};

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
    std::optional<GameConnection> game_connection;

    bool is_connected();
    bool send(const ClientRequest& request);
    LobbyResult<GameTicket> enter_game(const ClientRequest& request, const std::string& game_name,
                                       const CharacterDraft& character);

public:
    static constexpr const char* WORLD_ID = "servidor";

    // Corta lo que se esté esperando. Si ya se entró a una partida no hace nada: esa conexión es
    // del juego.
    void interrupt() override;

    // Después de interrupt() ni se intenta. Un intento que ya empezó no se corta: al cerrar el
    // launcher, LobbyWorker espera a que termine.
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

    // La conexión de la partida en la que entró create_game() o join_game(). Se llama una vez que
    // terminó LobbyWorker; la segunda vez devuelve nada.
    std::optional<GameConnection> take_game_connection();
};

}

#endif
