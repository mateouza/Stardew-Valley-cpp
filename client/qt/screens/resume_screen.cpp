#include "resume_screen.h"

#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <algorithm>
#include <cstddef>

#include "qt/widgets/saved_game_card.h"
#include "qt/widgets/ui_helpers.h"

namespace launcher {

namespace {

constexpr int CARD_SPACING = 10;
constexpr int VISIBLE_CARDS = 3;

}

ResumeScreen::ResumeScreen(QWidget* parent):
        QWidget(parent),
        games_container(new QWidget()),
        games_layout(new QVBoxLayout(games_container)),
        games_scroll(new QScrollArea(this)),
        intro_label(create_hint("Elegí una partida para seguir jugando.", this)),
        empty_label(create_hint("Todavía no tenés partidas guardadas. Creá una desde «Nueva "
                                "partida» o unite a la de alguien con su código.",
                                this)),
        error_label(create_error_label(this)) {
    setObjectName("resume_screen");

    games_layout->setContentsMargins(0, 0, 4, 0);
    games_layout->setSpacing(CARD_SPACING);
    games_layout->addStretch();
    games_scroll->setObjectName("games_scroll");
    games_scroll->setWidget(games_container);
    games_scroll->setWidgetResizable(true);
    games_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    intro_label->setObjectName("intro_label");
    empty_label->setObjectName("empty_label");
    empty_label->setAlignment(Qt::AlignCenter);

    QPushButton* back_button = create_button("Volver", "secondary", this);
    back_button->setObjectName("back_button");
    back_button->setShortcut(QKeySequence(Qt::Key_Escape));
    setFocusProxy(back_button);

    auto* buttons = new QHBoxLayout();
    buttons->addWidget(back_button);
    buttons->addStretch();

    Panel panel = create_panel(600, this);
    panel.content->addWidget(intro_label);
    panel.content->addWidget(games_scroll);
    panel.content->addWidget(empty_label);
    panel.content->addWidget(error_label);
    panel.content->addSpacing(8);
    panel.content->addLayout(buttons);
    layout_screen(this, create_screen_title("Retomar partida", this), panel.frame);

    connect(back_button, &QPushButton::clicked, this, &ResumeScreen::back_requested);
}

void ResumeScreen::set_games(const std::vector<lobby::SavedGameInfo>& games) {
    qDeleteAll(games_container->findChildren<SavedGameCard*>(Qt::FindDirectChildrenOnly));

    for (std::size_t i = 0; i < games.size(); ++i) {
        auto* card = new SavedGameCard(games[i]);
        games_layout->insertWidget(static_cast<int>(i), card);

        const QString game_id = QString::fromStdString(games[i].game_id);
        connect(card, &QPushButton::clicked, this,
                [this, game_id] { emit game_selected(game_id); });
    }

    // La lista crece con las partidas hasta VISIBLE_CARDS tarjetas; a partir de ahí, scroll.
    const int visible_cards = std::min(static_cast<int>(games.size()), VISIBLE_CARDS);
    games_scroll->setFixedHeight(
            std::max(0, visible_cards * (SavedGameCard::HEIGHT + CARD_SPACING) - CARD_SPACING));

    intro_label->setVisible(!games.empty());
    games_scroll->setVisible(!games.empty());
    empty_label->setVisible(games.empty());
    set_error(error_label, "");
}

void ResumeScreen::show_error(const QString& message) { set_error(error_label, message); }

}
