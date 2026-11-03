#ifndef EDITOR_QT_TILE_GRID_VIEW_H_
#define EDITOR_QT_TILE_GRID_VIEW_H_

#include <QPoint>
#include <QWidget>

#include "editor/model/zone.h"

class QMouseEvent;
class QPaintEvent;

namespace editor {

class TileGridView: public QWidget {
    Q_OBJECT

private:
    Zone* zone = nullptr;
    Tile brush = Tile::Grass;

    void paint_at(const QPoint& position);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

public:
    static constexpr int TILE_SIZE = 20;

    explicit TileGridView(QWidget* parent = nullptr);

    // No es dueña de la zona.
    void set_zone(Zone* new_zone);

    void set_brush(Tile tile);
    Tile get_brush() const;

signals:
    void tile_painted(int column, int row);
};

}

#endif
