#include "character_sprite.h"

#include <QColor>
#include <QPainter>
#include <QRgb>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "character_options.h"

namespace launcher {

namespace {

constexpr std::size_t SPRITE_WIDTH = 16;
constexpr std::size_t SPRITE_HEIGHT = 24;
constexpr int OUTLINE = 1;
constexpr int IMAGE_WIDTH = static_cast<int>(SPRITE_WIDTH) + 2 * OUTLINE;
constexpr int IMAGE_HEIGHT = static_cast<int>(SPRITE_HEIGHT) + 2 * OUTLINE;

using Sprite = std::array<std::string_view, SPRITE_HEIGHT>;

// S piel, s piel en sombra, E ojos, B mejillas, M boca, T remera, t remera en sombra,
// K cinturón, P pantalón, p pantalón en sombra, O zapatos, H pelo, h pelo en sombra.
// Las filas que faltan al final de un sprite quedan transparentes.
// clang-format off
constexpr Sprite BODY = {
        "................",
        "................",
        "................",
        "....SSSSSSSS....",
        "....SSSSSSSS....",
        "....SSSSSSSS....",
        "...SSSESSESSS...",
        "...SSSESSESSS...",
        "....SBSSSSBS....",
        "....SSSMMSSS....",
        ".....SSSSSS.....",
        "......ssss......",
        "...TTTTTTTTTT...",
        "...TTTTTTTTTT...",
        "...tTTTTTTTTt...",
        "...tTTTTTTTTt...",
        "...StTTTTTTtS...",
        "...SKKKKKKKKS...",
        "....PPPPPPPP....",
        "....PPPPPPPP....",
        "....PPP..PPP....",
        "....pPP..PPp....",
        "...OOOO..OOOO...",
        "................",
};

constexpr std::array<Sprite, lobby::HAIR_STYLE_COUNT> HAIR_STYLE_SPRITES{{
        {
                // Corto
                "................",
                "................",
                ".....HHHHHH.....",
                "....HHHHHHHH....",
                "....HHHHHHHH....",
                "....HH.H..HH....",
                "....h......h....",
        },
        {
                // Largo
                "................",
                "................",
                ".....HHHHHH.....",
                "....HHHHHHHH....",
                "...HHHHHHHHHH...",
                "...HHH.HH.HHH...",
                "...HH......HH...",
                "...HH......HH...",
                "...HH......HH...",
                "...Hh......hH...",
                "...Hh......hH...",
                "...hh......hh...",
                "...hh......hh...",
        },
        {
                // Puntas
                "................",
                "....H..HH..H....",
                "....HHHHHHHH....",
                "...HHHHHHHHHH...",
                "....HHHHHHHH....",
                "....HH.HH.HH....",
                "....h......h....",
        },
        {
                // Rodete
                "......hHHh......",
                "......HHHH......",
                ".....HHHHHH.....",
                "....HHHHHHHH....",
                "....HHHHHHHH....",
                "....HH....HH....",
                "....h......h....",
        },
}};
// clang-format on

constexpr bool has_valid_rows(const Sprite& sprite) {
    return std::all_of(sprite.begin(), sprite.end(), [](std::string_view row) {
        return row.empty() || row.size() == SPRITE_WIDTH;
    });
}

static_assert(has_valid_rows(BODY), "Cada fila del cuerpo debe tener 16 celdas");
static_assert(std::all_of(HAIR_STYLE_SPRITES.begin(), HAIR_STYLE_SPRITES.end(), has_valid_rows),
              "Cada fila de los peinados debe tener 16 celdas o estar vacía");

constexpr QRgb CLEAR = qRgba(0, 0, 0, 0);
constexpr QRgb EYES = qRgb(40, 28, 20);
constexpr QRgb BELT = qRgb(92, 60, 30);
constexpr QRgb SHOES = qRgb(64, 42, 26);
constexpr QRgb OUTLINE_COLOR = qRgb(48, 30, 20);
constexpr QRgb BLUSH_TINT = qRgb(232, 120, 120);

struct SpriteColors {
    QRgb skin;
    QRgb hair;
    QRgb shirt;
    QRgb pants;
};

template <std::size_t N>
QRgb pick(const std::array<ColorOption, N>& options, std::uint8_t index) {
    return options[index % N].rgb;
}

QRgb darker(QRgb color, int factor) { return QColor(color).darker(factor).rgb(); }

QRgb mix(QRgb from, QRgb to, double amount) {
    auto channel = [amount](int start, int end) {
        return static_cast<int>(start + (end - start) * amount);
    };
    return qRgb(channel(qRed(from), qRed(to)), channel(qGreen(from), qGreen(to)),
                channel(qBlue(from), qBlue(to)));
}

QRgb color_for(char code, const SpriteColors& colors) {
    switch (code) {
        case 'S':
            return colors.skin;
        case 's':
            return darker(colors.skin, 125);
        case 'B':
            return mix(colors.skin, BLUSH_TINT, 0.35);
        case 'M':
            return darker(colors.skin, 145);
        case 'E':
            return EYES;
        case 'H':
            return colors.hair;
        case 'h':
            return darker(colors.hair, 125);
        case 'T':
            return colors.shirt;
        case 't':
            return darker(colors.shirt, 120);
        case 'K':
            return BELT;
        case 'P':
            return colors.pants;
        case 'p':
            return darker(colors.pants, 125);
        case 'O':
            return SHOES;
        default:
            return CLEAR;
    }
}

void stamp(QImage& image, const Sprite& sprite, const SpriteColors& colors) {
    for (std::size_t y = 0; y < sprite.size(); ++y) {
        for (std::size_t x = 0; x < sprite[y].size(); ++x) {
            if (sprite[y][x] != '.') {
                image.setPixel(static_cast<int>(x) + OUTLINE, static_cast<int>(y) + OUTLINE,
                               color_for(sprite[y][x], colors));
            }
        }
    }
}

bool is_opaque(const QImage& image, int x, int y) {
    return image.rect().contains(x, y) && qAlpha(image.pixel(x, y)) > 0;
}

QImage add_outline(const QImage& sprite) {
    QImage outlined = sprite;
    for (int y = 0; y < sprite.height(); ++y) {
        for (int x = 0; x < sprite.width(); ++x) {
            const bool touches_sprite = is_opaque(sprite, x - 1, y) ||
                                        is_opaque(sprite, x + 1, y) ||
                                        is_opaque(sprite, x, y - 1) || is_opaque(sprite, x, y + 1);
            if (!is_opaque(sprite, x, y) && touches_sprite) {
                outlined.setPixel(x, y, OUTLINE_COLOR);
            }
        }
    }
    return outlined;
}

}

QImage render_character(const lobby::CharacterAppearance& appearance) {
    const SpriteColors colors{
            pick(SKIN_TONES, appearance.skin_tone), pick(HAIR_COLORS, appearance.hair_color),
            pick(SHIRT_COLORS, appearance.shirt_color), pick(PANTS_COLORS, appearance.pants_color)};

    QImage image(IMAGE_WIDTH, IMAGE_HEIGHT, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    stamp(image, BODY, colors);
    stamp(image, HAIR_STYLE_SPRITES[appearance.hair_style % HAIR_STYLE_SPRITES.size()], colors);
    return add_outline(image);
}

QRect fit_character(const QRect& area) {
    const int scale =
            std::max(1, std::min(area.width() / IMAGE_WIDTH, area.height() / IMAGE_HEIGHT));
    const int width = IMAGE_WIDTH * scale;
    const int height = IMAGE_HEIGHT * scale;
    return QRect(area.x() + (area.width() - width) / 2, area.y() + (area.height() - height) / 2,
                 width, height);
}

void paint_character(QPainter& painter, const QRect& target,
                     const lobby::CharacterAppearance& appearance) {
    painter.save();
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.drawImage(target, render_character(appearance));
    painter.restore();
}

}
