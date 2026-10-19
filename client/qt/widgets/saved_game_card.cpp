#include "saved_game_card.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QString>
#include <QVBoxLayout>

#include "character_preview.h"
#include "ui_helpers.h"

namespace launcher {

SavedGameCard::SavedGameCard(const lobby::SavedGameInfo& game, QWidget* parent):
        QPushButton(parent) {
    setObjectName(QString::fromStdString("saved_game_" + game.game_id));
    setProperty("variant", "card");
    setCursor(Qt::PointingHandCursor);
    setFixedHeight(HEIGHT);

    auto* preview = new CharacterPreview(this);
    preview->setFixedSize(68, 92);
    preview->set_appearance(game.character.appearance);
    preview->setAttribute(Qt::WA_TransparentForMouseEvents);

    QLabel* game_name = create_label(QString::fromStdString(game.game_name), "card_title", this);
    QLabel* character_name = create_label(
            "Personaje: " + QString::fromStdString(game.character.name), "card_detail", this);
    QLabel* badge = create_label(game.multiplayer ? "Multijugador" : "Un jugador",
                                 game.multiplayer ? "badge_multiplayer" : "badge_single", this);
    badge->setObjectName("badge");
    QLabel* action = create_label("Continuar ▶", "card_action", this);

    auto* details = new QVBoxLayout();
    details->setSpacing(4);
    details->addStretch();
    details->addWidget(game_name);
    details->addWidget(character_name);
    details->addWidget(badge, 0, Qt::AlignLeft);
    details->addStretch();

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 8, 18, 8);
    layout->setSpacing(14);
    layout->addWidget(preview);
    layout->addLayout(details, 1);
    layout->addWidget(action);

    for (QLabel* label: {game_name, character_name, badge, action}) {
        label->setAttribute(Qt::WA_TransparentForMouseEvents);
    }
}

}
