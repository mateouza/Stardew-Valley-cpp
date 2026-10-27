#ifndef CLIENT_QT_WIDGETS_SAVED_GAME_CARD_H_
#define CLIENT_QT_WIDGETS_SAVED_GAME_CARD_H_

#include <QPushButton>

#include "lobby/lobby_types.h"

namespace launcher {

// Tarjeta clickeable de una partida guardada: personaje, nombres y si es multijugador. Su
// objectName es "saved_game_<id de la partida>".
class SavedGameCard: public QPushButton {
    Q_OBJECT

public:
    static constexpr int HEIGHT = 108;

    explicit SavedGameCard(const lobby::SavedGameInfo& game, QWidget* parent = nullptr);
};

}

#endif
