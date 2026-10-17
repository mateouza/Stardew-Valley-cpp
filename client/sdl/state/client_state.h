#ifndef CLIENT_STATE_H
#define CLIENT_STATE_H

class ClientState {
    private:
        double x;
        double y;
    public:
        ClientState(double x_inicial, double y_inicial);

        double obtener_x() const;
        double obtener_y() const;

        void mover_a(double nuevo_x, double nuevo_y);
};

#endif
