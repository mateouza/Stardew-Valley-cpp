#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

#include "lobby/fake_lobby_client.h"
#include "lobby/network_lobby_client.h"
#include "sdl/sdl_client.h"

#include "launcher.h"

namespace {

int run_demo(int& argc, char** argv, const launcher::LauncherOptions& options) {
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
}

int run_with_server(int& argc, char** argv, const launcher::LauncherOptions& options) {
    lobby::NetworkLobbyClient lobby_client;
    if (!launcher::run_launcher(argc, argv, lobby_client, options)) {
        return EXIT_SUCCESS;
    }

    const auto connection = lobby_client.take_game_connection();
    if (!connection) {
        std::cerr << "El lobby no entrego la conexion de la partida\n";
        return EXIT_FAILURE;
    }

    std::cout << "Entraste a la partida con código " << connection->invite_code << " como jugador "
              << connection->player_id << ".\n";

    NetworkClient& network = *connection->network;
    const int result = ejecutar_cliente_sdl(network, connection->player_id, connection->game_info,
                                            connection->invite_code);

    network.stop();
    network.join();
    network.rethrow_if_failed();

    return result;
}

}

int main(int argc, char* argv[]) {
    const bool demo = argc > 1 && std::string(argv[1]) == "--demo";
    const int first_address_arg = demo ? 2 : 1;

    launcher::LauncherOptions options;
    if (argc == first_address_arg + 2) {
        options.host = argv[first_address_arg];
        options.port = argv[first_address_arg + 1];
    } else if (argc != first_address_arg) {
        std::cerr << "Uso: " << argv[0] << " [--demo] [<host> <puerto>]\n";
        return EXIT_FAILURE;
    }

    try {
        return demo ? run_demo(argc, argv, options) : run_with_server(argc, argv, options);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
    } catch (...) {
        std::cerr << "Error desconocido\n";
    }

    return EXIT_FAILURE;
}
