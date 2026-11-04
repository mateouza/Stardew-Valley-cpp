#include "menu.h"

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

    bool punto_en_rectangulo(int x, int y, int rect_x, int rect_y, int rect_ancho, int rect_alto){
        return x >= rect_x && x < rect_x + rect_ancho && y >= rect_y && y < rect_y + rect_alto;
    };
}

Menu::Menu() : abierto(false) {}

bool Menu::esta_abierto() const {
    return abierto;
}

bool Menu::procesar_click(int x, int y, int ancho_ventana, int alto_ventana) {
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
    renderer.dibujar_rectangulo(x_contenido, y_contenido, ancho_barra, 10, 70, 70, 80);
    renderer.dibujar_rectangulo(x_contenido, y_contenido, ancho_barra * 7 / 10, 10, 120, 170, 220);
    y_contenido += 10 + 14;

    SDL2pp::Texture texto_musica = renderer.cargar_textura_fuente(fuente, "MUSICA", 220, 220, 220);
    renderer.dibujar(texto_musica, x_contenido, y_contenido, texto_musica.GetWidth(), texto_musica.GetHeight(),
                     220, 220, 220);
    y_contenido += texto_musica.GetHeight() + 4;
    renderer.dibujar_rectangulo(x_contenido, y_contenido, ancho_barra, 10, 70, 70, 80);
    renderer.dibujar_rectangulo(x_contenido, y_contenido, ancho_barra * 7 / 10, 10, 120, 170, 220);
    y_contenido += 10 + 16;

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