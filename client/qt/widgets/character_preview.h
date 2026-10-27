#ifndef CLIENT_QT_WIDGETS_CHARACTER_PREVIEW_H_
#define CLIENT_QT_WIDGETS_CHARACTER_PREVIEW_H_

#include <QSize>
#include <QWidget>

#include "lobby/lobby_types.h"

class QPaintEvent;

namespace launcher {

// Muestra el personaje parado sobre un recuadro de pasto, escalado a su tamaño.
class CharacterPreview: public QWidget {
    Q_OBJECT

private:
    lobby::CharacterAppearance appearance;

protected:
    void paintEvent(QPaintEvent* event) override;

public:
    explicit CharacterPreview(QWidget* parent = nullptr);

    void set_appearance(const lobby::CharacterAppearance& new_appearance);
    const lobby::CharacterAppearance& get_appearance() const;

    QSize sizeHint() const override;
};

}

#endif
