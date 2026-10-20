#include "world_config.h"

#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <toml++/toml.hpp>

namespace game {
namespace {

std::size_t read_max_players(const toml::table& world, const std::string& path) {
    const auto value = world["max_players"].value_exact<std::int64_t>();
    if (!value || !std::in_range<std::size_t>(*value)) {
        throw std::runtime_error(path + ": world.max_players debe ser un entero no negativo "
                                        "representable");
    }
    return static_cast<std::size_t>(*value);
}

double read_number(const toml::table& world, const char* key, const std::string& path) {
    const auto node = world[key];
    const auto value = node.value<double>();
    if (!node.is_number() || !value) {
        throw std::runtime_error(path + ": world." + key + " debe ser un numero representable");
    }
    return *value;
}

}

WorldConfig load_world_config(const std::string& path) {
    try {
        const auto document = toml::parse_file(path);
        const auto* world = document["world"].as_table();
        if (world == nullptr) {
            throw std::runtime_error(path + ": se requiere la tabla [world]");
        }
        return {read_max_players(*world, path), read_number(*world, "player_speed", path),
                read_number(*world, "player_width", path),
                read_number(*world, "player_height", path)};
    } catch (const toml::parse_error& error) {
        std::ostringstream message;
        message << "No se pudo leer '" << path << "': " << error;
        throw std::runtime_error(message.str());
    }
}

}