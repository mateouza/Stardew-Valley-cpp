#ifndef CLIENT_STATE_H
#define CLIENT_STATE_H

class ClientState {
    private:
        int x;
        int y;
    public:
        ClientState(int x_inicial, int y_inicial) : x(x_inicial), y(y_inicial) {}

        int obtener_x() const { return x; }
        int obtener_y() const { return y; }

        void mover_a(int nuevo_x, int nuevo_y) {
            x = nuevo_x;
            y = nuevo_y;
        }
};

#endif
