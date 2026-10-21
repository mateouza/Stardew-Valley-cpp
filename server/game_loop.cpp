#include "game_loop.h"

#include <cstddef>
#include <stdexcept>
#include <thread>
#include <variant>

#include "server_message.h"
#include "tick_scheduler.h"

GameLoop::GameLoop(game::GameModel& model, Queue<MatchCommand>& commands,
                   std::chrono::milliseconds tick_interval):
        model(model), commands(commands), tick_interval(tick_interval) {
    if (tick_interval.count() <= 0) {
        throw std::invalid_argument("El intervalo de actualizacion debe ser positivo");
    }
}

void GameLoop::process_add_player(const AddPlayerCommand& command) {
    const auto& channel = command.channel;

    if (!channel) {
        throw std::invalid_argument("El pedido de ingreso debe tener un canal");
    }

    if (channel->is_closing()) {
        return;
    }

    const auto session_id = channel->get_session_id();

    if (sessions.contains(session_id)) {
        return;
    }

    const auto result = model.add_player(command.name);

    if (const auto* player_id = std::get_if<game::PlayerId>(&result)) {
        sessions.emplace(session_id, PlayerSession{*player_id, channel});
    }

    try {
        if (!channel->get_outgoing_queue().try_push(JoinResponse{result})) {
            channel->request_close();
        }
    } catch (const ClosedQueue&) {
        channel->request_close();
    }
}

void GameLoop::remove_closed_sessions() {
    auto session = sessions.begin();

    while (session != sessions.end()) {
        if (session->second.channel->is_closing()) {
            model.disconnect_player(session->second.player_id);
            session = sessions.erase(session);
        } else {
            ++session;
        }
    }
}

void GameLoop::process_movement(const PlayerCommand& command) {
    const auto session = sessions.find(command.session_id);

    if (session == sessions.end()) {
        return;
    }

    if (session->second.channel->is_closing()) {
        return;
    }

    model.set_direction(session->second.player_id, command.request.direction);
}

void GameLoop::process_commands() {
    constexpr std::size_t MAX_COMMANDS_PER_TICK = 128;

    for (std::size_t processed = 0; processed < MAX_COMMANDS_PER_TICK; ++processed) {
        if (!should_keep_running()) {
            break;
        }

        MatchCommand command;
        if (!commands.try_pop(command)) {
            break;
        }

        if (const auto* movement = std::get_if<PlayerCommand>(&command)) {
            process_movement(*movement);
        } else if (const auto* entry = std::get_if<AddPlayerCommand>(&command)) {
            process_add_player(*entry);
        }
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

                remove_closed_sessions();
                process_commands();
                remove_closed_sessions();

                if (!should_keep_running()) {
                    break;
                }

                model.update_world(elapsed);
                publish_state();
            }

            if (!should_keep_running()) {
                break;
            }

            const auto wait_time = scheduler.get_wait_time();

            if (previous_time > std::chrono::steady_clock::time_point::max() - wait_time) {
                throw std::overflow_error("El plazo de espera supera el rango representable");
            }

            std::this_thread::sleep_until(previous_time + wait_time);
        }
    } catch (const ClosedQueue&) {
    } catch (...) {
        failure = std::current_exception();
    }

    close_commands();
    close_pending_channels();
    close_sessions();
}

void GameLoop::close_commands() {
    std::lock_guard<std::mutex> lock(close_mutex);

    if (commands_closed) {
        return;
    }

    commands.close();
    commands_closed = true;
}

void GameLoop::close_sessions() {
    for (const auto& session: sessions) {
        session.second.channel->request_close();

        try {
            model.disconnect_player(session.second.player_id);
        } catch (...) {
            if (!failure) {
                failure = std::current_exception();
            }
        }
    }

    sessions.clear();
}

void GameLoop::publish_state() {
    if (sessions.empty()) {
        return;
    }

    const ServerMessage message = model.get_state();

    for (const auto& session: sessions) {
        const auto& channel = session.second.channel;

        if (channel->is_closing()) {
            continue;
        }

        try {
            if (!channel->get_outgoing_queue().try_push(message)) {
                channel->request_close();
            }
        } catch (const ClosedQueue&) {
            channel->request_close();
        }
    }
}

void GameLoop::close_pending_channels() {
    try {
        MatchCommand command;

        while (commands.try_pop(command)) {
            const auto* entry = std::get_if<AddPlayerCommand>(&command);

            if (entry && entry->channel) {
                entry->channel->request_close();
            }
        }
    } catch (const ClosedQueue&) {}
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
