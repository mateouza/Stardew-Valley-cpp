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
#include "core/input_handler.h"
#include "render/renderer.h"
#include "render/sdl_window.h"
#include "render/texture_manager.h"
#include "state/client_state.h"


namespace {

constexpr double PIXELS_POR_CELDA = 32.0;
constexpr std::chrono::seconds ESPERA_RESPUESTA{8};
constexpr unsigned int CAPACIDAD_PEDIDOS = 256;
constexpr unsigned int CAPACIDAD_MENSAJES = 256;

}

struct HandshakeResult {
    game::PlayerId id_propio;
    game::GameInfo info_partida;    
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

        std::cout << "Codigo de invitacion: " << *respuesta_partida->invitation_code << '\n';

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

        return HandshakeResult{*id_propio, *info_partida};
}

void actualizar_direccion_pendiente(const game::MovementDirection& direccion_actual,
    game::MovementDirection& ultima_direccion_enviada,
    std::optional<game::MovementDirection>& pendiente) {
    if (direccion_actual.horizontal != ultima_direccion_enviada.horizontal ||
        direccion_actual.vertical != ultima_direccion_enviada.vertical) {
        pendiente = direccion_actual;
    } else {
        pendiente.reset();
    }
}

void procesar_mensajes_entrantes(NetworkClient& cliente, ClientState& estado){
    std::optional<ServerMessage> mensaje = cliente.try_receive();
    while (mensaje) {
        game::GameState* nuevo_estado = std::get_if<game::GameState>(&*mensaje);
        if (nuevo_estado) {
            estado.actualizar(*nuevo_estado);
        } 
        mensaje = cliente.try_receive();
    }
}

void dibujar_mapa(Renderer& renderer, const game::MapData& mapa) {
    const int celda_px = static_cast<int>(PIXELS_POR_CELDA);

    for (std::uint32_t fila = 0; fila < mapa.height; ++fila) {
        for (std::uint32_t columna = 0; columna < mapa.width; ++columna) {
            const std::size_t indice = static_cast<std::size_t>(fila) * mapa.width + columna;
            const int x = static_cast<int>(columna) * celda_px;
            const int y = static_cast<int>(fila) * celda_px;

            if (mapa.tiles[indice] == game::TileType::Blocked) {
                renderer.dibujar_rectangulo(x, y, celda_px, celda_px, 90, 60, 40); //marron
            } else {
                renderer.dibujar_rectangulo(x, y, celda_px, celda_px, 120, 200, 120); //verde
            }
        }
    }
}

void dibujar_jugadores(Renderer& renderer, SDL2pp::Texture& textura, const ClientState& estado_cliente, int ancho_sprite, int alto_sprite) {
    const std::map<game::PlayerId, game::PlayerState>& jugadores = estado_cliente.obtener_jugadores();
    for (const std::pair<const game::PlayerId, game::PlayerState>& entrada_mapa : jugadores) {
        const game::PlayerState& jugador = entrada_mapa.second;

        const int centro_x_px = static_cast<int>(jugador.position.x * PIXELS_POR_CELDA);
        const int centro_y_px = static_cast<int>(jugador.position.y * PIXELS_POR_CELDA);
        const int x = centro_x_px - ancho_sprite / 2;
        const int y = centro_y_px - alto_sprite / 2;

        if (jugador.id == estado_cliente.obtener_id_propio()) {
            renderer.dibujar(textura, x, y, ancho_sprite, alto_sprite, 255, 255, 255); //blanco
        } else {
            renderer.dibujar(textura, x, y, ancho_sprite, alto_sprite, 67, 38, 255); //azul
        }
    }
}

void ejecutar_bucle_juego(NetworkClient& cliente, ClientState& estado, InputHandler& input_handler,
                          Renderer& renderer, SDL2pp::Texture& textura, const game::MapData& mapa,
                          int ancho_sprite, int alto_sprite) {
    game::MovementDirection ultima_direccion_enviada{0, 0};
    std::optional<game::MovementDirection> pendiente;

    bool corriendo = true;
    while(corriendo){
        if (cliente.has_finished()){
            corriendo = false;  
            continue;
        }
        
        InputResultado entrada = input_handler.procesar_eventos();
        if (entrada.salir){
            corriendo = false;
            continue;
        }

        actualizar_direccion_pendiente(entrada.direccion, ultima_direccion_enviada, pendiente);

        if (pendiente) {
            if(cliente.try_send(game::MoveRequest{*pendiente})) {
                ultima_direccion_enviada = *pendiente;
                pendiente.reset();
            }
        }

        procesar_mensajes_entrantes(cliente, estado);

        renderer.limpiar();
        dibujar_mapa(renderer, mapa);
        dibujar_jugadores(renderer, textura, estado, ancho_sprite, alto_sprite);
        renderer.presentar();
    }
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

        if (!resultado) {
            return EXIT_FAILURE;
        }

        ClientState estado_cliente(resultado->id_propio);

        SDL2pp::SDL sdl(SDL_INIT_VIDEO);
        SDL2pp::SDLImage sdl_image(IMG_INIT_PNG);

        SDLWindow ventana("Stardew Valley", 800, 600, false);
        Renderer renderer(ventana.obtener_handle());
        TextureManager texturas(renderer);
        SDL2pp::Texture& textura = texturas.obtener_textura("assets/rock.png");

        const int ancho_sprite_px = static_cast<int>(resultado->info_partida.player_width * PIXELS_POR_CELDA);
        const int alto_sprite_px = static_cast<int>(resultado->info_partida.player_height * PIXELS_POR_CELDA);

        InputHandler input_handler;
        
        ejecutar_bucle_juego(client, estado_cliente, input_handler, renderer, textura, resultado->info_partida.map, ancho_sprite_px, alto_sprite_px);

        client.stop();
        client.join();
        client.rethrow_if_failed();

    } catch (const SDL2pp::Exception& e) {
        std::cerr << "Error de SDL (" << e.GetSDLFunction() << "): " << e.GetSDLError() << '\n';
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}