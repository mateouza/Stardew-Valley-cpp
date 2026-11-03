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

struct GameConnection {
    std::unique_ptr<NetworkClient> network;
    std::string invite_code;
    game::PlayerId player_id{};
    game::GameInfo game_info;
};

class NetworkLobbyClient: public LobbyClient {
private:
    static constexpr unsigned int REQUEST_CAPACITY = 256;
    static constexpr unsigned int MESSAGE_CAPACITY = 256;

    // Protege network e interrupted.
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

    void interrupt() override;

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

    std::optional<GameConnection> take_game_connection();
};

}

#endif
