#include "join_screen.h"

#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

#include "qt/widgets/ui_helpers.h"

namespace launcher {

namespace {

constexpr int MAX_INVITE_CODE_LENGTH = 12;

}

JoinScreen::JoinScreen(QWidget* parent):
        QWidget(parent),
        invite_code_input(new QLineEdit(this)),
        find_game_button(create_button("Buscar partida", "primary", this)),
        error_label(create_error_label(this)) {
    setObjectName("join_screen");
    setFocusProxy(invite_code_input);

    invite_code_input->setObjectName("invite_code_input");
    invite_code_input->setProperty("role", "invite_code");
    invite_code_input->setAlignment(Qt::AlignCenter);
    invite_code_input->setPlaceholderText("Ej: ABC123");
    invite_code_input->setMaxLength(MAX_INVITE_CODE_LENGTH);
    invite_code_input->setValidator(new QRegularExpressionValidator(
            QRegularExpression("[A-Za-z0-9-]*"), invite_code_input));
    find_game_button->setObjectName("find_game_button");

    QPushButton* back_button = create_button("Volver", "secondary", this);
    back_button->setObjectName("back_button");
    back_button->setShortcut(QKeySequence(Qt::Key_Escape));

    Panel panel = create_panel(480, this);
    add_field(panel.content, "Código de invitación", invite_code_input);
    panel.content->addWidget(create_hint("Pedile el código a quien creó la partida: lo genera "
                                         "desde Configuración → Código Multiplayer."));
    panel.content->addWidget(error_label);
    panel.content->addSpacing(8);
    panel.content->addLayout(create_button_row(back_button, find_game_button));
    layout_screen(this, create_screen_title("Unirse a una partida", this), panel.frame);

    connect(invite_code_input, &QLineEdit::textEdited, this, [this](const QString& text) {
        const int cursor = invite_code_input->cursorPosition();
        invite_code_input->setText(text.toUpper());
        invite_code_input->setCursorPosition(cursor);
    });
    connect(invite_code_input, &QLineEdit::textChanged, this, &JoinScreen::update_find_game_button);
    connect(invite_code_input, &QLineEdit::returnPressed, this, &JoinScreen::submit);
    connect(back_button, &QPushButton::clicked, this, &JoinScreen::back_requested);
    connect(find_game_button, &QPushButton::clicked, this, &JoinScreen::submit);
    update_find_game_button();
}

void JoinScreen::reset() {
    invite_code_input->clear();
    set_error(error_label, "");
}

void JoinScreen::show_error(const QString& message) { set_error(error_label, message); }

void JoinScreen::update_find_game_button() {
    find_game_button->setEnabled(!invite_code_input->text().trimmed().isEmpty());
}

void JoinScreen::submit() {
    if (!find_game_button->isEnabled()) {
        return;
    }

    set_error(error_label, "");
    emit invite_code_submitted(invite_code_input->text().trimmed());
}

}
