#ifndef EDITOR_QT_TILE_COLORS_H_
#define EDITOR_QT_TILE_COLORS_H_

#include <QColor>

#include "editor/model/tile.h"

namespace editor {

// Cómo se ve cada casilla en el editor, hasta que haya assets: un color, y encima la letra de
// tile_code() para las que no son terreno. Los comerciantes comparten color, y las salidas también:
// se distinguen por la letra.
QColor tile_color(Tile tile);

bool shows_letter(Tile tile);

}

#endif
