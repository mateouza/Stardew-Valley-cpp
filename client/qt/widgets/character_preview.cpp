#include "character_preview.h"

#include <QColor>
#include <QPainter>
#include <QPen>
#include <QRectF>
#include <QRgb>
#include <algorithm>

#include "character_sprite.h"

namespace launcher {

namespace {

constexpr QRgb STAGE = qRgb(0xcf, 0xe8, 0xa9);
constexpr QRgb STAGE_BORDER = qRgb(0xb0, 0x7a, 0x46);
constexpr int SPRITE_IMAGE_HEIGHT = 26;

}

CharacterPreview::CharacterPreview(QWidget* parent): QWidget(parent) {}

void CharacterPreview::set_appearance(const lobby::CharacterAppearance& new_appearance) {
    appearance = new_appearance;
    update();
}

const lobby::CharacterAppearance& CharacterPreview::get_appearance() const { return appearance; }

QSize CharacterPreview::sizeHint() const { return QSize(180, 252); }

void CharacterPreview::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const qreal radius = std::min(width(), height()) / 10.0;
    painter.setPen(QPen(QColor(STAGE_BORDER), 3));
    painter.setBrush(QColor(STAGE));
    painter.drawRoundedRect(QRectF(rect()).adjusted(1.5, 1.5, -1.5, -1.5), radius, radius);

    const int margin = std::max(4, width() / 14);
    const QRect target = fit_character(rect().adjusted(margin, margin, -margin, -margin));
    const qreal scale = static_cast<qreal>(target.height()) / SPRITE_IMAGE_HEIGHT;

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 45));
    painter.drawEllipse(QRectF(target.left() + target.width() * 0.18, target.top() + scale * 22.5,
                               target.width() * 0.64, scale * 2.6));

    painter.setRenderHint(QPainter::Antialiasing, false);
    paint_character(painter, target, appearance);
}

}
