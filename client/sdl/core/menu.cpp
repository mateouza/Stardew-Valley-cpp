#include "menu.h"

#include <algorithm>

namespace{
    constexpr int BOTON_MENU_ANCHO_PX = 70;
    constexpr int BOTON_MENU_ALTO_PX = 28;
    constexpr int MARGEN_PX = 12;

    constexpr int PANEL_ANCHO_PX = 280;
    constexpr int PANEL_ALTO_PX = 220;

    constexpr int BOTON_SALIR_ANCHO_PX = 200;
    constexpr int BOTON_SALIR_ALTO_PX = 32;

    constexpr int BOTON_CERRAR_LADO_PX = 20;
    constexpr int MARGEN_CERRAR_PX = 8;

    constexpr int ALTO_BARRA_PX = 10;
    constexpr double VOLUMEN_MUSICA_INICIAL = 0.7;
    constexpr int ANCHO_PERILLA_PX = 8;
    constexpr int ALTO_PERILLA_PX = 18;

    bool punto_en_rectangulo(int x, int y, int rect_x, int rect_y, int rect_ancho, int rect_alto){
        return x >= rect_x && x < rect_x + rect_ancho && y >= rect_y && y < rect_y + rect_alto;
    };
}

Menu::Menu() : abierto(false), volumen_musica(VOLUMEN_MUSICA_INICIAL) {}

bool Menu::esta_abierto() const {
    return abierto;
}

double Menu::obtener_volumen_musica() const {
    return volumen_musica;
}

SDL2pp::Rect Menu::calcular_barra_musica(SDL2pp::Font& fuente, int ancho_ventana, int alto_ventana) const {
    const int panel_x = (ancho_ventana - PANEL_ANCHO_PX) / 2;
    const int panel_y = (alto_ventana - PANEL_ALTO_PX) / 2;
    const int x_contenido = panel_x + 20;
    const int ancho_barra = PANEL_ANCHO_PX - 40;

    int y_contenido = panel_y + 20;
    y_contenido += fuente.GetHeight() + 16; // Titulo
    y_contenido += fuente.GetHeight() + 4; // Texto "SONIDO"
    y_contenido += ALTO_BARRA_PX + 14; // Barra de sonido
    y_contenido += fuente.GetHeight() + 4; // Texto "MUSICA"

    return SDL2pp::Rect(x_contenido, y_contenido, ancho_barra, ALTO_BARRA_PX);
}

bool Menu::procesar_click(int x, int y, int ancho_ventana, int alto_ventana, SDL2pp::Font& fuente) {
    const int boton_menu_x = ancho_ventana - BOTON_MENU_ANCHO_PX - MARGEN_PX;
    const int boton_menu_y = MARGEN_PX;

    if (punto_en_rectangulo(x, y, boton_menu_x, boton_menu_y, BOTON_MENU_ANCHO_PX, BOTON_MENU_ALTO_PX)) {
        abierto = !abierto;
        return false;
    }

    if (!abierto) {
        return false;
    }

    const int panel_x = (ancho_ventana - PANEL_ANCHO_PX) / 2;
    const int panel_y = (alto_ventana - PANEL_ALTO_PX) / 2;
    const int boton_cerrar_x = panel_x + PANEL_ANCHO_PX - BOTON_CERRAR_LADO_PX - MARGEN_CERRAR_PX;
    const int boton_cerrar_y = panel_y + MARGEN_CERRAR_PX;

    if (punto_en_rectangulo(x, y, boton_cerrar_x, boton_cerrar_y, BOTON_CERRAR_LADO_PX, BOTON_CERRAR_LADO_PX)) {
        abierto = false;
        return false;
    }

    const SDL2pp::Rect barra_musica = calcular_barra_musica(fuente, ancho_ventana, alto_ventana);
    if (punto_en_rectangulo(x, y, barra_musica.x, barra_musica.y, barra_musica.w, barra_musica.h)) {
        const double proporcion = static_cast<double>(x - barra_musica.x) / barra_musica.w;
        volumen_musica = std::clamp(proporcion, 0.0, 1.0);
        return false;
    }   
    
    const int boton_salir_x = panel_x + (PANEL_ANCHO_PX - BOTON_SALIR_ANCHO_PX) / 2;
    const int boton_salir_y = panel_y + PANEL_ALTO_PX - BOTON_SALIR_ALTO_PX - MARGEN_PX;

    if (punto_en_rectangulo(x, y, boton_salir_x, boton_salir_y, BOTON_SALIR_ANCHO_PX, BOTON_SALIR_ALTO_PX)) {
        return true; // Indica que se hizo click en el botón de salir
    }

    return false;
}

