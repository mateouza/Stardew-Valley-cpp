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

void layout_screen(QWidget* screen, QWidget* header, QWidget* panel);

QLabel* create_label(const QString& text, const char* role, QWidget* parent = nullptr);

QLabel* create_hint(const QString& text, QWidget* parent = nullptr);

QLabel* create_error_label(QWidget* parent = nullptr);

QPushButton* create_button(const QString& text, const char* variant, QWidget* parent = nullptr);

QHBoxLayout* create_button_row(QPushButton* left, QPushButton* right);

// Con Qt::AlignHCenter, las etiquetas con word wrap quedan cortadas.
void add_centered(QVBoxLayout* layout, QWidget* widget);

void add_field(QVBoxLayout* layout, const QString& caption, QWidget* field);

void set_name_rules(QLineEdit* input, std::size_t max_length);

void set_error(QLabel* error_label, const QString& message);

}

#endif
