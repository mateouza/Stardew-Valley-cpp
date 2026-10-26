#ifndef CLIENT_QT_WIDGETS_UI_HELPERS_H_
#define CLIENT_QT_WIDGETS_UI_HELPERS_H_

#include <QString>
#include <cstddef>

class QFrame;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;
class QWidget;

namespace launcher {

struct Panel {
    QFrame* frame;
    QVBoxLayout* content;
};

QWidget* create_logo(QWidget* parent);

QLabel* create_screen_title(const QString& text, QWidget* parent);

Panel create_panel(int width, QWidget* parent);

// Encabezado y panel centrados, con más aire abajo que arriba.
void layout_screen(QWidget* screen, QWidget* header, QWidget* panel);

QLabel* create_label(const QString& text, const char* role, QWidget* parent = nullptr);

QLabel* create_hint(const QString& text, QWidget* parent = nullptr);

// Etiqueta de error con objectName "error_label", oculta mientras no tenga mensaje.
QLabel* create_error_label(QWidget* parent = nullptr);

QPushButton* create_button(const QString& text, const char* variant, QWidget* parent = nullptr);

// Botón secundario a la izquierda (volver, salir) y la acción principal a la derecha.
QHBoxLayout* create_button_row(QPushButton* left, QPushButton* right);

// Centra horizontalmente con stretches en vez de Qt::AlignHCenter: con alineación, Qt calcula la
// altura de las etiquetas con word wrap usando el ancho de toda la fila y el panel queda cortado.
void add_centered(QVBoxLayout* layout, QWidget* widget);

void add_field(QVBoxLayout* layout, const QString& caption, QWidget* field);

// Deja escribir o pegar solo los caracteres de lobby::count_name_characters(), hasta max_length.
void set_name_rules(QLineEdit* input, std::size_t max_length);

void set_error(QLabel* error_label, const QString& message);

}

#endif
