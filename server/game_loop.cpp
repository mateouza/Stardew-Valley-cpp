#include "game_loop.h"

#include <cstddef>
#include <stdexcept>
#include <thread>
#include <variant>

#include "server_message.h"
#include "tick_scheduler.h"

GameLoop::GameLoop(game::GameModel& model, Queue<MatchCommand>& commands,
                   std::chrono::milliseconds tick_interval, const game::GameInfo& game_info):
        model(model), commands(commands), game_info(game_info), tick_interval(tick_interval) {
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

    try {
        const auto result = model.add_player(command.name);
        const auto* player_id = std::get_if<game::PlayerId>(&result);

        if (player_id) {
            sessions.emplace(session_id, PlayerSession{*player_id, channel});
        }

        try {
            if (!channel->get_outgoing_queue().try_push(JoinResponse{result})) {
                channel->request_close();
                return;
            }

            if (player_id) {
                if (!channel->get_outgoing_queue().try_push(game_info)) {
                    channel->request_close();
                    return;
                }

                send_inventory(*player_id, *channel);

                if (!channel->is_closing()) {
                    send_farm_state(*channel);
                }
            }
        } catch (const ClosedQueue&) {
            channel->request_close();
        }
    } catch (...) {
        channel->request_close();
        throw;
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

void GameLoop::send_inventory(game::PlayerId player_id, SessionChannel& channel) {
    const auto inventory = model.get_inventory_state(player_id);
    if (!inventory) {
        throw std::logic_error("La sesion no tiene un jugador registrado");
    }

    send_to_channel(*inventory, channel);
}

void GameLoop::process_selection(const SelectSlotCommand& command) {
    const auto session = sessions.find(command.session_id);
    if (session == sessions.end() || session->second.channel->is_closing()) {
        return;
    }

    const auto player_id = session->second.player_id;
    if (model.select_slot(player_id, command.request.slot) == game::SelectSlotResult::Accepted) {
        send_inventory(player_id, *session->second.channel);
    }
}

void GameLoop::process_use_cell(const UseCellCommand& command) {
    const auto session = sessions.find(command.session_id);
    if (session == sessions.end() || session->second.channel->is_closing()) {
        return;
    }

    const auto player_id = session->second.player_id;
    auto& channel = *session->second.channel;
    const auto result = model.use_selected_item(player_id, command.request.cell);

    send_to_channel(UseItemResponse{result}, channel);

    if (const auto* prepare = std::get_if<game::PrepareSoilResult>(&result)) {
        if (*prepare == game::PrepareSoilResult::Accepted) {
            const auto soil = model.get_soil_state();
            if (!soil) {
                throw std::logic_error("Una preparacion aceptada debe tener suelo");
            }

            publish_message(*soil);
        }
        return;
    }

    if (const auto* plant = std::get_if<game::PlantCropResult>(&result)) {
        if (*plant == game::PlantCropResult::Accepted) {
            send_inventory(player_id, channel);
            publish_message(game::CropsState{model.get_crops_state()});
        }
        return;
    }

    if (const auto* water = std::get_if<game::WaterCropResult>(&result)) {
        if (*water == game::WaterCropResult::Accepted) {
            publish_message(game::CropsState{model.get_crops_state()});
        }
    }
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
        } else if (const auto* selection = std::get_if<SelectSlotCommand>(&command)) {
            process_selection(*selection);
        }  else if (const auto* use_cell = std::get_if<UseCellCommand>(&command)) {
            process_use_cell(*use_cell);
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
    if (!sessions.empty()) {
        publish_message(model.get_state());
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

void GameLoop::send_to_channel(const ServerMessage& message, SessionChannel& channel) {
    if (channel.is_closing()) {
        return;
    }

    try {
        if (!channel.get_outgoing_queue().try_push(message)) {
            channel.request_close();
        }
    } catch (const ClosedQueue&) {
        channel.request_close();
    }
}

void GameLoop::publish_message(const ServerMessage& message) {
    for (const auto& entry: sessions) {
        send_to_channel(message, *entry.second.channel);
    }
}

void GameLoop::send_farm_state(SessionChannel& channel) {
    const auto soil = model.get_soil_state();
    if (!soil) {
        return;
    }

    send_to_channel(*soil, channel);

    if (!channel.is_closing()) {
        send_to_channel(game::CropsState{model.get_crops_state()}, channel);
    }
}
