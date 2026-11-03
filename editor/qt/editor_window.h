#ifndef EDITOR_QT_EDITOR_WINDOW_H_
#define EDITOR_QT_EDITOR_WINDOW_H_

#include <QMainWindow>
#include <QString>
#include <optional>

#include "editor/model/world_map.h"

class QComboBox;
class QListWidget;

namespace editor {

class TileGridView;

class EditorWindow: public QMainWindow {
    Q_OBJECT

private:
    WorldMap map;
    bool modified = false;
    QString current_path;
    QComboBox* zone_selector;
    QListWidget* palette;
    TileGridView* grid;

    // 0 granja, 1 pueblo, desde 2 niveles de la mazmorra.
    Zone& zone_at(int index);
    void show_zone(Zone& zone);
    void choose_brush(int palette_row);
    void show_map();

    void ask_open();
    void ask_save();
    void ask_save_as();
    void show_error(const QString& message);

public:
    explicit EditorWindow(QWidget* parent = nullptr);

    void new_map();

    std::optional<QString> open_map(const QString& path);
    std::optional<QString> save_map_to(const QString& path);

    const WorldMap& get_map() const;

    bool is_modified() const;
};

}

#endif
