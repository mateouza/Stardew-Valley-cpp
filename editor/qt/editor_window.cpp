#include "editor_window.h"

#include <QAction>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QWidget>

#include "qt/widgets/ui_helpers.h"

namespace editor {

EditorWindow::EditorWindow(QWidget* parent): QMainWindow(parent) {
    setObjectName("editor_window");
    setProperty("role", "editor");
    setWindowTitle("Editor de mapas · Stardew Valley");
    resize(1100, 760);
    setMinimumSize(900, 600);

    QMenu* file_menu = menuBar()->addMenu("&Archivo");
    QAction* quit_action = file_menu->addAction("&Salir");
    quit_action->setShortcut(QKeySequence::Quit);
    connect(quit_action, &QAction::triggered, this, &QWidget::close);

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->addStretch();
    launcher::add_centered(layout, launcher::create_label("Editor de mapas", "panel_title"));
    launcher::add_centered(layout, launcher::create_hint("Acá se van a armar la granja, el pueblo "
                                                         "y los 10 niveles de la mazmorra."));
    layout->addStretch();
    setCentralWidget(central);
}

}
