#ifndef CLIENT_QT_WIDGETS_CHARACTER_SPRITE_H_
#define CLIENT_QT_WIDGETS_CHARACTER_SPRITE_H_

#include <QImage>
#include <QRect>

#include "lobby/lobby_types.h"

class QPainter;

namespace launcher {

QImage render_character(const lobby::CharacterAppearance& appearance);

QRect fit_character(const QRect& area);

void paint_character(QPainter& painter, const QRect& target,
                     const lobby::CharacterAppearance& appearance);

}

#endif
