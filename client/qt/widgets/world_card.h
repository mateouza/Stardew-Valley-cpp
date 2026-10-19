#ifndef CLIENT_QT_WIDGETS_WORLD_CARD_H_
#define CLIENT_QT_WIDGETS_WORLD_CARD_H_

#include <QPushButton>

#include "lobby/lobby_types.h"

namespace launcher {

// Tarjeta seleccionable con el nombre y la descripción de un mundo. Su objectName es
// "world_<id del mundo>".
class WorldCard: public QPushButton {
    Q_OBJECT

public:
    explicit WorldCard(const lobby::WorldInfo& world, QWidget* parent = nullptr);
};

}

#endif
