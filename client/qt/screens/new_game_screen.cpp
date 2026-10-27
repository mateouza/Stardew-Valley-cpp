#include "new_game_screen.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <cstddef>

#include "lobby/lobby_validation.h"
#include "qt/widgets/ui_helpers.h"
#include "qt/widgets/world_card.h"

namespace launcher {

NewGameScreen::NewGameScreen(QWidget* parent):
        QWidget(parent),
        game_name_input(new QLineEdit(this)),
        worlds_layout(new QVBoxLayout()),
        world_buttons(new QButtonGroup(this)),
        continue_button(create_button("Siguiente: personaje", "primary", this)),
        error_label(create_error_label(this)) {
    setObjectName("new_game_screen");
    setFocusProxy(game_name_input);

    game_name_input->setObjectName("game_name_input");
    game_name_input->setPlaceholderText("Ej: Granja Los Girasoles");
    set_name_rules(game_name_input, lobby::MAX_GAME_NAME_LENGTH);
    continue_button->setObjectName("continue_button");

    QPushButton* back_button = create_button("Volver", "secondary", this);
    back_button->setObjectName("back_button");
    back_button->setShortcut(QKeySequence(Qt::Key_Escape));

    auto* worlds_container = new QWidget();
    worlds_layout->setContentsMargins(0, 0, 4, 0);
    worlds_layout->setSpacing(8);
    worlds_layout->addStretch();
    worlds_container->setLayout(worlds_layout);

    auto* worlds_scroll = new QScrollArea(this);
    worlds_scroll->setWidget(worlds_container);
    worlds_scroll->setWidgetResizable(true);
    worlds_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    worlds_scroll->setFixedHeight(252);

    Panel panel = create_panel(560, this);
    add_field(panel.content, "Nombre de la partida", game_name_input);
    add_field(panel.content, "Elegí un mundo", worlds_scroll);
    panel.content->addWidget(error_label);
    panel.content->addSpacing(8);
    panel.content->addLayout(create_button_row(back_button, continue_button));
    layout_screen(this, create_screen_title("Nueva partida", this), panel.frame);

    connect(game_name_input, &QLineEdit::textChanged, this, &NewGameScreen::update_continue_button);
    connect(game_name_input, &QLineEdit::returnPressed, this, &NewGameScreen::submit);
    connect(world_buttons, &QButtonGroup::idToggled, this, &NewGameScreen::update_continue_button);
    connect(back_button, &QPushButton::clicked, this, &NewGameScreen::back_requested);
    connect(continue_button, &QPushButton::clicked, this, &NewGameScreen::submit);
    update_continue_button();
}

void NewGameScreen::reset(const std::vector<lobby::WorldInfo>& available_worlds) {
    for (QAbstractButton* button: world_buttons->buttons()) {
        world_buttons->removeButton(button);
        delete button;
    }

    worlds = available_worlds;
    for (std::size_t i = 0; i < worlds.size(); ++i) {
        auto* card = new WorldCard(worlds[i]);
        world_buttons->addButton(card, static_cast<int>(i));
        worlds_layout->insertWidget(static_cast<int>(i), card);
    }
    if (!worlds.empty()) {
        world_buttons->button(0)->setChecked(true);
    }

    game_name_input->clear();
    set_error(error_label, worlds.empty() ? "El servidor no tiene mundos disponibles." : "");
    update_continue_button();
}

void NewGameScreen::show_error(const QString& message) { set_error(error_label, message); }

void NewGameScreen::update_continue_button() {
    continue_button->setEnabled(lobby::is_valid_game_name(game_name_input->text().toStdString()) &&
                                world_buttons->checkedId() >= 0);
}

void NewGameScreen::submit() {
    if (!continue_button->isEnabled()) {
        return;
    }

    const lobby::WorldInfo& world = worlds[static_cast<std::size_t>(world_buttons->checkedId())];
    set_error(error_label, "");
    emit continue_requested(game_name_input->text().trimmed(), QString::fromStdString(world.id),
                            QString::fromStdString(world.name));
}

}
