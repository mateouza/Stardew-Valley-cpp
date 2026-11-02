#ifndef EDITOR_QT_EDITOR_WINDOW_H_
#define EDITOR_QT_EDITOR_WINDOW_H_

#include <QMainWindow>

#include "editor/model/world_map.h"

class QComboBox;
class QListWidget;

namespace editor {

class TileGridView;

// Ventana principal del editor de mapas. Es un programa aparte: no necesita ni el cliente ni el
// servidor para correr. A la izquierda se elige la zona y el pincel; a la derecha se pinta.
class EditorWindow: public QMainWindow {
    Q_OBJECT

private:
    WorldMap map;
    bool modified = false;
    QComboBox* zone_selector;
    QListWidget* palette;
    TileGridView* grid;

    // 0 es la granja, 1 el pueblo y desde 2, los niveles de la mazmorra en orden.
    Zone& zone_at(int index);
    void show_zone(Zone& zone);
    void choose_brush(int palette_row);

public:
    explicit EditorWindow(QWidget* parent = nullptr);

    const WorldMap& get_map() const;

    // Si se pintó algo desde que se abrió el mapa.
    bool is_modified() const;
};

}

#endif
