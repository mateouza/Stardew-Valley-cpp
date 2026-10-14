#ifndef CLIENT_STATE_H
#define CLIENT_STATE_H

class ClientState {
    private:
        int x;
        int y;
    public:
        ClientState(int x_inicial, int y_inicial);

        int obtener_x() const;
        int obtener_y() const;

        void mover_a(int nuevo_x, int nuevo_y);
};

#endif
