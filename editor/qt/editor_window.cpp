#include "editor_window.h"

#include <QAbstractButton>
#include <QAction>
#include <QCloseEvent>
#include <QComboBox>
#include <QFileDialog>
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

#include "editor/model/map_file.h"
#include "editor/model/map_validation.h"
#include "qt/widgets/ui_helpers.h"

#include "tile_colors.h"
#include "tile_grid_view.h"

namespace editor {

namespace {

constexpr int SIDE_PANEL_WIDTH = 240;
constexpr const char* MAP_FILE_FILTER = "Mapas (*.toml)";
constexpr const char* NEW_MAP_NAME = "Mundo nuevo";

QIcon swatch(Tile tile) {
    QPixmap pixmap(18, 18);
    pixmap.fill(tile_color(tile));
    return QIcon(pixmap);
}

}

EditorWindow::EditorWindow(QWidget* parent):
        QMainWindow(parent),
        map(WorldMap::create_empty(NEW_MAP_NAME)),
        zone_selector(new QComboBox(this)),
        palette(new QListWidget(this)),
        grid(new TileGridView(this)) {
    setObjectName("editor_window");
    setProperty("role", "editor");
    setWindowTitle("Editor de mapas · Stardew Valley");
    resize(1100, 760);
    setMinimumSize(900, 600);

    QMenu* file_menu = menuBar()->addMenu("&Archivo");
    const auto add_action = [this, file_menu](const char* text, QKeySequence::StandardKey key,
                                              auto slot) {
        QAction* action = file_menu->addAction(text);
        action->setShortcut(key);
        connect(action, &QAction::triggered, this, slot);
    };
    add_action("&Nuevo", QKeySequence::New, &EditorWindow::ask_new);
    add_action("&Abrir…", QKeySequence::Open, &EditorWindow::ask_open);
    add_action("&Guardar", QKeySequence::Save, &EditorWindow::ask_save);
    add_action("Guardar &como…", QKeySequence::SaveAs, &EditorWindow::ask_save_as);
    file_menu->addSeparator();
    add_action("&Salir", QKeySequence::Quit, &QWidget::close);

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

void EditorWindow::show_map() {
    modified = false;
    const QSignalBlocker blocker(zone_selector);
    zone_selector->setCurrentIndex(0);
    show_zone(map.farm);
}

void EditorWindow::new_map() {
    map = WorldMap::create_empty(NEW_MAP_NAME);
    current_path.clear();
    show_map();
}

std::optional<QString> EditorWindow::open_map(const QString& path) {
    try {
        map = load_map(path.toStdString());
    } catch (const MapFileError& error) {
        return QString::fromStdString(error.what());
    }

    current_path = path;
    show_map();
    return std::nullopt;
}

std::optional<QString> EditorWindow::save_map_to(const QString& path) {
    const auto problems = validate(map);
    if (!problems.empty()) {
        QString message = "No se puede guardar mientras el mapa tenga problemas:";
        for (const MapProblem& problem: problems) {
            message += QString("\n• %1: %2")
                               .arg(QString::fromStdString(problem.place),
                                    QString::fromStdString(problem.description));
        }
        return message;
    }

    try {
        save_map(map, path.toStdString());
    } catch (const MapFileError& error) {
        return QString::fromStdString(error.what());
    }

    current_path = path;
    modified = false;
    return std::nullopt;
}

bool EditorWindow::can_leave_map() {
    if (!modified) {
        return true;
    }
    const QMessageBox::StandardButton answer = ask_about_changes();
    if (answer == QMessageBox::Save) {
        ask_save();
        return !modified;
    }
    return answer == QMessageBox::Discard;
}

void EditorWindow::ask_new() {
    if (can_leave_map()) {
        new_map();
    }
}

void EditorWindow::ask_open() {
    if (!can_leave_map()) {
        return;
    }
    const QString path = choose_open_path();
    if (path.isEmpty()) {
        return;
    }
    if (const auto error = open_map(path)) {
        show_error(*error);
    }
}

void EditorWindow::ask_save() {
    if (current_path.isEmpty()) {
        ask_save_as();
    } else if (const auto error = save_map_to(current_path)) {
        show_error(*error);
    }
}

void EditorWindow::ask_save_as() {
    QString path = choose_save_path();
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(".toml")) {
        path += ".toml";
    }
    if (const auto error = save_map_to(path)) {
        show_error(*error);
    }
}

QMessageBox::StandardButton EditorWindow::ask_about_changes() {
    QMessageBox box(QMessageBox::Warning, "Editor de mapas", "El mapa tiene cambios sin guardar.",
                    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, this);
    box.setInformativeText("¿Querés guardarlos?");
    box.button(QMessageBox::Save)->setText("Guardar");
    box.button(QMessageBox::Discard)->setText("Descartar");
    box.button(QMessageBox::Cancel)->setText("Cancelar");
    box.setDefaultButton(QMessageBox::Save);
    return static_cast<QMessageBox::StandardButton>(box.exec());
}

QString EditorWindow::choose_open_path() {
    return QFileDialog::getOpenFileName(this, "Abrir mapa", current_path, MAP_FILE_FILTER);
}

QString EditorWindow::choose_save_path() {
    return QFileDialog::getSaveFileName(this, "Guardar mapa", current_path, MAP_FILE_FILTER);
}

void EditorWindow::show_error(const QString& message) {
    QMessageBox::warning(this, "Editor de mapas", message);
}

void EditorWindow::closeEvent(QCloseEvent* event) {
    if (can_leave_map()) {
        event->accept();
    } else {
        event->ignore();
    }
}

const WorldMap& EditorWindow::get_map() const { return map; }

bool EditorWindow::is_modified() const { return modified; }

}
