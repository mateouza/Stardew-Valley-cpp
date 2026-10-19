#include "ui_helpers.h"

#include <QColor>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

namespace launcher {

namespace {

void add_title_shadow(QWidget* widget) {
    auto* shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(0);
    shadow->setOffset(3, 3);
    shadow->setColor(QColor(0x5b, 0x3a, 0x1e));
    widget->setGraphicsEffect(shadow);
}

}

QWidget* create_logo(QWidget* parent) {
    auto* logo = new QWidget(parent);
    auto* layout = new QVBoxLayout(logo);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    for (QLabel* label:
         {create_label("Stardew Valley", "logo_title", logo),
          create_label("Edición Taller de Programación · FIUBA", "logo_subtitle", logo)}) {
        label->setAlignment(Qt::AlignCenter);
        add_title_shadow(label);
        layout->addWidget(label);
    }
    return logo;
}

Panel create_panel(int width, QWidget* parent) {
    auto* frame = new QFrame(parent);
    frame->setProperty("role", "panel");
    frame->setFixedWidth(width);

    auto* content = new QVBoxLayout(frame);
    content->setContentsMargins(30, 24, 30, 26);
    content->setSpacing(8);
    return Panel{frame, content};
}

QLabel* create_label(const QString& text, const char* role, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setProperty("role", role);
    label->setTextFormat(Qt::PlainText);
    return label;
}

QLabel* create_hint(const QString& text, QWidget* parent) {
    QLabel* hint = create_label(text, "hint", parent);
    hint->setWordWrap(true);
    return hint;
}

QLabel* create_error_label(QWidget* parent) {
    QLabel* error_label = create_label("", "error", parent);
    error_label->setObjectName("error_label");
    error_label->setWordWrap(true);
    error_label->hide();
    return error_label;
}

QPushButton* create_button(const QString& text, const char* variant, QWidget* parent) {
    auto* button = new QPushButton(text, parent);
    button->setProperty("variant", variant);
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

void add_centered(QVBoxLayout* layout, QWidget* widget) {
    auto* row = new QHBoxLayout();
    row->addStretch();
    row->addWidget(widget);
    row->addStretch();
    layout->addLayout(row);
}

void add_field(QVBoxLayout* layout, const QString& caption, QWidget* field) {
    layout->addSpacing(4);
    layout->addWidget(create_label(caption, "field_label"));
    layout->addWidget(field);
}

void set_error(QLabel* error_label, const QString& message) {
    error_label->setText(message);
    error_label->setVisible(!message.isEmpty());
}

}
