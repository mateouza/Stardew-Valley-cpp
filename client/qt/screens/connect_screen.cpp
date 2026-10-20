#include "connect_screen.h"

#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

#include "lobby/lobby_types.h"
#include "qt/widgets/ui_helpers.h"

namespace launcher {

ConnectScreen::ConnectScreen(const LauncherOptions& options, QWidget* parent):
        QWidget(parent),
        username_input(new QLineEdit(QString::fromStdString(options.username), this)),
        host_input(new QLineEdit(QString::fromStdString(options.host), this)),
        port_input(new QLineEdit(QString::fromStdString(options.port), this)),
        connect_button(create_button("Conectar", "primary", this)),
        error_label(create_error_label(this)) {
    setObjectName("connect_screen");
    setFocusProxy(username_input);

    username_input->setObjectName("username_input");
    username_input->setPlaceholderText("Ej: Juanceto01");
    username_input->setMaxLength(static_cast<int>(lobby::MAX_USERNAME_LENGTH));
    username_input->setValidator(
            new QRegularExpressionValidator(QRegularExpression("[A-Za-z0-9_]*"), username_input));
    host_input->setObjectName("host_input");
    port_input->setObjectName("port_input");
    port_input->setValidator(new QIntValidator(1, 65535, port_input));
    connect_button->setObjectName("connect_button");

    QPushButton* quit_button = create_button("Salir", "secondary", this);
    quit_button->setObjectName("quit_button");

    Panel panel = create_panel(480, this);
    panel.content->addWidget(create_label("Conectarse al servidor", "panel_title"));
    add_field(panel.content, "Nombre de usuario", username_input);
    panel.content->addWidget(create_hint("Es tu identidad en el servidor: con este nombre retomás "
                                         "tus partidas y te escriben por el chat."));

    auto* host_column = new QVBoxLayout();
    auto* port_column = new QVBoxLayout();
    add_field(host_column, "Servidor", host_input);
    add_field(port_column, "Puerto", port_input);
    auto* address_row = new QHBoxLayout();
    address_row->addLayout(host_column, 3);
    address_row->addLayout(port_column, 1);
    panel.content->addLayout(address_row);
    panel.content->addWidget(error_label);
    panel.content->addSpacing(8);
    panel.content->addLayout(create_button_row(quit_button, connect_button));
    layout_screen(this, create_logo(this), panel.frame);

    for (QLineEdit* input: {username_input, host_input, port_input}) {
        connect(input, &QLineEdit::textChanged, this, &ConnectScreen::update_connect_button);
        connect(input, &QLineEdit::returnPressed, this, &ConnectScreen::submit);
    }
    connect(connect_button, &QPushButton::clicked, this, &ConnectScreen::submit);
    connect(quit_button, &QPushButton::clicked, this, &ConnectScreen::quit_requested);
    update_connect_button();
}

void ConnectScreen::update_connect_button() {
    connect_button->setEnabled(!username_input->text().trimmed().isEmpty() &&
                               !host_input->text().trimmed().isEmpty() &&
                               !port_input->text().trimmed().isEmpty());
}

void ConnectScreen::submit() {
    if (!connect_button->isEnabled()) {
        return;
    }

    set_error(error_label, "");
    emit connect_requested(username_input->text().trimmed(), host_input->text().trimmed(),
                           port_input->text().trimmed());
}

void ConnectScreen::show_error(const QString& message) { set_error(error_label, message); }

}
