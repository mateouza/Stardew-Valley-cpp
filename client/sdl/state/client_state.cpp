#include "client_state.h"

ClientState::ClientState(double x_inicial, double y_inicial) : x(x_inicial), y(y_inicial) {}

double ClientState::obtener_x() const {
    return x;
}

double ClientState::obtener_y() const {
    return y;
}

void ClientState::mover_a(double nuevo_x, double nuevo_y) {
    x = nuevo_x;
    y = nuevo_y;
}