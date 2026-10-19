#ifndef CLIENT_QT_WIDGETS_UI_HELPERS_H_
#define CLIENT_QT_WIDGETS_UI_HELPERS_H_

#include <QString>

class QFrame;
class QLabel;
class QPushButton;
class QVBoxLayout;
class QWidget;

namespace launcher {

struct Panel {
    QFrame* frame;
    QVBoxLayout* content;
};

QWidget* create_logo(QWidget* parent);

Panel create_panel(int width, QWidget* parent);

QLabel* create_label(const QString& text, const char* role, QWidget* parent = nullptr);

QLabel* create_hint(const QString& text, QWidget* parent = nullptr);

// Etiqueta de error con objectName "error_label", oculta mientras no tenga mensaje.
QLabel* create_error_label(QWidget* parent = nullptr);

QPushButton* create_button(const QString& text, const char* variant, QWidget* parent = nullptr);

// Centra horizontalmente con stretches en vez de Qt::AlignHCenter: con alineación, Qt calcula la
// altura de las etiquetas con word wrap usando el ancho de toda la fila y el panel queda cortado.
void add_centered(QVBoxLayout* layout, QWidget* widget);

void add_field(QVBoxLayout* layout, const QString& caption, QWidget* field);

void set_error(QLabel* error_label, const QString& message);

}

#endif
