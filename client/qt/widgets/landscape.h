#ifndef CLIENT_QT_WIDGETS_LANDSCAPE_H_
#define CLIENT_QT_WIDGETS_LANDSCAPE_H_

class QPainter;
class QRect;

namespace launcher {

// Fondo del launcher: cielo, nubes, colinas, pasto y una cerca, en bloques sin antialiasing.
void paint_landscape(QPainter& painter, const QRect& area);

}

#endif
