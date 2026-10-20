# Launcher Qt

Ventana que se abre antes del juego: conexión al servidor, menú principal, nueva partida
(nombre, mundo y personaje), unirse con código de invitación y retomar partidas guardadas.
Cuando el usuario elige una partida, el launcher se cierra y devuelve un `lobby::GameTicket`
para que arranque el cliente SDL.

## Estructura

| Carpeta | Contenido |
|---|---|
| `client/lobby/` | Contrato con el servidor, **sin Qt**: tipos (`lobby_types.h`), interfaz `LobbyClient` y `FakeLobbyClient`, que simula al servidor en memoria. Biblioteca `client_lobby`. |
| `client/qt/` | `run_launcher()` (`launcher.h`), la ventana controladora (`launcher_window`), la hoja de estilo y los mensajes de error. Biblioteca `qt_launcher`. |
| `client/qt/screens/` | Una clase por pantalla. Solo emiten señales; la ventana decide a qué pantalla ir y qué pedirle al lobby. |
| `client/qt/widgets/` | Fondo, sprite del personaje, selectores, tarjetas y helpers de UI. |

## Dependencias

Además de las del README principal:

```bash
sudo apt install qt6-base-dev libgl1-mesa-dev
```

## Compilar, ejecutar y testear

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DBUILD_CLIENT=OFF
cmake --build build -j4
./build/stardew_launcher                 # o: ./build/stardew_launcher <host> <puerto>
ctest --test-dir build --output-on-failure
```

- `-DBUILD_CLIENT=OFF` evita necesitar SDL2pp. El launcher se compila con `BUILD_LAUNCHER`, que está en `ON` por defecto.
- Los tests del launcher (`launcher_window_tests` y `character_widgets_tests`) usan Qt en modo `offscreen`, así que no necesitan pantalla.
- Los del lobby (`fake_lobby_client_tests`) no dependen de Qt.

## Datos de prueba

Mientras el protocolo no tenga mensajes de lobby, `stardew_launcher` usa `FakeLobbyClient`:

- Cualquier nombre de usuario válido se conecta (letras, números y `_`, hasta 16).
- Código de invitación: **`VALLE1`** ("Granja de Pandamos", 2 de 4 jugadores).
- `dipa` tiene dos partidas guardadas: una multijugador y una individual. `Pandamos` tiene una.
- Hay tres mundos: Valle Clásico, Orilla del Río y Colinas Altas.

Lo creado en una ejecución no se guarda: se pierde al cerrar.

## Integración con el cliente SDL

La `QApplication` vive solo dentro de `run_launcher()`, así que al volver se puede inicializar
SDL en el mismo proceso. En `client/sdl/main.cpp`:

```cpp
#include "lobby/fake_lobby_client.h"
#include "qt/launcher.h"

int main(int argc, char* argv[]) {
    lobby::FakeLobbyClient lobby_client;  // reemplazar por el cliente real cuando exista
    const std::optional<lobby::GameTicket> ticket =
            launcher::run_launcher(argc, argv, lobby_client, launcher::LauncherOptions{});
    if (!ticket) {
        return EXIT_SUCCESS;  // cerró el launcher sin elegir partida
    }

    // Recién acá se inicializa SDL. El ticket trae game_id, game_name, multiplayer y el
    // personaje (nombre e índices de apariencia definidos en lobby/lobby_types.h).
}
```

Y en `CMakeLists.txt`, agregar `qt_launcher` a `target_link_libraries(stardew_client ...)`.
El target existe con `BUILD_LAUNCHER` en `ON`, que es el valor por defecto.

## Implementación real del lobby

Hay que escribir una clase que herede de `lobby::LobbyClient` y hable con el servidor usando
el protocolo. Las pantallas no cambian. Cada método es una petición y su respuesta; las
llamadas son bloqueantes y la ventana muestra el cursor de espera mientras tanto.

| Método | Respuesta si sale bien | Errores (`lobby::LobbyError`) |
|---|---|---|
| `connect(host, puerto, usuario)` | — | `ServerUnavailable`, `InvalidUsername` |
| `list_worlds()` | mundos: id, nombre y descripción | `ServerUnavailable` |
| `list_saved_games()` | partidas del usuario: id, nombre, personaje y si es multijugador | `ServerUnavailable` |
| `create_game(nombre, mundo, personaje)` | `GameTicket` | `InvalidGameName`, `UnknownWorld`, `InvalidCharacterName`, `InvalidAppearance` |
| `find_game(código)` | nombre, creador y jugadores / máximo | `InvalidInviteCode`, `AlreadyJoined`, `GameFull` |
| `join_game(código, personaje)` | `GameTicket` multijugador | los de `find_game`, más los de personaje |
| `resume_game(id)` | `GameTicket` | `UnknownGame` |

El servidor valida. La UI solo limita largos, filtra caracteres y deshabilita botones:

- Usuario: `[A-Za-z0-9_]{1,16}`, porque se usa para los mensajes privados del chat (`@usuario`).
- Partida: de 1 a 24 caracteres. Personaje: de 1 a 16. Se cuentan caracteres, no bytes, y se recortan los espacios de los extremos.
- Índices de apariencia menores a las cantidades de `lobby_types.h`.
- El código de invitación no distingue mayúsculas de minúsculas.

`FakeLobbyClient` implementa exactamente estas reglas y sus tests sirven de especificación.
Después del lobby, la misma conexión se usa para jugar, así que la clase real tiene que
compartir el socket con el cliente.

Para charlar con el corrector: el enunciado no dice cómo se identifica a un jugador entre
sesiones. Acá se asume que el nombre de usuario es único en el servidor.

## Convenciones de la UI

- El estilo se elige con propiedades que lee la hoja de estilo (`launcher_style.cpp`):
  `role` para etiquetas y paneles, `variant` para botones. El `objectName` se usa para
  identificar widgets en los tests.
- Los textos que vienen del servidor se muestran como texto plano (`Qt::PlainText`).
- Para centrar paneles se usa `add_centered()`, no `Qt::AlignHCenter`: con alineación, Qt
  calcula mal la altura de las etiquetas con salto de línea y el panel queda cortado.
- El sprite del personaje se dibuja en `widgets/character_sprite.cpp` a partir de grillas de
  texto. Cuando lleguen los assets de la cátedra, se reemplaza esa función.
- Pasan clang-format y cppcheck con la configuración del pre-commit del grupo.
