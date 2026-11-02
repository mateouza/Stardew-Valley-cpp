#include "editor_window.h"

#include <QAction>
#include <QComboBox>
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
#include <algorithm>

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
        zone_selector(new QComboBox(this)),
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

    zone_selector->setObjectName("zone_selector");
    zone_selector->addItems({"Granja", "Pueblo"});
    for (int level = 1; level <= WorldMap::DUNGEON_LEVELS; ++level) {
        zone_selector->addItem(QString("Mazmorra, nivel %1").arg(level));
    }
    palette->setObjectName("palette");
    grid->setObjectName("tile_grid");

    auto* side_panel = new QWidget(this);
    side_panel->setFixedWidth(SIDE_PANEL_WIDTH);
    auto* side_layout = new QVBoxLayout(side_panel);
    side_layout->setContentsMargins(0, 0, 0, 0);
    launcher::add_field(side_layout, "Zona", zone_selector);
    launcher::add_field(side_layout, "Pincel", palette);

    auto* scroll = new QScrollArea(this);
    scroll->setWidget(grid);
    scroll->setAlignment(Qt::AlignCenter);

    auto* central = new QWidget(this);
    auto* layout = new QHBoxLayout(central);
    layout->addWidget(side_panel);
    layout->addWidget(scroll, 1);
    setCentralWidget(central);

    connect(zone_selector, &QComboBox::currentIndexChanged, this,
            [this](int index) { show_zone(zone_at(index)); });
    connect(palette, &QListWidget::currentRowChanged, this, &EditorWindow::choose_brush);
    connect(grid, &TileGridView::tile_painted, this, [this] { modified = true; });
    show_zone(map.farm);
}

Zone& EditorWindow::zone_at(int index) {
    if (index == 0) {
        return map.farm;
    }
    if (index == 1) {
        return map.town;
    }
    return map.dungeon.at(index - 2);
}

void EditorWindow::show_zone(Zone& zone) {
    grid->set_zone(&zone);

    // La paleta cambia con la zona. Si el pincel también sirve en la nueva, se mantiene.
    const QSignalBlocker blocker(palette);
    palette->clear();
    const auto& tiles = tiles_for(zone.kind());
    for (Tile tile: tiles) {
        auto* item = new QListWidgetItem(swatch(tile), tile_name(tile), palette);
        item->setData(Qt::UserRole, static_cast<int>(tile));
    }

    const auto kept = std::find(tiles.begin(), tiles.end(), grid->get_brush());
    const int row = kept == tiles.end() ? 0 : static_cast<int>(kept - tiles.begin());
    palette->setCurrentRow(row);
    grid->set_brush(tiles[row]);
}

void EditorWindow::choose_brush(int palette_row) {
    if (palette_row >= 0) {
        grid->set_brush(static_cast<Tile>(palette->item(palette_row)->data(Qt::UserRole).toInt()));
    }
}

const WorldMap& EditorWindow::get_map() const { return map; }

bool EditorWindow::is_modified() const { return modified; }

}
