#ifndef MATCH_MANAGER_H
#define MATCH_MANAGER_H

#include <chrono>
#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include "match.h"

struct ManagedMatch {
    std::string name;
    std::string invitation_code;
    std::shared_ptr<Match> match;
};

class MatchManager {
private:
    game::Map initial_map;
    game::Position spawn_position;
    game::WorldConfig config;
    unsigned int command_capacity;
    std::chrono::milliseconds tick_interval;

    std::map<std::string, ManagedMatch> matches;
    std::uint64_t next_code = 1;
    mutable std::mutex mutex;
    bool stopped = false;
    std::exception_ptr failure;

public:
    MatchManager(game::Map initial_map, game::Position spawn_position, game::WorldConfig config,
                 unsigned int command_capacity, std::chrono::milliseconds tick_interval);

    ~MatchManager();

    ManagedMatch create_match(const std::string& name);
    std::optional<ManagedMatch> find_match(const std::string& invitation_code);

    void stop();
    void rethrow_if_failed() const;
};

#endif