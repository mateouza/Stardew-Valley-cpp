#include "network_lobby_client.h"

#include <exception>
#include <utility>

#include "lobby_validation.h"

namespace lobby {

void NetworkLobbyClient::interrupt() {
    std::lock_guard<std::mutex> lock(mutex);
    interrupted = true;
    if (network) {
        network->stop();
    }
}

std::optional<LobbyError> NetworkLobbyClient::connect(const std::string& host,
                                                      const std::string& port,
                                                      const std::string& username) {
    if (trim_spaces(host).empty() || !is_valid_port(port)) {
        return LobbyError::ServerUnavailable;
    }

    if (!is_valid_username(username)) {
        return LobbyError::InvalidUsername;
    }

    // Conectarse bloquea, así que se hace sin el lock para no demorar a interrupt().
    std::unique_ptr<NetworkClient> connection;
    try {
        connection = std::make_unique<NetworkClient>(host.c_str(), port.c_str(), REQUEST_CAPACITY,
                                                     MESSAGE_CAPACITY);
        connection->start();
    } catch (const std::exception&) {
        return LobbyError::ServerUnavailable;
    }

    // Se declara después de connection, así se libera antes: la conexión anterior, si había, se
    // cierra sin el lock.
    std::lock_guard<std::mutex> lock(mutex);
    if (interrupted) {
        return LobbyError::ServerUnavailable;
    }

    std::swap(network, connection);
    return std::nullopt;
}

LobbyResult<std::vector<WorldInfo>> NetworkLobbyClient::list_worlds() {
    return LobbyError::ServerUnavailable;
}

LobbyResult<std::vector<SavedGameInfo>> NetworkLobbyClient::list_saved_games() {
    return LobbyError::ServerUnavailable;
}

LobbyResult<GameTicket> NetworkLobbyClient::create_game(const std::string& /*game_name*/,
                                                        const std::string& /*world_id*/,
                                                        const CharacterDraft& /*character*/) {
    return LobbyError::ServerUnavailable;
}

LobbyResult<JoinableGameInfo> NetworkLobbyClient::find_game(const std::string& /*invite_code*/) {
    return LobbyError::ServerUnavailable;
}

LobbyResult<GameTicket> NetworkLobbyClient::join_game(const std::string& /*invite_code*/,
                                                      const CharacterDraft& /*character*/) {
    return LobbyError::ServerUnavailable;
}

LobbyResult<GameTicket> NetworkLobbyClient::resume_game(const std::string& /*game_id*/) {
    return LobbyError::ServerUnavailable;
}

}
