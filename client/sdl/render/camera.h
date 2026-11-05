#ifndef CAMERA_H
#define CAMERA_H

class Camera {
    private:
        int ancho_ventana;
        int alto_ventana;
        int desplazamiento_x;
        int desplazamiento_y;
    public:
        Camera(int ancho_ventana, int alto_ventana);

        // Si el mapa es más chico que la ventana en algún eje, queda anclada en 0 en ese eje.
        void centrar_en(int obj_x_px, int obj_y_px, int mapa_ancho_px, int mapa_alto_px);


        int transformar_x(int x_mundo_px) const;
        int transformar_y(int y_mundo_px) const;
};

#endif
