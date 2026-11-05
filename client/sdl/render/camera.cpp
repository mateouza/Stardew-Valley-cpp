#include "camera.h"

#include <algorithm>

Camera::Camera(int ancho_ventana, int alto_ventana) : ancho_ventana(ancho_ventana), alto_ventana(alto_ventana), desplazamiento_x(0), desplazamiento_y(0) {}

void Camera::centrar_en(int obj_x_px, int obj_y_px, int mapa_ancho_px, int mapa_alto_px) {
    const int max_desplazamiento_x = std::max(0, mapa_ancho_px - ancho_ventana);
    const int max_desplazamiento_y = std::max(0, mapa_alto_px - alto_ventana);

    desplazamiento_x = std::clamp(obj_x_px - ancho_ventana / 2, 0, max_desplazamiento_x);
    desplazamiento_y = std::clamp(obj_y_px - alto_ventana / 2, 0, max_desplazamiento_y);
}

int Camera::transformar_x(int x_mundo_px) const {
    return x_mundo_px - desplazamiento_x;
}

int Camera::transformar_y(int y_mundo_px) const {
    return y_mundo_px - desplazamiento_y;
}