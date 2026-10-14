#include "client_state.h"

ClientState::ClientState(int x_inicial, int y_inicial) : x(x_inicial), y(y_inicial) {}

int ClientState::obtener_x() const {
    return x;
}

int ClientState::obtener_y() const {
    return y;
}

void ClientState::mover_a(int nuevo_x, int nuevo_y) {
    x = nuevo_x;
    y = nuevo_y;
}