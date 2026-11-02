#include "network_lobby_client.h"

#include <exception>
#include <utility>
#include <variant>

#include "lobby_validation.h"

namespace lobby {

namespace {

// Espera el próximo mensaje, que tiene que ser del tipo Message: el servidor contesta siempre en
// el orden MatchResponse, JoinResponse, GameInfo. Si llega otro o se cerró la conexión, devuelve
// nada.
template <typename Message>
std::optional<Message> receive_expected(NetworkClient& network) {
    const auto message = network.receive();
    if (!message || !std::holds_alternative<Message>(*message)) {
        return std::nullopt;
    }
    return std::get<Message>(*message);
}

}

bool NetworkLobbyClient::is_connected() {
    std::lock_guard<std::mutex> lock(mutex);
    return network && !interrupted && !network->has_finished();
}

bool NetworkLobbyClient::send(const ClientRequest& request) {
    std::lock_guard<std::mutex> lock(mutex);
    if (!network || interrupted) {
        return false;
    }

    try {
        return network->try_send(request);
    } catch (const std::exception&) {
        // La conexión ya se cerró: la cola de pedidos está cerrada o NetworkClient se detuvo.
        return false;
    }
}

LobbyResult<GameTicket> NetworkLobbyClient::enter_game(const ClientRequest& request,
                                                       const std::string& game_name,
                                                       const CharacterDraft& character) {
    if (!send(request)) {
        return LobbyError::ServerUnavailable;
    }

    // Se espera sin el lock: así interrupt() puede cerrar la conexión, y eso despierta a receive().
    // Solo este hilo cambia network, así que se puede leer sin el lock.
    const auto match = receive_expected<MatchResponse>(*network);
    if (!match) {
        return LobbyError::ServerUnavailable;
    }

    // Solo pasa al unirse con un código que no es de ninguna partida.
    if (!match->invitation_code) {
        return LobbyError::InvalidInviteCode;
    }

    const auto join = receive_expected<JoinResponse>(*network);
    if (!join) {
        return LobbyError::ServerUnavailable;
    }

    const auto* player_id = std::get_if<game::PlayerId>(&join->result);
    if (!player_id) {
        return LobbyError::GameFull;
    }

    const auto game_info = receive_expected<game::GameInfo>(*network);
    if (!game_info) {
        return LobbyError::ServerUnavailable;
    }

    std::lock_guard<std::mutex> lock(mutex);
    if (interrupted) {
        return LobbyError::ServerUnavailable;
    }

    // Desde acá la conexión es del juego, e interrupt() ya no la toca.
    game_connection =
            GameConnection{std::move(network), *match->invitation_code, *player_id, *game_info};
    return GameTicket{*match->invitation_code, game_name, character, true};
}

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
    if (!is_connected()) {
        return LobbyError::ServerUnavailable;
    }

    return std::vector<WorldInfo>{
            {WORLD_ID, "Mapa del servidor", "El mapa que cargó el servidor al arrancar."}};
}

LobbyResult<std::vector<SavedGameInfo>> NetworkLobbyClient::list_saved_games() {
    if (!is_connected()) {
        return LobbyError::ServerUnavailable;
    }

    return std::vector<SavedGameInfo>{};
}

LobbyResult<GameTicket> NetworkLobbyClient::create_game(const std::string& game_name,
                                                        const std::string& world_id,
                                                        const CharacterDraft& character) {
    const std::string name = trim_spaces(game_name);
    if (!is_valid_game_name(name)) {
        return LobbyError::InvalidGameName;
    }

    if (world_id != WORLD_ID) {
        return LobbyError::UnknownWorld;
    }

    const CharacterDraft draft{trim_spaces(character.name), character.appearance};
    if (auto error = validate_character(draft)) {
        return *error;
    }

    return enter_game(CreateMatchRequest{name, draft.name}, name, draft);
}

LobbyResult<JoinableGameInfo> NetworkLobbyClient::find_game(const std::string& invite_code) {
    if (!is_connected()) {
        return LobbyError::ServerUnavailable;
    }

    const std::string code = trim_spaces(invite_code);
    if (code.empty()) {
        return LobbyError::InvalidInviteCode;
    }

    // El protocolo no deja consultar una partida sin entrar: el código se confirma en join_game(),
    // y hasta entonces no se sabe el nombre ni cuántos jugadores tiene.
    return JoinableGameInfo{code, "", "", 0, 0};
}

LobbyResult<GameTicket> NetworkLobbyClient::join_game(const std::string& invite_code,
                                                      const CharacterDraft& character) {
    // Un código vacío haría fallar al hilo que envía, y eso cerraría la conexión.
    const std::string code = trim_spaces(invite_code);
    if (code.empty()) {
        return LobbyError::InvalidInviteCode;
    }

    const CharacterDraft draft{trim_spaces(character.name), character.appearance};
    if (auto error = validate_character(draft)) {
        return *error;
    }

    // El servidor no le dice el nombre de la partida a quien se une.
    return enter_game(JoinMatchRequest{code, draft.name}, "", draft);
}

LobbyResult<GameTicket> NetworkLobbyClient::resume_game(const std::string& /*game_id*/) {
    if (!is_connected()) {
        return LobbyError::ServerUnavailable;
    }

    return LobbyError::UnknownGame;
}

std::optional<GameConnection> NetworkLobbyClient::take_game_connection() {
    return std::exchange(game_connection, std::nullopt);
}

}
