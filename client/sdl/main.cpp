#include <cstdlib>
#include <iostream>
#include <chrono>
#include <optional>
#include <thread>
#include <string>

#include <SDL2pp/Exception.hh>
#include <SDL2pp/SDL.hh>
#include <SDL2pp/SDLImage.hh>
#include <SDL_image.h>

#include "client_protocol.h"
#include "client_request.h"
#include "network_client.h"
#include "server_message.h"
#include "sdl_client.h"


namespace {

constexpr std::chrono::seconds ESPERA_RESPUESTA{8};
constexpr unsigned int CAPACIDAD_PEDIDOS = 256;
constexpr unsigned int CAPACIDAD_MENSAJES = 256;

}

struct HandshakeResult {
    game::PlayerId id_propio;
    game::GameInfo info_partida; 
    std::string codigo_invitacion;   
};

void mostrar_uso(const char* nombre_programa){
    std::cerr << "Uso:\n"
            << "  " << nombre_programa << " <host> <puerto> crear <nombre_partida> <nombre_jugador>\n"
            << "  " << nombre_programa << " <host> <puerto> unirse <codigo> <nombre_jugador>\n";
}

template <typename T>
std::optional<T> esperar_mensaje(NetworkClient& cliente, std::chrono::steady_clock::duration timeout) {
    const std::chrono::steady_clock::time_point limite = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < limite) {
        if (cliente.has_finished()) {
            return std::nullopt;
        }
        std::optional<ServerMessage> mensaje = cliente.try_receive();
        if (mensaje) {
            if (T* encontrado = std::get_if<T>(&*mensaje)) {
                return *encontrado;
            }
            continue; // se ignoran los mensajes de otro tipo
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return std::nullopt;
}


void enviar_con_reintento(NetworkClient& cliente, const ClientRequest& pedido) {
    while (!cliente.try_send(pedido)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

std::optional<HandshakeResult> realizar_handshake(NetworkClient& cliente, const std::string& comando, 
    const std::string& argumento_partida,
    const std::string& nombre_jugador) {
        if (comando == "crear") {
            enviar_con_reintento(cliente, CreateMatchRequest{argumento_partida, nombre_jugador});
        } else {
            enviar_con_reintento(cliente, JoinMatchRequest{argumento_partida, nombre_jugador});
        }

        std::optional<MatchResponse> respuesta_partida = esperar_mensaje<MatchResponse>(cliente, ESPERA_RESPUESTA);

        if (!respuesta_partida) {
            std::cerr << (cliente.has_finished() ? "Se perdió la conexión con el servidor\n" : "El servidor no respondio a tiempo a la solicitud de partida\n");
            return std::nullopt;
        }

        if (!respuesta_partida->invitation_code) {
            std::cerr << "No se encontro la partida\n";
            return std::nullopt;
        }

        const std::string codigo_invitacion = *respuesta_partida->invitation_code;
        std::cout << "Codigo de invitacion: " << codigo_invitacion << '\n';

        std::optional<JoinResponse> respuesta_union = esperar_mensaje<JoinResponse>(cliente, ESPERA_RESPUESTA);

        if (!respuesta_union) {
            std::cerr << (cliente.has_finished() ? "Se perdió la conexión con el servidor\n" : "El servidor no respondio a tiempo a la solicitud de union\n");
            return std::nullopt;
        }

        const game::PlayerId* id_propio = std::get_if<game::PlayerId>(&respuesta_union->result);

        if (!id_propio) {
            std::cerr << "No se pudo ingresar a la partida: esta llena\n";            
            return std::nullopt;
        }

        std::optional<game::GameInfo> info_partida = esperar_mensaje<game::GameInfo>(cliente, ESPERA_RESPUESTA);
        if (!info_partida) {
            std::cerr << (cliente.has_finished() ? "Se perdió la conexión con el servidor\n" : "El servidor no respondio a tiempo con la informacion de la partida\n");
            return std::nullopt;
        }

        return HandshakeResult{*id_propio, *info_partida, codigo_invitacion};
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        mostrar_uso(argv[0]);
        return EXIT_FAILURE;
    }
    const std::string host = argv[1];
    const std::string puerto = argv[2];
    const std::string comando = argv[3];
    const std::string argumento_partida = argv[4];
    const std::string nombre_jugador = argv[5];

    if (comando != "crear" && comando != "unirse") {
        mostrar_uso(argv[0]);
        return EXIT_FAILURE;
    }

    try {
        NetworkClient client(host.c_str(), puerto.c_str(), CAPACIDAD_PEDIDOS, CAPACIDAD_MENSAJES);
        client.start();

        std::optional<HandshakeResult> resultado = realizar_handshake(client, comando, argumento_partida, nombre_jugador);

        int codigo_salida = EXIT_FAILURE;
        if (resultado) {
            codigo_salida = ejecutar_cliente_sdl(client, resultado->id_propio, resultado->info_partida, resultado->codigo_invitacion);
        }

        client.stop();
        client.join();
        client.rethrow_if_failed();

        return codigo_salida;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
}