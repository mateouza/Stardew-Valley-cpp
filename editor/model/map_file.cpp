#include "map_file.h"

#include <filesystem>
#include <fstream>
#include <system_error>
#include <utility>

#include <toml++/toml.hpp>

namespace editor {

namespace {

toml::table zone_to_toml(const Zone& zone) {
    toml::array rows;
    for (int row = 0; row < zone.height(); ++row) {
        std::string line;
        for (int column = 0; column < zone.width(); ++column) {
            line += tile_code(zone.at(column, row));
        }
        rows.push_back(std::move(line));
    }
    return toml::table{
            {"width", zone.width()}, {"height", zone.height()}, {"rows", std::move(rows)}};
}

}

void save_map(const WorldMap& map, const std::string& path) {
    toml::array dungeon;
    for (const Zone& level: map.dungeon) {
        dungeon.push_back(zone_to_toml(level));
    }
    const toml::table document{{"version", MAP_FILE_VERSION},
                               {"name", map.name},
                               {"farm", zone_to_toml(map.farm)},
                               {"town", zone_to_toml(map.town)},
                               {"dungeon", std::move(dungeon)}};

    // Escribe en un archivo aparte y después lo renombra, para no romper el mapa anterior si falla
    // a mitad de camino.
    const std::string temporary = path + ".tmp";
    std::ofstream file(temporary);
    file << document << '\n';
    file.close();

    std::error_code error;
    if (file) {
        std::filesystem::rename(temporary, path, error);
    }
    if (!file || error) {
        std::filesystem::remove(temporary, error);
        throw MapFileError("No se pudo guardar el mapa en " + path);
    }
}

}
