#include "landscape.h"

#include <QColor>
#include <QLinearGradient>
#include <QPainter>
#include <QPoint>
#include <QRect>
#include <QRgb>
#include <algorithm>
#include <array>

namespace launcher {

namespace {

constexpr QRgb SKY_TOP = qRgb(0x6f, 0xb9, 0xe8);
constexpr QRgb SKY_BOTTOM = qRgb(0xd6, 0xf1, 0xfb);
constexpr QRgb CLOUD = qRgb(0xff, 0xff, 0xff);
constexpr QRgb CLOUD_SHADE = qRgb(0xdc, 0xec, 0xf7);
constexpr QRgb FAR_HILLS = qRgb(0x5f, 0x9a, 0x47);
constexpr QRgb NEAR_HILLS = qRgb(0x76, 0xb3, 0x54);
constexpr QRgb GRASS = qRgb(0x8c, 0xc6, 0x5f);
constexpr QRgb GRASS_STRIPE = qRgb(0x82, 0xbc, 0x57);
constexpr QRgb FENCE = qRgb(0xa8, 0x74, 0x42);
constexpr QRgb FENCE_SHADE = qRgb(0x6e, 0x47, 0x26);
constexpr std::array<QRgb, 3> FLOWERS = {qRgb(0xf7, 0xd8, 0x4a), qRgb(0xff, 0xff, 0xff),
                                         qRgb(0xf2, 0x8c, 0xb0)};

void paint_cloud(QPainter& painter, int x, int y, int unit) {
    painter.setBrush(QColor(CLOUD));
    painter.drawRect(x + 3 * unit, y, 4 * unit, unit);
    painter.drawRect(x + unit, y + unit, 8 * unit, unit);
    painter.drawRect(x, y + 2 * unit, 10 * unit, unit);
    painter.setBrush(QColor(CLOUD_SHADE));
    painter.drawRect(x + unit, y + 3 * unit, 8 * unit, unit);
}

void paint_hills(QPainter& painter, const QRect& area, int center_y, int offset, QRgb color) {
    const int radius_x = area.width() / 5;
    const int radius_y = area.height() / 8;
    painter.setBrush(QColor(color));
    for (int i = 0; i <= 4; ++i) {
        painter.drawEllipse(QPoint(area.left() + offset + i * area.width() / 4, center_y), radius_x,
                            radius_y);
    }
}

void paint_fence(QPainter& painter, const QRect& area, int top) {
    painter.setBrush(QColor(FENCE));
    painter.drawRect(area.left(), top + 8, area.width(), 5);
    painter.drawRect(area.left(), top + 20, area.width(), 5);

    for (int x = area.left() + 14; x < area.right(); x += 56) {
        painter.setBrush(QColor(FENCE));
        painter.drawRect(x, top, 10, 34);
        painter.setBrush(QColor(FENCE_SHADE));
        painter.drawRect(x + 7, top, 3, 34);
    }
}

void paint_flowers(QPainter& painter, const QRect& meadow) {
    // Posiciones fijas (pseudoaleatorias) para que no cambien entre repintados.
    for (int i = 0; i < 60; ++i) {
        const int x = meadow.left() + (i * 7919) % meadow.width();
        const int y = meadow.top() + 12 + (i * 104729) % std::max(1, meadow.height() - 20);
        painter.setBrush(QColor(FLOWERS[i % FLOWERS.size()]));
        painter.drawRect(x, y, 4, 4);
    }
}

}

void paint_landscape(QPainter& painter, const QRect& area) {
    painter.save();
    painter.setPen(Qt::NoPen);

    const int horizon = area.top() + area.height() * 3 / 4;

    QLinearGradient sky(area.topLeft(), QPoint(area.left(), horizon));
    sky.setColorAt(0, QColor(SKY_TOP));
    sky.setColorAt(1, QColor(SKY_BOTTOM));
    painter.fillRect(area, sky);

    paint_cloud(painter, area.left() + area.width() / 12, area.top() + area.height() / 10, 9);
    paint_cloud(painter, area.left() + area.width() * 7 / 10, area.top() + area.height() / 16, 11);
    paint_cloud(painter, area.left() + area.width() * 17 / 20, area.top() + area.height() * 3 / 10,
                6);
    paint_cloud(painter, area.left() + area.width() / 40, area.top() + area.height() / 3, 5);

    paint_hills(painter, area, horizon - area.height() / 14, 0, FAR_HILLS);
    paint_hills(painter, area, horizon, area.width() / 8, NEAR_HILLS);

    const QRect meadow(area.left(), horizon, area.width(), area.bottom() - horizon + 1);
    painter.fillRect(meadow, QColor(GRASS));
    for (int y = meadow.top() + 14; y < meadow.bottom(); y += 26) {
        painter.fillRect(meadow.left(), y, meadow.width(), 8, QColor(GRASS_STRIPE));
    }
    paint_flowers(painter, meadow);
    paint_fence(painter, area, horizon - 26);

    painter.restore();
}

}
