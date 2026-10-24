#include "match_manager.h"

#include <limits>
#include <stdexcept>
#include <utility>

MatchManager::MatchManager(game::Map initial_map, game::Position spawn_position,
                           game::WorldConfig config, unsigned int command_capacity,
                           std::chrono::milliseconds tick_interval):
        initial_map(std::move(initial_map)),
        spawn_position(spawn_position),
        config(config),
        command_capacity(command_capacity),
        tick_interval(tick_interval) {
    if (command_capacity == 0) {
        throw std::invalid_argument("La capacidad de comandos debe ser mayor que cero");
    }

    if (tick_interval.count() <= 0) {
        throw std::invalid_argument("El intervalo de actualizacion debe ser positivo");
    }
}

MatchManager::~MatchManager() { stop(); }

ManagedMatch MatchManager::create_match(const std::string& name) {
    if (name.empty()) {
        throw std::invalid_argument("La partida debe tener un nombre");
    }

    std::lock_guard<std::mutex> lock(mutex);

    if (stopped) {
        throw std::logic_error("El administrador de partidas esta cerrado");
    }

    if (next_code == std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error("Se agotaron los codigos de partida");
    }

    const auto code = std::to_string(next_code);
    auto match = std::make_shared<Match>(initial_map, spawn_position, config, command_capacity,
                                         tick_interval);

    ManagedMatch entry{name, code, std::move(match)};
    const auto inserted = matches.emplace(code, entry);

    try {
        entry.match->start();
    } catch (...) {
        matches.erase(inserted.first);
        throw;
    }

    ++next_code;

    return entry;
}

std::optional<ManagedMatch> MatchManager::find_match(const std::string& invitation_code) {
    std::lock_guard<std::mutex> lock(mutex);

    if (stopped) {
        return std::nullopt;
    }

    const auto entry = matches.find(invitation_code);

    if (entry == matches.end()) {
        return std::nullopt;
    }

    return entry->second;
}

void MatchManager::stop() {
    std::map<std::string, ManagedMatch> matches_to_close;

    {
        std::lock_guard<std::mutex> lock(mutex);

        if (stopped) {
            return;
        }

        stopped = true;
        matches_to_close.swap(matches);
    }

    for (const auto& entry: matches_to_close) {
        entry.second.match->stop();
    }

    for (const auto& entry: matches_to_close) {
        entry.second.match->join();

        try {
            entry.second.match->rethrow_if_failed();
        } catch (...) {
            if (!failure) {
                failure = std::current_exception();
            }
        }
    }
}

void MatchManager::rethrow_if_failed() const {
    if (!stopped) {
        throw std::logic_error("Se debe detener el administrador antes de consultar errores");
    }

    if (failure) {
        std::rethrow_exception(failure);
    }
}