#ifndef CLIENT_LOBBY_LOBBY_REQUESTS_H_
#define CLIENT_LOBBY_LOBBY_REQUESTS_H_

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "lobby_types.h"

namespace lobby {

// Un pedido por operación de LobbyClient, con sus parámetros, para pasarlo entre hilos por una
// cola. Cada uno tiene su respuesta con el mismo nombre, que trae lo que devolvió LobbyClient.

struct ConnectRequest {
    std::string host;
    std::string port;
    std::string username;
};

struct ListWorldsRequest {};

struct ListSavedGamesRequest {};

struct CreateGameRequest {
    std::string game_name;
    std::string world_id;
    CharacterDraft character;
};

struct FindGameRequest {
    std::string invite_code;
};

struct JoinGameRequest {
    std::string invite_code;
    CharacterDraft character;
};

struct ResumeGameRequest {
    std::string game_id;
};

using LobbyRequest =
        std::variant<ConnectRequest, ListWorldsRequest, ListSavedGamesRequest, CreateGameRequest,
                     FindGameRequest, JoinGameRequest, ResumeGameRequest>;

struct ConnectResponse {
    std::optional<LobbyError> error;
};

struct ListWorldsResponse {
    LobbyResult<std::vector<WorldInfo>> result;
};

struct ListSavedGamesResponse {
    LobbyResult<std::vector<SavedGameInfo>> result;
};

struct CreateGameResponse {
    LobbyResult<GameTicket> result;
};

struct FindGameResponse {
    LobbyResult<JoinableGameInfo> result;
};

struct JoinGameResponse {
    LobbyResult<GameTicket> result;
};

struct ResumeGameResponse {
    LobbyResult<GameTicket> result;
};

using LobbyResponse =
        std::variant<ConnectResponse, ListWorldsResponse, ListSavedGamesResponse,
                     CreateGameResponse, FindGameResponse, JoinGameResponse, ResumeGameResponse>;

}

#endif
