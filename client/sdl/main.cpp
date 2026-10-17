#include <cstdlib>
#include <iostream>

#include <SDL2pp/Exception.hh>
#include <SDL2pp/SDL.hh>
#include <SDL2pp/SDLImage.hh>
#include <SDL_image.h>

#include "core/input_handler.h"
#include "render/renderer.h"
#include "render/sdl_window.h"
#include "state/client_state.h"

namespace {

constexpr double PIXELS_POR_CELDA = 32.0;
constexpr double CELDAS_POR_SEGUNDO = 4.0;

}

int main() {
    try {
        SDL2pp::SDL sdl(SDL_INIT_VIDEO);
        SDL2pp::SDLImage sdl_image(IMG_INIT_PNG);

        SDLWindow ventana("Stardew Valley (demo cliente)", 800, 600, false);
        Renderer renderer(ventana.obtener_handle());
        SDL2pp::Texture textura = renderer.cargar_textura("assets/rock.png");

        ClientState estado_demo(10.0, 9.0);

        InputHandler input_handler;
        bool corriendo = true;
        Uint32 tiempo_anterior = SDL_GetTicks();

        while (corriendo) {
            InputResultado entrada = input_handler.procesar_eventos();
            if (entrada.salir) {
                corriendo = false;
                continue;
            }

            Uint32 tiempo_actual = SDL_GetTicks();
            double delta_segundos = (tiempo_actual - tiempo_anterior) / 1000.0;
            tiempo_anterior = tiempo_actual;

            double nueva_x = estado_demo.obtener_x() +
                             entrada.direccion.horizontal * CELDAS_POR_SEGUNDO * delta_segundos;
            double nueva_y = estado_demo.obtener_y() +
                             entrada.direccion.vertical * CELDAS_POR_SEGUNDO * delta_segundos;
            estado_demo.mover_a(nueva_x, nueva_y);

            renderer.limpiar();
            renderer.dibujar(textura, static_cast<int>(estado_demo.obtener_x() * PIXELS_POR_CELDA),
                             static_cast<int>(estado_demo.obtener_y() * PIXELS_POR_CELDA), 48, 48);
            renderer.presentar();
        }
    } catch (const SDL2pp::Exception& e) {
        std::cerr << "Error de SDL (" << e.GetSDLFunction() << "): " << e.GetSDLError() << '\n';
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}