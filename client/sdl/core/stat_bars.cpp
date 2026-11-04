#include "stat_bars.h"

namespace{
    constexpr int MARGEN_PX = 16;
    constexpr int ANCHO_BARRA_PX = 150;
    constexpr int ALTO_BARRA_PX = 16;
    constexpr int SEPARACION_PX = 4;
    constexpr double RATIO_PLACEHOLDER = 1.0; //TO-DO: sin datos reales, por ahora siempre lleno
}

void StatBars::dibujar(Renderer& renderer) const {
    const int x = MARGEN_PX;
    const int y_vida = MARGEN_PX;
    const int y_energia = y_vida + ALTO_BARRA_PX + SEPARACION_PX;

    renderer.dibujar_rectangulo(x, y_vida, ANCHO_BARRA_PX, ALTO_BARRA_PX, 40, 40, 40);
    renderer.dibujar_rectangulo(x, y_vida, static_cast<int>(ANCHO_BARRA_PX * RATIO_PLACEHOLDER), ALTO_BARRA_PX, 20, 40, 40);
    renderer.dibujar_rectangulo(x, y_energia, ANCHO_BARRA_PX, ALTO_BARRA_PX, 40, 40, 40);
    renderer.dibujar_rectangulo(x, y_energia, static_cast<int>(ANCHO_BARRA_PX * RATIO_PLACEHOLDER), ALTO_BARRA_PX, 60, 180, 70);
}