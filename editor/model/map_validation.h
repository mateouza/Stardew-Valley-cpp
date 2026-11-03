#ifndef EDITOR_MODEL_MAP_VALIDATION_H_
#define EDITOR_MODEL_MAP_VALIDATION_H_

#include <string>
#include <vector>

#include "world_map.h"

namespace editor {

struct MapProblem {
    std::string place;
    std::string description;
};

std::vector<MapProblem> validate(const WorldMap& map);

}

#endif
