#include "tile_grid_view.h"

#include <QFont>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QString>
#include <algorithm>

#include "tile_colors.h"

namespace editor {

namespace {

constexpr int TILE_SIZE = TileGridView::TILE_SIZE;

QRect cell_rect(int column, int row) {
    return QRect(column * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE);
}

}

TileGridView::TileGridView(QWidget* parent): QWidget(parent) { set_zone(nullptr); }

void TileGridView::set_zone(Zone* new_zone) {
    zone = new_zone;
    if (zone == nullptr) {
        setFixedSize(0, 0);
    } else {
        setFixedSize(zone->width() * TILE_SIZE, zone->height() * TILE_SIZE);
    }
    update();
}

void TileGridView::set_brush(Tile tile) { brush = tile; }

Tile TileGridView::get_brush() const { return brush; }

void TileGridView::paintEvent(QPaintEvent* event) {
    if (zone == nullptr) {
        return;
    }
    QPainter painter(this);
    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(TILE_SIZE * 3 / 5);
    painter.setFont(font);

    const QRect dirty = event->rect();
    const int last_column = std::min(zone->width() - 1, dirty.right() / TILE_SIZE);
    const int last_row = std::min(zone->height() - 1, dirty.bottom() / TILE_SIZE);
    for (int row = std::max(0, dirty.top() / TILE_SIZE); row <= last_row; ++row) {
        for (int column = std::max(0, dirty.left() / TILE_SIZE); column <= last_column; ++column) {
            const QRect cell = cell_rect(column, row);
            const Tile tile = zone->at(column, row);
            painter.fillRect(cell, tile_color(tile));
            painter.setPen(QColor(0, 0, 0, 40));
            painter.drawRect(cell.adjusted(0, 0, -1, -1));
            if (shows_letter(tile)) {
                painter.setPen(QColor(0x3b, 0x2a, 0x1a));
                painter.drawText(cell, Qt::AlignCenter,
                                 QString(QChar::fromLatin1(tile_code(tile))));
            }
        }
    }
}

void TileGridView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        paint_at(event->position().toPoint());
    }
}

void TileGridView::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        paint_at(event->position().toPoint());
    }
}

void TileGridView::paint_at(const QPoint& position) {
    // Al arrastrar, el mouse puede salir del widget.
    if (zone == nullptr || position.x() < 0 || position.y() < 0) {
        return;
    }
    const int column = position.x() / TILE_SIZE;
    const int row = position.y() / TILE_SIZE;
    if (!zone->contains(column, row) || !is_allowed(zone->kind(), brush) ||
        zone->at(column, row) == brush) {
        return;
    }
    zone->set(column, row, brush);
    update(cell_rect(column, row));
    emit tile_painted(column, row);
}

}
