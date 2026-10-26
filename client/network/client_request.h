#ifndef CLIENT_REQUEST_H
#define CLIENT_REQUEST_H

#include <variant>

#include "game_command.h"
#include "lobby_request.h"

using ClientRequest = std::variant<CreateMatchRequest, JoinMatchRequest, game::MoveRequest>;

#endif
