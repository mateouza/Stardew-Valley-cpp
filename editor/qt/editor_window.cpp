#include "editor_window.h"

#include <QAction>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequence>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QPixmap>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QWidget>

#include "qt/widgets/ui_helpers.h"

#include "tile_colors.h"
#include "tile_grid_view.h"

namespace editor {

namespace {

constexpr int SIDE_PANEL_WIDTH = 240;

QIcon swatch(Tile tile) {
    QPixmap pixmap(18, 18);
    pixmap.fill(tile_color(tile));
    return QIcon(pixmap);
}

}

EditorWindow::EditorWindow(QWidget* parent):
        QMainWindow(parent),
        map(WorldMap::create_empty("Mundo nuevo")),
        palette(new QListWidget(this)),
        grid(new TileGridView(this)) {
    setObjectName("editor_window");
    setProperty("role", "editor");
    setWindowTitle("Editor de mapas · Stardew Valley");
    resize(1100, 760);
    setMinimumSize(900, 600);

    QMenu* file_menu = menuBar()->addMenu("&Archivo");
    QAction* quit_action = file_menu->addAction("&Salir");
    quit_action->setShortcut(QKeySequence::Quit);
    connect(quit_action, &QAction::triggered, this, &QWidget::close);

    palette->setObjectName("palette");
    grid->setObjectName("tile_grid");

    auto* side_panel = new QWidget(this);
    side_panel->setFixedWidth(SIDE_PANEL_WIDTH);
    auto* side_layout = new QVBoxLayout(side_panel);
    side_layout->setContentsMargins(0, 0, 0, 0);
    launcher::add_field(side_layout, "Pincel", palette);

    auto* scroll = new QScrollArea(this);
    scroll->setWidget(grid);
    scroll->setAlignment(Qt::AlignCenter);

    auto* central = new QWidget(this);
    auto* layout = new QHBoxLayout(central);
    layout->addWidget(side_panel);
    layout->addWidget(scroll, 1);
    setCentralWidget(central);

    connect(palette, &QListWidget::currentRowChanged, this, &EditorWindow::choose_brush);
    connect(grid, &TileGridView::tile_painted, this, [this] { modified = true; });
    show_zone(map.farm);
}

void EditorWindow::show_zone(Zone& zone) {
    grid->set_zone(&zone);

    // La paleta muestra las casillas que admite la zona.
    const QSignalBlocker blocker(palette);
    palette->clear();
    const auto& tiles = tiles_for(zone.kind());
    for (Tile tile: tiles) {
        auto* item = new QListWidgetItem(swatch(tile), tile_name(tile), palette);
        item->setData(Qt::UserRole, static_cast<int>(tile));
    }

    palette->setCurrentRow(0);
    grid->set_brush(tiles.front());
}

void EditorWindow::choose_brush(int palette_row) {
    if (palette_row >= 0) {
        grid->set_brush(static_cast<Tile>(palette->item(palette_row)->data(Qt::UserRole).toInt()));
    }
}

const WorldMap& EditorWindow::get_map() const { return map; }

bool EditorWindow::is_modified() const { return modified; }

}
