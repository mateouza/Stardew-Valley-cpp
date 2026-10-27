#ifndef CLIENT_LOBBY_FAKE_LOBBY_CLIENT_H_
#define CLIENT_LOBBY_FAKE_LOBBY_CLIENT_H_

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "lobby_client.h"
#include "lobby_types.h"

namespace lobby {

// Simula en memoria las respuestas del servidor mientras el protocolo no tenga mensajes de lobby.
class FakeLobbyClient: public LobbyClient {
private:
    struct StoredGame {
        std::string id;
        std::string name;
        std::string world_id;
        std::string owner;
        std::string invite_code;
        std::map<std::string, CharacterDraft> characters;
    };

    std::vector<WorldInfo> worlds;
    std::vector<StoredGame> games;
    std::optional<std::string> username;
    unsigned int next_game_number = 1;

    bool has_world(const std::string& world_id) const;
    bool is_member(const StoredGame& game) const;
    StoredGame* find_by_invite_code(const std::string& invite_code);
    StoredGame* find_by_id(const std::string& game_id);
    StoredGame& store_game(const std::string& name, const std::string& world_id,
                           const std::string& owner, const std::string& invite_code);
    GameTicket make_ticket(const StoredGame& game) const;
    void seed_demo_games();

public:
    static constexpr const char* DEMO_INVITE_CODE = "VALLE1";
    static constexpr const char* DEMO_USERS = "Juanceto03, Juanceto01";
    static constexpr std::size_t MAX_PLAYERS_PER_GAME = 4;

    explicit FakeLobbyClient(bool with_demo_games = true);

    // Responde en memoria y nunca bloquea, así que no hay nada que interrumpir.
    void interrupt() override;

    std::optional<LobbyError> connect(const std::string& host, const std::string& port,
                                      const std::string& requested_username) override;

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
