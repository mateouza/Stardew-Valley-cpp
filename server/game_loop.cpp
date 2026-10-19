#include "game_loop.h"

#include <cstddef>
#include <stdexcept>
#include <thread>

#include "tick_scheduler.h"

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
        TickScheduler scheduler(tick_interval);
        const std::chrono::duration<double> elapsed = tick_interval;
        auto previous_time = std::chrono::steady_clock::now();

        while (should_keep_running()) {
            const auto now = std::chrono::steady_clock::now();
            const auto steps = scheduler.advance(now - previous_time);
            previous_time = now;

            for (std::size_t step = 0; step < steps; ++step) {
                if (!should_keep_running()) {
                    break;
                }

                process_commands();

                if (!should_keep_running()) {
                    break;
                }

                model.update_world(elapsed);
            }

            if (!should_keep_running()) {
                break;
            }

            std::this_thread::sleep_until(previous_time + scheduler.get_wait_time());
        }
    } catch (const ClosedQueue&) {
    } catch (...) {
        failure = std::current_exception();
    }

    close_commands();
}

void GameLoop::close_commands() {
    std::lock_guard<std::mutex> lock(close_mutex);

    if (commands_closed) {
        return;
    }

    commands.close();
    commands_closed = true;
}

void GameLoop::stop() {
    Thread::stop();
    close_commands();
}

void GameLoop::rethrow_if_failed() const {
    if (failure) {
        std::rethrow_exception(failure);
    }
}
