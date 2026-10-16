#include "game_loop.h"

#include <cstddef>
#include <stdexcept>
#include <thread>

GameLoop::GameLoop(game::GameModel& model, Queue<PlayerCommand>& commands,
                   std::chrono::milliseconds tick_interval):
        model(model), commands(commands), tick_interval(tick_interval) {
    if (tick_interval.count() <= 0) {
        throw std::invalid_argument("El intervalo de actualizacion debe ser positivo");
    }
}

void GameLoop::process_commands() {
    constexpr std::size_t MAX_COMMANDS_PER_TICK = 128;

    for (std::size_t processed = 0; processed < MAX_COMMANDS_PER_TICK; ++processed) {
        if (!should_keep_running()) {
            break;
        }

        PlayerCommand command;
        if (!commands.try_pop(command)) {
            break;
        }

        model.set_direction(command.player_id, command.request.direction);
    }
}

void GameLoop::run() {
    try {
        const std::chrono::duration<double> elapsed = tick_interval;
        auto next_tick = std::chrono::steady_clock::now() + tick_interval;

        while (should_keep_running()) {
            std::this_thread::sleep_until(next_tick);

            if (!should_keep_running()) {
                break;
            }

            process_commands();

            if (!should_keep_running()) {
                break;
            }

            model.update_world(elapsed);

            next_tick += tick_interval;

            const auto now = std::chrono::steady_clock::now();
            if (next_tick < now) {
                next_tick = now + tick_interval;
            }
        }
    } catch (const ClosedQueue&) {
    } catch (...) {
        failure = std::current_exception();
    }
}

void GameLoop::rethrow_if_failed() const {
    if (failure) {
        std::rethrow_exception(failure);
    }
}
