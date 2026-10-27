#ifndef CLIENT_LOBBY_LOBBY_CLIENT_H_
#define CLIENT_LOBBY_LOBBY_CLIENT_H_

#include <optional>
#include <string>
#include <vector>

#include "lobby_types.h"

namespace lobby {

// Pedidos al lobby del servidor. Las operaciones son bloqueantes y las llama un único hilo, el de
// LobbyWorker, nunca el de la interfaz. No lanzan excepciones: si se pierde la conexión devuelven
// LobbyError::ServerUnavailable. Cada implementación valida sus parámetros.
class LobbyClient {
public:
    virtual ~LobbyClient() = default;

    // Se llama desde otro hilo para cerrar mientras una operación puede estar bloqueada. Es
    // definitivo: la operación en curso y las siguientes tienen que terminar enseguida (por
    // ejemplo, haciendo shutdown del socket).
    virtual void interrupt() = 0;

    virtual std::optional<LobbyError> connect(const std::string& host, const std::string& port,
                                              const std::string& username) = 0;

    virtual LobbyResult<std::vector<WorldInfo>> list_worlds() = 0;

    virtual LobbyResult<std::vector<SavedGameInfo>> list_saved_games() = 0;

    virtual LobbyResult<GameTicket> create_game(const std::string& game_name,
                                                const std::string& world_id,
                                                const CharacterDraft& character) = 0;

    virtual LobbyResult<JoinableGameInfo> find_game(const std::string& invite_code) = 0;

    virtual LobbyResult<GameTicket> join_game(const std::string& invite_code,
                                              const CharacterDraft& character) = 0;

    // Provisorio: falta acordar cómo se acredita que la conexión puede recuperar ese personaje.
    virtual LobbyResult<GameTicket> resume_game(const std::string& game_id) = 0;
};

}

#endif
