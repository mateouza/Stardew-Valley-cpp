#ifndef COMMON_LOBBY_REQUEST_H
#define COMMON_LOBBY_REQUEST_H

#include <string>
#include <variant>

struct CreateMatchRequest {
    std::string match_name;
    std::string player_name;
};

struct JoinMatchRequest {
    std::string invitation_code;
    std::string player_name;
};

using LobbyRequest = std::variant<CreateMatchRequest, JoinMatchRequest>;

#endif