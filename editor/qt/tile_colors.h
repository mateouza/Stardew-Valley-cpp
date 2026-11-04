#ifndef EDITOR_QT_TILE_COLORS_H_
#define EDITOR_QT_TILE_COLORS_H_

#include <QColor>

#include "editor/model/tile.h"

namespace editor {

QColor tile_color(Tile tile);

bool shows_letter(Tile tile);

}

#endif
