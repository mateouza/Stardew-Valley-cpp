#include <cstdlib>
#include <exception>
#include <iostream>

#include "lobby/fake_lobby_client.h"

#include "launcher.h"

int main(int argc, char* argv[]) {
    launcher::LauncherOptions options;
    if (argc == 3) {
        options.host = argv[1];
        options.port = argv[2];
    } else if (argc != 1) {
        std::cerr << "Uso: " << argv[0] << " [<host> <puerto>]\n";
        return EXIT_FAILURE;
    }

    try {
        lobby::FakeLobbyClient lobby_client;
        std::cout << "Lobby local de prueba. Código para unirse: "
                  << lobby::FakeLobbyClient::DEMO_INVITE_CODE
                  << ". Usuarios con partidas guardadas: " << lobby::FakeLobbyClient::DEMO_USERS
                  << ".\n";

        const auto ticket = launcher::run_launcher(argc, argv, lobby_client, options);
        if (ticket) {
            std::cout << "Partida elegida: " << ticket->game_name << " (" << ticket->game_id
                      << "), personaje " << ticket->character.name
                      << (ticket->multiplayer ? ", multijugador" : ", un jugador") << '\n';
        }
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
    } catch (...) {
        std::cerr << "Error desconocido\n";
    }

    return EXIT_FAILURE;
}
