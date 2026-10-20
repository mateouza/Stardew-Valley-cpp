#include "main_menu_screen.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "qt/widgets/ui_helpers.h"

namespace launcher {

MainMenuScreen::MainMenuScreen(QWidget* parent):
        QWidget(parent),
        session_label(create_label("", "session", this)),
        error_label(create_error_label(this)) {
    setObjectName("main_menu_screen");
    session_label->setObjectName("session_label");
    session_label->setAlignment(Qt::AlignCenter);

    QPushButton* new_game_button = create_button("Nueva partida", "primary", this);
    QPushButton* join_button = create_button("Unirse a partida", "primary", this);
    QPushButton* resume_button = create_button("Retomar partida", "primary", this);
    QPushButton* quit_button = create_button("Salir", "secondary", this);
    new_game_button->setObjectName("new_game_button");
    join_button->setObjectName("join_button");
    resume_button->setObjectName("resume_button");
    quit_button->setObjectName("quit_button");
    setFocusProxy(new_game_button);

    Panel panel = create_panel(380, this);
    panel.content->setSpacing(12);
    for (QPushButton* button: {new_game_button, join_button, resume_button, quit_button}) {
        panel.content->addWidget(button);
    }
    panel.content->addWidget(error_label);
    panel.content->addWidget(session_label);
    layout_screen(this, create_logo(this), panel.frame);

    connect(new_game_button, &QPushButton::clicked, this, &MainMenuScreen::new_game_requested);
    connect(join_button, &QPushButton::clicked, this, &MainMenuScreen::join_requested);
    connect(resume_button, &QPushButton::clicked, this, &MainMenuScreen::resume_requested);
    connect(quit_button, &QPushButton::clicked, this, &MainMenuScreen::quit_requested);
}

void MainMenuScreen::set_session(const QString& username, const QString& server) {
    session_label->setText(QString("Usuario: %1 · Servidor: %2").arg(username, server));
}

void MainMenuScreen::show_error(const QString& message) { set_error(error_label, message); }

}
