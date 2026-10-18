#include "match.h"

#include <stdexcept>
#include <utility>

Match::Match(game::Map map, game::Position spawn_position, game::WorldConfig config,
             unsigned int command_capacity, std::chrono::milliseconds tick_interval):
        model(std::move(map), spawn_position, config),
        commands(command_capacity),
        game_loop(model, commands, tick_interval) {
    if (command_capacity == 0) {
        throw std::invalid_argument("La capacidad de la cola debe ser mayor que cero");
    }
}

Match::~Match() {
    stop();
    join();
}

void Match::start() {
    if (started) {
        throw std::logic_error("La partida ya fue iniciada");
    }

    game_loop.start();
    started = true;
}

void Match::stop() {
    if (started && !joined) {
        game_loop.stop();
    }
}

void Match::join() {
    if (started && !joined) {
        game_loop.join();
        joined = true;
    }
}

void Match::rethrow_if_failed() const {
    if (started && !joined) {
        throw std::logic_error(
                "Se debe esperar el cierre de la partida antes de consultar errores");
    }

    game_loop.rethrow_if_failed();
}

Queue<PlayerCommand>& Match::get_commands() { return commands; }
