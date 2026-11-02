#ifndef EDITOR_QT_TILE_GRID_VIEW_H_
#define EDITOR_QT_TILE_GRID_VIEW_H_

#include <QWidget>

#include "editor/model/zone.h"

class QPaintEvent;

namespace editor {

// Muestra una zona casilla por casilla, con los colores y las letras de tile_colors.h.
class TileGridView: public QWidget {
    Q_OBJECT

private:
    Zone* zone = nullptr;

protected:
    void paintEvent(QPaintEvent* event) override;

public:
    // Lado de cada casilla en pantalla, en píxeles.
    static constexpr int TILE_SIZE = 20;

    explicit TileGridView(QWidget* parent = nullptr);

    // No es dueña de la zona: quien se la pasa la mantiene viva mientras se muestre. Hay que volver
    // a llamarla si la zona cambia de tamaño. Con nullptr no muestra nada.
    void set_zone(Zone* new_zone);
};

}

#endif
