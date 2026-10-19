#ifndef CLIENT_QT_WIDGETS_CHARACTER_OPTIONS_H_
#define CLIENT_QT_WIDGETS_CHARACTER_OPTIONS_H_

#include <QRgb>
#include <array>

#include "lobby/lobby_types.h"

namespace launcher {

struct ColorOption {
    const char* name;
    QRgb rgb;
};

inline constexpr std::array<ColorOption, lobby::SKIN_TONE_COUNT> SKIN_TONES{{
        {"Muy clara", qRgb(255, 224, 196)},
        {"Clara", qRgb(240, 196, 156)},
        {"Trigueña", qRgb(212, 158, 112)},
        {"Morena", qRgb(166, 112, 74)},
        {"Oscura", qRgb(112, 72, 48)},
}};

inline constexpr std::array<const char*, lobby::HAIR_STYLE_COUNT> HAIR_STYLES{"Corto", "Largo",
                                                                              "Puntas", "Rodete"};

inline constexpr std::array<ColorOption, lobby::HAIR_COLOR_COUNT> HAIR_COLORS{{
        {"Negro", qRgb(46, 34, 34)},
        {"Castaño", qRgb(112, 68, 36)},
        {"Rubio", qRgb(232, 190, 92)},
        {"Pelirrojo", qRgb(192, 82, 40)},
        {"Gris", qRgb(172, 172, 178)},
        {"Violeta", qRgb(142, 92, 190)},
}};

inline constexpr std::array<ColorOption, lobby::SHIRT_COLOR_COUNT> SHIRT_COLORS{{
        {"Verde", qRgb(88, 160, 72)},
        {"Azul", qRgb(64, 112, 200)},
        {"Rojo", qRgb(200, 64, 56)},
        {"Amarillo", qRgb(230, 190, 60)},
        {"Violeta", qRgb(140, 90, 180)},
        {"Turquesa", qRgb(60, 170, 170)},
        {"Blanco", qRgb(236, 236, 226)},
        {"Naranja", qRgb(230, 130, 50)},
}};

inline constexpr std::array<ColorOption, lobby::PANTS_COLOR_COUNT> PANTS_COLORS{{
        {"Jean", qRgb(62, 82, 142)},
        {"Marrón", qRgb(112, 76, 46)},
        {"Negro", qRgb(46, 46, 56)},
        {"Oliva", qRgb(102, 112, 62)},
        {"Gris", qRgb(122, 122, 132)},
        {"Beige", qRgb(202, 182, 140)},
}};

}

#endif
