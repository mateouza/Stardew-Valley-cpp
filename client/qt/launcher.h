#ifndef CLIENT_QT_LAUNCHER_H_
#define CLIENT_QT_LAUNCHER_H_

#include <optional>
#include <string>

#include "lobby/lobby_client.h"
#include "lobby/lobby_types.h"

namespace launcher {

struct LauncherOptions {
    std::string host = "localhost";
    std::string port = "8080";
    std::string username;
};

// Muestra el launcher y bloquea hasta que se cierra. Devuelve la partida elegida, o nada si el
// usuario salió. La QApplication se destruye antes de retornar, así SDL puede abrir su ventana.
std::optional<lobby::GameTicket> run_launcher(int& argc, char** argv,
                                              lobby::LobbyClient& lobby_client,
                                              const LauncherOptions& options);

}

#endif
