#include "sdl_client.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <algorithm>

#include <SDL.h>
#include <SDL2pp/Exception.hh>
#include <SDL2pp/Font.hh>
#include <SDL2pp/SDL.hh>
#include <SDL2pp/SDLImage.hh>
#include <SDL2pp/SDLTTF.hh>
#include <SDL_image.h>

#include "core/input_handler.h"
#include "core/inventory_bar.h"
#include "core/stat_bars.h"
#include "game_command.h"
#include "render/camera.h"
#include "render/renderer.h"
#include "render/sdl_window.h"
#include "render/texture_manager.h"
#include "server_message.h"
#include "state/client_state.h"

namespace {
    constexpr double PIXELS_POR_CELDA = 32.0;
    constexpr int ANCHO_VENTANA = 800;
    constexpr int ALTO_VENTANA = 600;
    constexpr int TAMANIO_FUENTE_PX = 12;
    const std::string RUTA_FUENTE = "assets/Press_Start/PressStart2P-Regular.ttf";   
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

void procesar_mensajes_entrantes(NetworkClient& cliente, ClientState& estado, InventoryBar& inventario) {
    std::optional<ServerMessage> mensaje = cliente.try_receive();
    while (mensaje) {
        game::GameState* nuevo_estado = std::get_if<game::GameState>(&*mensaje);
        if (nuevo_estado) {
            estado.actualizar(*nuevo_estado);
        } 
        game::InventoryState* nuevo_inventario = std::get_if<game::InventoryState>(&*mensaje);
        if (nuevo_inventario) {
            inventario.actualizar(*nuevo_inventario);
        }
        mensaje = cliente.try_receive();
    }
}

void actualizar_camara(Camera& camara, const ClientState& estado_cliente, const game::MapData& mapa) {
    const std::map<game::PlayerId, game::PlayerState>& jugadores = estado_cliente.obtener_jugadores();
    std::map<game::PlayerId, game::PlayerState>::const_iterator propio = jugadores.find(estado_cliente.obtener_id_propio());
    if (propio == jugadores.end()) {
        return;
    }
    const int obj_x_px = static_cast<int>(propio->second.position.x * PIXELS_POR_CELDA);
    const int obj_y_px = static_cast<int>(propio->second.position.y * PIXELS_POR_CELDA);
    const int mapa_ancho_px = static_cast<int>(mapa.width * PIXELS_POR_CELDA);
    const int mapa_alto_px = static_cast<int>(mapa.height * PIXELS_POR_CELDA);
    
    camara.centrar_en(obj_x_px, obj_y_px, mapa_ancho_px, mapa_alto_px);
}

void dibujar_mapa(Renderer& renderer, const game::MapData& mapa, const Camera& camara) {
    const int celda_px = static_cast<int>(PIXELS_POR_CELDA);

    for (std::uint32_t fila = 0; fila < mapa.height; ++fila) {
        for (std::uint32_t columna = 0; columna < mapa.width; ++columna) {
            const std::size_t indice = static_cast<std::size_t>(fila) * mapa.width + columna;
            const int x = camara.transformar_x(static_cast<int>(columna) * celda_px);
            const int y = camara.transformar_y(static_cast<int>(fila) * celda_px);

            if (mapa.tiles[indice] == game::TileType::Blocked) {
                renderer.dibujar_rectangulo(x, y, celda_px, celda_px, 90, 60, 40); //marron
            } else {
                renderer.dibujar_rectangulo(x, y, celda_px, celda_px, 120, 200, 120); //verde
            }
        }
    }
}

void dibujar_jugadores(Renderer& renderer, SDL2pp::Texture& textura, const ClientState& estado_cliente, const Camera& camara, int ancho_sprite, int alto_sprite) {
    const std::map<game::PlayerId, game::PlayerState>& jugadores = estado_cliente.obtener_jugadores();
    for (const std::pair<const game::PlayerId, game::PlayerState>& entrada_mapa : jugadores) {
        const game::PlayerState& jugador = entrada_mapa.second;

        const int centro_x_px = static_cast<int>(jugador.position.x * PIXELS_POR_CELDA);
        const int centro_y_px = static_cast<int>(jugador.position.y * PIXELS_POR_CELDA);
        const int x = camara.transformar_x(centro_x_px) - ancho_sprite / 2;
        const int y = camara.transformar_y(centro_y_px) - alto_sprite / 2;

        if (jugador.id == estado_cliente.obtener_id_propio()) {
            renderer.dibujar(textura, x, y, ancho_sprite, alto_sprite, 255, 255, 255); //blanco
        } else {
            renderer.dibujar(textura, x, y, ancho_sprite, alto_sprite, 67, 38, 255); //azul
        }
    }
}


bool ejecutar_bucle_juego(NetworkClient& cliente, ClientState& estado, InputHandler& input_handler,
                          Renderer& renderer, TextureManager& texturas, SDL2pp::Font& fuente, SDL2pp::Texture& textura, const game::MapData& mapa,
                          InventoryBar& inventario, const StatBars& estadisticas, Camera& camara,
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

        //TO-DO: manejar seleccion de slot de inventario

        actualizar_direccion_pendiente(entrada.direccion, ultima_direccion_enviada, pendiente);

        if (pendiente) {
            if(cliente.try_send(game::MoveRequest{*pendiente})) {
                ultima_direccion_enviada = *pendiente;
                pendiente.reset();
            }
        }

        procesar_mensajes_entrantes(cliente, estado, inventario);
        actualizar_camara(camara, estado, mapa);

        renderer.limpiar();
        dibujar_mapa(renderer, mapa, camara);
        dibujar_jugadores(renderer, textura, estado, camara, ancho_sprite, alto_sprite);
        estadisticas.dibujar(renderer);
        inventario.dibujar(renderer, texturas, fuente, ANCHO_VENTANA, ALTO_VENTANA);
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
        SDL2pp::Font fuente(RUTA_FUENTE, TAMANIO_FUENTE_PX);

        const int ancho_sprite_px = static_cast<int>(info_partida.player_width * PIXELS_POR_CELDA);
        const int alto_sprite_px = static_cast<int>(info_partida.player_height * PIXELS_POR_CELDA);

        ClientState estado_cliente(id_propio);
        InputHandler input_handler;
        InventoryBar inventario;
        StatBars estadisticas;
        Camera camara(ANCHO_VENTANA, ALTO_VENTANA);

        const bool conexion_perdida = ejecutar_bucle_juego(cliente, estado_cliente, input_handler, renderer, texturas, fuente, textura, info_partida.map, inventario, estadisticas, camara, ancho_sprite_px, alto_sprite_px);
        
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