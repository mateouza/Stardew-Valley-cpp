#ifndef EDITOR_MODEL_MAP_VALIDATION_H_
#define EDITOR_MODEL_MAP_VALIDATION_H_

#include <string>
#include <vector>

#include "world_map.h"

namespace editor {

// Algo que falta o sobra en el mundo. El editor no deja guardar mientras haya alguno.
struct MapProblem {
    // "Nombre del mundo", "Granja", "Pueblo" o "Mazmorra, nivel N".
    std::string place;
    std::string description;
};

// Reglas provisorias hasta acordarlas con el servidor:
// - el nombre sigue la misma regla que el de una partida;
// - granja: una cabaña y al menos una salida al pueblo;
// - pueblo: uno de cada comerciante, un centro comunitario, una entrada a la mazmorra y al menos
//   una salida a la granja;
// - cada nivel de la mazmorra: al menos una salida, y una escalera al siguiente nivel salvo el
//   último, que no tiene ninguna.
std::vector<MapProblem> validate(const WorldMap& map);

}

#endif
