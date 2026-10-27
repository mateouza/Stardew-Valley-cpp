#ifndef CLIENT_QT_LAUNCHER_STYLE_H_
#define CLIENT_QT_LAUNCHER_STYLE_H_

#include <QString>

namespace launcher {

// Los widgets eligen su estilo con las propiedades "role" (etiquetas y paneles) y "variant"
// (botones); el objectName queda libre para identificarlos.
QString launcher_stylesheet();

}

#endif
