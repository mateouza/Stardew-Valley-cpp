#ifndef CLIENT_STATE_H
#define CLIENT_STATE_H

#include <map>

#include "game_state.h"
#include "game_types.h"

class ClientState {
private:
    game::PlayerId id_propio;
    std::map<game::PlayerId, game::PlayerState> jugadores;

public:
    explicit ClientState(game::PlayerId id_propio);

    game::PlayerId obtener_id_propio() const;

    void actualizar(const game::GameState& estado);

    const std::map<game::PlayerId, game::PlayerState>& obtener_jugadores() const;
};

#endif
