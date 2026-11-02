#include "sdl_client.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>

#include <SDL.h>
#include <SDL2pp/Exception.hh>
#include <SDL2pp/SDL.hh>
#include <SDL2pp/SDLImage.hh>
#include <SDL_image.h>

#include "core/input_handler.h"
#include "game_command.h"
#include "render/renderer.h"
#include "render/sdl_window.h"
#include "render/texture_manager.h"
#include "server_message.h"
#include "state/client_state.h"

namespace {
    constexpr double PIXELS_POR_CELDA = 32.0;
    constexpr int ANCHO_VENTANA = 800;
    constexpr int ALTO_VENTANA = 600;   
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


bool ejecutar_bucle_juego(NetworkClient& cliente, ClientState& estado, InputHandler& input_handler,
                          Renderer& renderer, SDL2pp::Texture& textura, const game::MapData& mapa,
                          int ancho_sprite, int alto_sprite) {
    game::MovementDirection ultima_direccion_enviada{0, 0};
    std::optional<game::MovementDirection> pendiente;

    bool corriendo = true;
    bool conexion_perdida = false;

    while(corriendo){
        if (cliente.has_finished()){
            corriendo = false;  
            conexion_perdida = true;
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

    return conexion_perdida;
}

int ejecutar_cliente_sdl(NetworkClient& cliente, game::PlayerId id_propio,
                         const game::GameInfo& info_partida, const std::string& codigo_invitacion){
    try {
        SDL2pp::SDL sdl(SDL_INIT_VIDEO);
        SDL2pp::SDLImage sdl_image(IMG_INIT_PNG);

        const std::string titulo = codigo_invitacion.empty() ? "Stardew Valley" : "Stardew Valley - Código: " + codigo_invitacion;
        SDLWindow ventana(titulo.c_str(), ANCHO_VENTANA, ALTO_VENTANA, false);
        Renderer renderer(ventana.obtener_handle());
        TextureManager texturas(renderer);
        SDL2pp::Texture& textura = texturas.obtener_textura("assets/rock.png");

        const int ancho_sprite_px = static_cast<int>(info_partida.player_width * PIXELS_POR_CELDA);
        const int alto_sprite_px = static_cast<int>(info_partida.player_height * PIXELS_POR_CELDA);

        ClientState estado_cliente(id_propio);
        InputHandler input_handler;

        const bool conexion_perdida = ejecutar_bucle_juego(cliente, estado_cliente, input_handler, renderer, textura, info_partida.map, ancho_sprite_px, alto_sprite_px);
        
        if (conexion_perdida) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Conexion perdida",
                                     "Se perdio la conexion con el servidor", nullptr);
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    } catch (const SDL2pp::Exception& e) {
        std::cerr << "Error de SDL (" << e.GetSDLFunction() << "): " << e.GetSDLError() << '\n';
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
                         }