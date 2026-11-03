#include "client_state.h"

ClientState::ClientState(game::PlayerId id_propio): id_propio(id_propio) {}

game::PlayerId ClientState::obtener_id_propio() const {
    return id_propio;
}

void ClientState::actualizar(const game::GameState& estado) {
    jugadores.clear();
    for (const auto& jugador: estado.players) {
        jugadores[jugador.id] = jugador;
    }
}

const std::map<game::PlayerId, game::PlayerState>& ClientState::obtener_jugadores() const {
    return jugadores;
}