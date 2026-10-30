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
#include "state/client_state.h"

namespace {

constexpr double PIXELS_POR_CELDA = 32.0;
constexpr std::chrono::seconds ESPERA_RESPUESTA{8};
constexpr unsigned int CAPACIDAD_PEDIDOS = 256;
constexpr unsigned int CAPACIDAD_MENSAJES = 256;

}

void mostrar_uso(const char* nombre_programa){
    std::cerr << "Uso:\n"
            << "  " << nombre_programa << " <host> <puerto> crear <nombre_partida> <nombre_jugador>\n"
            << "  " << nombre_programa << " <host> <puerto> unirse <codigo> <nombre_jugador>\n";
}

template <typename T>
std::optional<T> esperar_mensaje(NetworkClient& cliente, std::chrono::steady_clock::duration timeout) {
    const std::chrono::steady_clock::time_point limite = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < limite) {
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

int main(int argc, char* argv[]) {
    if (argc != 6) {
        mostrar_uso(argv[0]);
        return EXIT_FAILURE;
    }
    const std::string host = argv[1];
    const std::string puerto = argv[2];
    const std::string comando = argv[3];

    try {
        NetworkClient client(host.c_str(), puerto.c_str(), CAPACIDAD_PEDIDOS, CAPACIDAD_MENSAJES);
        client.start();

        if (comando == "crear"){
            enviar_con_reintento(client, CreateMatchRequest{argv[4], argv[5]});
        } else if (comando == "unirse") {
            enviar_con_reintento(client, JoinMatchRequest{argv[4], argv[5]});
        } else {
            mostrar_uso(argv[0]);
            return EXIT_FAILURE;
        }

        std::optional<MatchResponse> respuesta_partida = esperar_mensaje<MatchResponse>(client, ESPERA_RESPUESTA);
        if (!respuesta_partida) {
            std::cerr << "El servidor no respondio a tiempo a la solicitud de partida\n";
            return EXIT_FAILURE;
        }
        if (!respuesta_partida->invitation_code) {
            std::cerr << "No se encontro la partida\n";
            return EXIT_FAILURE;
        }
        std::cout << "Codigo de invitacion: " << *respuesta_partida->invitation_code << '\n';


        std::optional<JoinResponse> respuesta_union = esperar_mensaje<JoinResponse>(client, ESPERA_RESPUESTA);
        if (!respuesta_union) {
            std::cerr << "El servidor no respondio a tiempo a la solicitud de union\n";
            return EXIT_FAILURE;
        }
        const auto* id_propio = std::get_if<game::PlayerId>(&respuesta_union->result);
        if (!id_propio) {
            std::cerr << "No se pudo ingresar a la partida: esta llena\n";            
            return EXIT_FAILURE;
        }

        ClientState estado_cliente(*id_propio);

        SDL2pp::SDL sdl(SDL_INIT_VIDEO);
        SDL2pp::SDLImage sdl_image(IMG_INIT_PNG);

        SDLWindow ventana("Stardew Valley", 800, 600, false);
        Renderer renderer(ventana.obtener_handle());
        SDL2pp::Texture textura = renderer.cargar_textura("assets/rock.png");

        //TO-DO: actualizar con las dimensiones reales del cuerpo del jugador
        constexpr int ANCHO_SPRITE_PX = 48; //constexpr poruqe es cte y se evalua en tiempo de compilacion
        constexpr int ALTO_SPRITE_PX = 48;

        InputHandler input_handler;
        bool corriendo = true;

        game::MovementDirection ultima_direccion_enviada{0, 0};
        std::optional<game::MovementDirection> pendiente;
        /*se guarda en pendiente si la direccion cambio respecto a
        la ultima enviada
        siempre se actualiza con la mas reciente en vez de acumular todas
        */

        while(corriendo){
            if (client.has_finished()){
                corriendo = false;
                continue;
            }
            InputResultado entrada = input_handler.procesar_eventos();

            if (entrada.salir){
                corriendo = false;
                continue;
            }

            if (entrada.direccion.horizontal != ultima_direccion_enviada.horizontal ||
                entrada.direccion.vertical != ultima_direccion_enviada.vertical) {
                pendiente  = entrada.direccion;
            } else {
                pendiente.reset();
            }
            
            if (pendiente) {
                if(client.try_send(game::MoveRequest{*pendiente})) {
                    ultima_direccion_enviada = *pendiente;
                    pendiente.reset();
                }
            }

            std::optional<ServerMessage> mensaje = client.try_receive();
            while (mensaje) {
                game::GameState* nuevo_estado = std::get_if<game::GameState>(&*mensaje);
                if (nuevo_estado) {
                    estado_cliente.actualizar(*nuevo_estado);
                }
                mensaje = client.try_receive();
            }

            renderer.limpiar();

            const std::map<game::PlayerId, game::PlayerState>& jugadores = estado_cliente.obtener_jugadores();
            for (const std::pair<const game::PlayerId, game::PlayerState>& entrada_mapa : jugadores) {
                const game::PlayerState& jugador = entrada_mapa.second; //accede al valor del par (id, jugador)

                const int centro_x_px = static_cast<int>(jugador.position.x * PIXELS_POR_CELDA); //centro en px
                const int centro_y_px = static_cast<int>(jugador.position.y * PIXELS_POR_CELDA);
                const int x = centro_x_px - ANCHO_SPRITE_PX / 2; // esquina izq
                const int y = centro_y_px - ALTO_SPRITE_PX / 2;

                if (jugador.id == estado_cliente.obtener_id_propio()) {
                    renderer.dibujar(textura, x, y, ANCHO_SPRITE_PX, ALTO_SPRITE_PX, 255, 255, 255); //blanco
                } else {
                    renderer.dibujar(textura, x, y, ANCHO_SPRITE_PX, ALTO_SPRITE_PX, 67, 38, 255); //azul
                }
            }
            renderer.presentar();
        }
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