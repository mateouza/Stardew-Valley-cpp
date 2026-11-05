#ifndef STAT_BARS_H
#define STAT_BARS_H

#include "../render/renderer.h"

//PlayerState todavia no tiene los campos asi que se dibuja un placeholder fijo
class StatBars {
    public:
        void dibujar(Renderer& renderer) const;
};

#endif
