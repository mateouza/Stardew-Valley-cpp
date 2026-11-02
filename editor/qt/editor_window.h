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

// Ventana principal del editor de mapas. Es un programa aparte: no necesita ni el cliente ni el
// servidor para correr. A la izquierda se elige la zona y el pincel; a la derecha se pinta.
class EditorWindow: public QMainWindow {
    Q_OBJECT

private:
    WorldMap map;
    bool modified = false;
    // De dónde se abrió o dónde se guardó el mapa; vacío si todavía no tiene archivo.
    QString current_path;
    QComboBox* zone_selector;
    QListWidget* palette;
    TileGridView* grid;

    // 0 es la granja, 1 el pueblo y desde 2, los niveles de la mazmorra en orden.
    Zone& zone_at(int index);
    void show_zone(Zone& zone);
    void choose_brush(int palette_row);
    // Muestra la granja de un mapa recién creado o abierto, todavía sin cambios.
    void show_map();

    // Las acciones del menú: piden el archivo con un diálogo y muestran los errores.
    void ask_open();
    void ask_save();
    void ask_save_as();
    void show_error(const QString& message);

public:
    explicit EditorWindow(QWidget* parent = nullptr);

    void new_map();

    // Devuelven el mensaje para el usuario si algo falla, o nada si salió bien. No abren diálogos.
    // Si no se puede abrir el archivo, el mapa actual queda como estaba.
    std::optional<QString> open_map(const QString& path);
    // Un mapa con problemas no se guarda: el mensaje dice cuáles son.
    std::optional<QString> save_map_to(const QString& path);

    const WorldMap& get_map() const;

    // Si se pintó algo desde que se abrió el mapa.
    bool is_modified() const;
};

}

#endif
