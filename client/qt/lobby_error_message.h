#ifndef CLIENT_QT_LOBBY_ERROR_MESSAGE_H_
#define CLIENT_QT_LOBBY_ERROR_MESSAGE_H_

#include <QString>

#include "lobby/lobby_types.h"

namespace launcher {

QString lobby_error_message(lobby::LobbyError error);

}

#endif