void Menu::dibujar(Renderer& renderer, SDL2pp::Font& fuente, const std::string& codigo_invitacion,
                      int ancho_ventana, int alto_ventana) const {
    const int boton_menu_x = ancho_ventana - BOTON_MENU_ANCHO_PX - MARGEN_PX;
    const int boton_menu_y = MARGEN_PX;
    renderer.dibujar_rectangulo(boton_menu_x, boton_menu_y, BOTON_MENU_ANCHO_PX, BOTON_MENU_ALTO_PX, 50, 50, 60);
    renderer.dibujar_rectangulo(boton_menu_x + 4, boton_menu_y + 4, BOTON_MENU_ANCHO_PX - 8, BOTON_MENU_ALTO_PX - 8, 70, 70, 80);
    SDL2pp::Texture texto_boton = renderer.cargar_textura_fuente(fuente, "MENU", 255, 255, 255);
    renderer.dibujar(texto_boton, boton_menu_x + (BOTON_MENU_ANCHO_PX - texto_boton.GetWidth()) / 2,
                     boton_menu_y + (BOTON_MENU_ALTO_PX - texto_boton.GetHeight()) / 2, texto_boton.GetWidth(),
                     texto_boton.GetHeight(), 255, 255, 255);

    if (!abierto) {
        return;
    }

    renderer.dibujar_rectangulo_blur(0, 0, ancho_ventana, alto_ventana, 0, 0, 0, 150);

    const int panel_x = (ancho_ventana - PANEL_ANCHO_PX) / 2;
    const int panel_y = (alto_ventana - PANEL_ALTO_PX) / 2;

    renderer.dibujar_rectangulo(panel_x, panel_y, PANEL_ANCHO_PX, PANEL_ALTO_PX, 30, 30, 40);
    renderer.dibujar_rectangulo(panel_x + 4, panel_y + 4, PANEL_ANCHO_PX - 8, PANEL_ALTO_PX - 8, 50, 50, 65);

    const int boton_cerrar_x = panel_x + PANEL_ANCHO_PX - BOTON_CERRAR_LADO_PX - MARGEN_CERRAR_PX;
    const int boton_cerrar_y = panel_y + MARGEN_CERRAR_PX;
    renderer.dibujar_rectangulo(boton_cerrar_x, boton_cerrar_y, BOTON_CERRAR_LADO_PX, BOTON_CERRAR_LADO_PX, 150, 40, 40);
    SDL2pp::Texture texto_cerrar = renderer.cargar_textura_fuente(fuente, "X", 255, 255, 255);
    renderer.dibujar(texto_cerrar, boton_cerrar_x + (BOTON_CERRAR_LADO_PX - texto_cerrar.GetWidth()) / 2,
                     boton_cerrar_y + (BOTON_CERRAR_LADO_PX - texto_cerrar.GetHeight()) / 2, texto_cerrar.GetWidth(),
                     texto_cerrar.GetHeight(), 255, 255, 255);

    const int x_contenido = panel_x + 20;
    int y_contenido = panel_y + 20;
    const int ancho_barra = PANEL_ANCHO_PX - 40;

    SDL2pp::Texture titulo = renderer.cargar_textura_fuente(fuente, "MENU", 255, 255, 255);
    renderer.dibujar(titulo, x_contenido, y_contenido, titulo.GetWidth(), titulo.GetHeight(), 255, 255, 255);
    y_contenido += titulo.GetHeight() + 16;

    // Volumen de sonido/musica: placeholder visual, sin conectar a audio real todavia
    // (se conecta cuando sumemos SDL2pp::Mixer).
    SDL2pp::Texture texto_sonido = renderer.cargar_textura_fuente(fuente, "SONIDO", 220, 220, 220);
    renderer.dibujar(texto_sonido, x_contenido, y_contenido, texto_sonido.GetWidth(), texto_sonido.GetHeight(),
                     220, 220, 220);
    y_contenido += texto_sonido.GetHeight() + 4;
    renderer.dibujar_rectangulo(x_contenido, y_contenido, ancho_barra, ALTO_BARRA_PX, 70, 70, 80);
    renderer.dibujar_rectangulo(x_contenido, y_contenido, ancho_barra * 7 / 10, ALTO_BARRA_PX, 120, 170, 220);
    y_contenido += 10 + 14;

    SDL2pp::Texture texto_musica = renderer.cargar_textura_fuente(fuente, "MUSICA", 220, 220, 220);
    renderer.dibujar(texto_musica, x_contenido, y_contenido, texto_musica.GetWidth(), texto_musica.GetHeight(),
                     220, 220, 220);
    y_contenido += texto_musica.GetHeight() + 4;
    renderer.dibujar_rectangulo(x_contenido, y_contenido, ancho_barra, ALTO_BARRA_PX, 70, 70, 80);
    renderer.dibujar_rectangulo(x_contenido, y_contenido, static_cast<int>(ancho_barra * volumen_musica), ALTO_BARRA_PX, 120, 170, 220);
    const int centro_perilla_x = x_contenido + static_cast<int>(ancho_barra * volumen_musica);
    const int perilla_x = std::clamp(centro_perilla_x - ANCHO_PERILLA_PX / 2, x_contenido,
                                      x_contenido + ancho_barra - ANCHO_PERILLA_PX);
    const int perilla_y = y_contenido - (ALTO_PERILLA_PX - ALTO_BARRA_PX) / 2;
    renderer.dibujar_rectangulo(perilla_x, perilla_y, ANCHO_PERILLA_PX, ALTO_PERILLA_PX, 255, 255, 255);
    y_contenido += ALTO_BARRA_PX + 16;

    const std::string texto_codigo = codigo_invitacion.empty() ? "CODIGO: ---" : "CODIGO: " + codigo_invitacion;
    SDL2pp::Texture textura_codigo = renderer.cargar_textura_fuente(fuente, texto_codigo, 255, 255, 255);
    renderer.dibujar(textura_codigo, x_contenido, y_contenido, textura_codigo.GetWidth(), textura_codigo.GetHeight(),
                     255, 255, 255);

    const int boton_salir_x = panel_x + (PANEL_ANCHO_PX - BOTON_SALIR_ANCHO_PX) / 2;
    const int boton_salir_y = panel_y + PANEL_ALTO_PX - BOTON_SALIR_ALTO_PX - MARGEN_PX;
    renderer.dibujar_rectangulo(boton_salir_x, boton_salir_y, BOTON_SALIR_ANCHO_PX, BOTON_SALIR_ALTO_PX, 150, 40, 40);
    SDL2pp::Texture texto_salir = renderer.cargar_textura_fuente(fuente, "SALIR DEL JUEGO", 255, 255, 255);
    renderer.dibujar(texto_salir, boton_salir_x + (BOTON_SALIR_ANCHO_PX - texto_salir.GetWidth()) / 2,
                     boton_salir_y + (BOTON_SALIR_ALTO_PX - texto_salir.GetHeight()) / 2, texto_salir.GetWidth(),
                     texto_salir.GetHeight(), 255, 255, 255);
}