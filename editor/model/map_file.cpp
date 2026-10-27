#include "map_file.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
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

int read_side(const toml::table& zone, const char* key, const std::string& place) {
    const std::optional<std::int64_t> side = zone[key].value_exact<std::int64_t>();
    if (!side || *side < 1 || *side > Zone::MAX_SIDE) {
        throw MapFileError(place + ": " + key + " tiene que ser un entero entre 1 y " +
                           std::to_string(Zone::MAX_SIDE));
    }
    return static_cast<int>(*side);
}

Zone zone_from_toml(const toml::table* table, ZoneKind kind, const std::string& place) {
    if (table == nullptr) {
        throw MapFileError(place + ": falta la tabla de la zona");
    }
    // read_side controla el tamaño antes de que Zone reserve ancho por alto casillas.
    Zone zone(kind, read_side(*table, "width", place), read_side(*table, "height", place));

    const toml::array* rows = (*table)["rows"].as_array();
    if (rows == nullptr || rows->size() != static_cast<std::size_t>(zone.height())) {
        throw MapFileError(place + ": rows tiene que tener " + std::to_string(zone.height()) +
                           " filas");
    }
    for (int row = 0; row < zone.height(); ++row) {
        const std::string where = place + ", fila " + std::to_string(row + 1);
        const std::optional<std::string> line = (*rows)[row].value_exact<std::string>();
        if (!line || line->size() != static_cast<std::size_t>(zone.width())) {
            throw MapFileError(where + ": tiene que ser un texto de " +
                               std::to_string(zone.width()) + " letras");
        }
        for (int column = 0; column < zone.width(); ++column) {
            const std::optional<Tile> tile = tile_from_code((*line)[column]);
            if (!tile || !is_allowed(kind, *tile)) {
                throw MapFileError(where + ", columna " + std::to_string(column + 1) +
                                   ": no es una casilla de esta zona");
            }
            zone.set(column, row, *tile);
        }
    }
    return zone;
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

WorldMap load_map(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw MapFileError("No se pudo abrir " + path);
    }
    toml::table document;
    try {
        document = toml::parse(file, path);
    } catch (const toml::parse_error& error) {
        throw MapFileError("No es un archivo TOML válido (línea " +
                           std::to_string(error.source().begin.line) +
                           "): " + std::string(error.description()));
    }

    if (document["version"].value_exact<std::int64_t>() != MAP_FILE_VERSION) {
        throw MapFileError("La versión del formato tiene que ser " +
                           std::to_string(MAP_FILE_VERSION));
    }
    const std::optional<std::string> name = document["name"].value_exact<std::string>();
    if (!name) {
        throw MapFileError("Falta el nombre del mundo");
    }
    const toml::array* levels = document["dungeon"].as_array();
    if (levels == nullptr || levels->size() != static_cast<std::size_t>(WorldMap::DUNGEON_LEVELS)) {
        throw MapFileError("La mazmorra tiene que tener " +
                           std::to_string(WorldMap::DUNGEON_LEVELS) + " niveles");
    }

    WorldMap map = WorldMap::create_empty(*name);
    map.farm = zone_from_toml(document["farm"].as_table(), ZoneKind::Farm, "Granja");
    map.town = zone_from_toml(document["town"].as_table(), ZoneKind::Town, "Pueblo");
    for (int level = 0; level < WorldMap::DUNGEON_LEVELS; ++level) {
        map.dungeon[level] = zone_from_toml((*levels)[level].as_table(), ZoneKind::Dungeon,
                                            "Mazmorra, nivel " + std::to_string(level + 1));
    }
    return map;
}

}
