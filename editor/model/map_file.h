#ifndef EDITOR_MODEL_MAP_FILE_H_
#define EDITOR_MODEL_MAP_FILE_H_

#include <stdexcept>
#include <string>

#include "world_map.h"

namespace editor {

inline constexpr int MAP_FILE_VERSION = 1;

class MapFileError: public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

void save_map(const WorldMap& map, const std::string& path);

WorldMap load_map(const std::string& path);

}

#endif
