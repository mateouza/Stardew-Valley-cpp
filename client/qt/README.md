# Launcher Qt

Ventana que se abre antes del juego: conexión al servidor, menú principal, nueva partida
(nombre, mundo y personaje), unirse con código de invitación y retomar partidas guardadas.
Cuando el usuario elige una partida, el launcher se cierra y devuelve un `lobby::GameTicket`
para que arranque el cliente SDL.

## Estructura

| Carpeta | Contenido |
| --- | --- |
| `client/lobby/` | Contrato con el servidor, **sin Qt**: tipos (`lobby_types.h`), interfaz `LobbyClient`, `FakeLobbyClient` (simula al servidor en memoria), validación compartida (`lobby_validation`) y `LobbyWorker`, el hilo que ejecuta los pedidos. Biblioteca `client_lobby`. `NetworkLobbyClient`, el lobby real sobre `NetworkClient`, va aparte en `client_lobby_network`. |
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
./build/server 8080                      # en otra terminal; 'q' + Enter lo cierra
./build/stardew_launcher                 # o: ./build/stardew_launcher <host> <puerto>
./build/stardew_launcher --demo          # sin servidor, con el lobby falso
ctest --test-dir build --output-on-failure
```

- `-DBUILD_CLIENT=OFF` evita necesitar SDL2pp. El launcher se compila con `BUILD_LAUNCHER` y el editor de mapas (`stardew_editor`) con `BUILD_EDITOR`; las dos están en `ON` por defecto. Con las dos en `OFF` no se busca Qt.
- Los tests del launcher (`launcher_window_tests` y `character_widgets_tests`) usan Qt en modo `offscreen`, así que no necesitan pantalla.
- Los del lobby (`fake_lobby_client_tests`, `lobby_validation_tests`, `lobby_worker_tests` y `network_lobby_client_tests`) no dependen de Qt y se compilan siempre.
- `make valgrind` revisa todos; los de Qt, solo si existen.

## Lobby real y modo demostración

`stardew_launcher` usa `NetworkLobbyClient`. Crear y unirse son reales: el servidor crea la
partida y devuelve su código, y otro launcher se une con ese código. El resto lo responde el
cliente, porque el protocolo todavía no lo tiene:

- El usuario y la apariencia no viajan al servidor; el nombre que ven los demás es el del personaje.
- El único mundo es el mapa que cargó el servidor, y no hay partidas guardadas para retomar.
- Buscar un código solo revisa que no esté vacío: si existe o está llena se sabe al unirse.
- Quien se une no recibe el nombre de la partida.
- Al entrar, `stardew_launcher` imprime el código y termina: todavía no arranca el juego.

`stardew_launcher --demo` usa `FakeLobbyClient`, que simula al servidor en memoria para probar
las pantallas sin servidor. Lo creado ahí se pierde al cerrar. Datos precargados:

- Cualquier nombre de usuario válido se conecta.
- Código de invitación: **`VALLE1`** ("Granja de Juanceto03", 2 de 4 jugadores).
- `Juanceto01` tiene dos partidas guardadas: una multijugador y una individual. `Juanceto03` tiene una.
- Hay tres mundos: Valle Clásico, Orilla del Río y Colinas Altas.

Los mundos, las opciones de apariencia y el máximo de 4 jugadores son datos de demostración, no
reglas del servidor: se acuerdan con infraestructura y con el modelo.

## Pedidos al lobby sin bloquear la interfaz

`LobbyClient` es síncrono: cada método es un pedido bloqueante con su respuesta. La ventana
nunca lo llama directamente:

1. `LauncherWindow` arma un pedido (`lobby_requests.h`) y lo encola en `LobbyWorker`.
   Deshabilita las pantallas y arranca un `QTimer`.
2. `LobbyWorker`, un `Thread` de la cátedra, saca los pedidos de su `Queue` de a uno y llama al
   `LobbyClient`.
3. Cada 20 ms el `QTimer` hace `try_pop` de la cola de respuestas. Cuando llega una, la ventana
   rehabilita las pantallas y la procesa. Los widgets se tocan solo desde el hilo de Qt.
4. Al destruirse la ventana, el worker cierra su cola, llama a `LobbyClient::interrupt()` y hace
   `join`. No quedan hilos sueltos ni respuestas dirigidas a widgets destruidos.

Una implementación real de `LobbyClient` tiene que:

- No lanzar excepciones: un corte de conexión se devuelve como `ServerUnavailable`.
- Implementar `interrupt()`: se llama desde otro hilo y hace terminar enseguida la operación en
  curso y las siguientes, por ejemplo con `shutdown` del socket.
- Validar sus parámetros aunque la UI ya lo haga.

Falta acordar con infraestructura cuánto puede tardar cada operación y los límites en bytes de
cada campo del protocolo, que hay que controlar antes de reservar memoria.

## Contrato del lobby (provisorio)

Son los métodos que usa la ventana y que implementa el fake. No son mensajes de protocolo: el
formato, los tamaños de los campos y las reglas se acuerdan con quienes hacen el servidor.

| Método | Respuesta si sale bien | Errores (`lobby::LobbyError`) |
| --- | --- | --- |
| `connect(host, puerto, usuario)` | — | `ServerUnavailable`, `InvalidUsername` |
| `list_worlds()` | mundos: id, nombre y descripción | `ServerUnavailable` |
| `list_saved_games()` | partidas del usuario: id, nombre, personaje y si es multijugador | `ServerUnavailable` |
| `create_game(nombre, mundo, personaje)` | `GameTicket` | `InvalidGameName`, `UnknownWorld`, `InvalidCharacterName`, `InvalidAppearance` |
| `find_game(código)` | nombre, creador y jugadores / máximo | `InvalidInviteCode`, `AlreadyJoined`, `GameFull` |
| `join_game(código, personaje)` | `GameTicket` multijugador | los de `find_game`, más los de personaje |
| `resume_game(id)` | `GameTicket` | `UnknownGame` |

Las reglas están en `lobby_validation.h`, sin Qt; las usan el fake y las pantallas, así que
aceptan exactamente lo mismo:

- Usuario: `[A-Za-z0-9_]{1,16}`, porque se usa para los mensajes privados del chat (`@usuario`).
- Puerto: un número entre 1 y 65535, sin signo ni espacios.
- Partida (1 a 24 caracteres) y personaje (1 a 16): letras `A-Z` y `a-z`, `áéíóúüñ` y sus
  mayúsculas, dígitos, espacio, guion y apóstrofo. Un carácter es un punto de código; con esta
  lista coincide con lo que cuenta Qt. Se recortan solo los espacios de los extremos y se
  rechaza el UTF-8 mal formado.
- Índices de apariencia menores a las cantidades de `lobby_types.h`.
- El código de invitación no distingue mayúsculas de minúsculas.

## Decisiones pendientes

**Identidad y recuperación de personajes.** Hoy, cualquiera que se conecte como `Juanceto01` ve
sus partidas: el fake no comprueba nada más. Hay cuatro identificadores distintos:

- el nombre de usuario, con el que se conecta y lo mencionan en el chat;
- el nombre del personaje, el que se ve en el juego;
- el identificador de la conexión, la sesión en el servidor;
- el identificador del jugador dentro de la partida (`game::PlayerId`).

Que el nombre de usuario sea único no alcanza para autorizar que una conexión recupere un
personaje, así que `resume_game()` es provisorio. Pregunta para el corrector: «¿Qué mecanismo
debe acreditar que una conexión puede recuperar un personaje existente? ¿Alcanza con el nombre
de usuario para el alcance del TP?»

**Paso de Qt a SDL2pp.** Acordado con Kika: Qt y SDL corren en el mismo proceso, uno después
del otro. `run_launcher()` destruye la `QApplication` antes de volver. Después,
`NetworkLobbyClient::take_game_connection()` entrega la misma conexión, ya adentro de la
partida, con lo que el lobby leyó: código, `PlayerId` y `GameInfo`. Falta que el launcher se la
pase a `ejecutar_cliente_sdl()`, cuando exista la biblioteca `client_sdl`.

## Qué prueban los tests

- Ventana: navegación entre pantallas y errores, con el fake; teclado, pegado y Enter en los
  campos con reglas; respuesta demorada y cierre con un pedido pendiente. Además, un launcher
  crea una partida en el servidor real y otro se une con su código.
- Fake y validación: las reglas del contrato, incluido el UTF-8 inválido.
- Lobby real (`network_lobby_client_tests`), contra el servidor en el mismo proceso: crear, unirse,
  código inexistente, partida llena, `interrupt()` mientras espera y la conexión que recibe el juego.
- Worker: orden de los pedidos, que no bloquea y que se interrumpe al destruirse.
- Widgets: sprite y selectores.

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
