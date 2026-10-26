#ifndef EDITOR_MODEL_MAP_FILE_H_
#define EDITOR_MODEL_MAP_FILE_H_

#include <stdexcept>
#include <string>

#include "world_map.h"

namespace editor {

// Formato provisorio hasta acordarlo con el servidor: un archivo TOML con version, name y una
// tabla por zona ([farm], [town] y un [[dungeon]] por nivel). Cada zona tiene width, height y
// rows: una fila de texto por fila de la zona, con la letra de tile_code() de cada casilla.
inline constexpr int MAP_FILE_VERSION = 1;

// Error al guardar o al leer un mapa, con un mensaje que se le puede mostrar al usuario.
class MapFileError: public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Guarda el mapa tal como está, sin validarlo: eso lo decide el editor. Si falla, lanza
// MapFileError y deja como estaba el archivo que hubiera en path.
void save_map(const WorldMap& map, const std::string& path);

// Lanza MapFileError si no puede leer el archivo o si no respeta el formato, por ejemplo con una
// zona de más de Zone::MAX_SIDE de lado. No revisa las reglas de validate(): de eso se ocupa el
// editor después de abrirlo.
WorldMap load_map(const std::string& path);

}

#endif
