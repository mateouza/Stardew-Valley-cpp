#ifndef EDITOR_QT_EDITOR_WINDOW_H_
#define EDITOR_QT_EDITOR_WINDOW_H_

#include <QMainWindow>

namespace editor {

// Ventana principal del editor de mapas. Es un programa aparte: no necesita ni el cliente ni el
// servidor para correr.
class EditorWindow: public QMainWindow {
    Q_OBJECT

public:
    explicit EditorWindow(QWidget* parent = nullptr);
};

}

#endif
