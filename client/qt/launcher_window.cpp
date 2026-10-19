#include "launcher_window.h"

#include <QApplication>
#include <QPainter>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <initializer_list>
#include <utility>
#include <variant>
#include <vector>

#include "qt/screens/character_screen.h"
#include "qt/screens/connect_screen.h"
#include "qt/screens/main_menu_screen.h"
#include "qt/screens/new_game_screen.h"
#include "qt/widgets/landscape.h"

#include "lobby_error_message.h"

namespace launcher {

namespace {

// Las llamadas al lobby son bloqueantes: mientras duran, el cursor indica que hay que esperar.
class BusyCursor {
public:
    BusyCursor() { QApplication::setOverrideCursor(Qt::WaitCursor); }
    ~BusyCursor() { QApplication::restoreOverrideCursor(); }

    BusyCursor(const BusyCursor&) = delete;
    BusyCursor& operator=(const BusyCursor&) = delete;
};

}

LauncherWindow::LauncherWindow(lobby::LobbyClient& lobby_client, const LauncherOptions& options,
                               QWidget* parent):
        QWidget(parent),
        lobby_client(lobby_client),
        screens(new QStackedWidget(this)),
        connect_screen(new ConnectScreen(options, screens)),
        main_menu_screen(new MainMenuScreen(screens)),
        new_game_screen(new NewGameScreen(screens)),
        character_screen(new CharacterScreen(screens)) {
    setWindowTitle("Stardew Valley");
    resize(1024, 720);
    setMinimumSize(900, 660);

    for (QWidget* screen: std::initializer_list<QWidget*>{connect_screen, main_menu_screen,
                                                          new_game_screen, character_screen}) {
        screens->addWidget(screen);
    }

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(screens);

    auto back_to_menu = [this] { show_screen(main_menu_screen); };

    connect(connect_screen, &ConnectScreen::connect_requested, this,
            &LauncherWindow::connect_to_server);
    connect(connect_screen, &ConnectScreen::quit_requested, this, &QWidget::close);
    connect(main_menu_screen, &MainMenuScreen::new_game_requested, this,
            &LauncherWindow::open_new_game);
    connect(main_menu_screen, &MainMenuScreen::quit_requested, this, &QWidget::close);
    connect(new_game_screen, &NewGameScreen::back_requested, this, back_to_menu);
    connect(new_game_screen, &NewGameScreen::continue_requested, this,
            &LauncherWindow::choose_world);
    connect(character_screen, &CharacterScreen::back_requested, this,
            [this] { show_screen(new_game_screen); });
    connect(character_screen, &CharacterScreen::confirm_requested, this,
            &LauncherWindow::confirm_character);

    show_screen(connect_screen);
}

void LauncherWindow::show_screen(QWidget* screen) {
    screens->setCurrentWidget(screen);
    screen->setFocus();
}

void LauncherWindow::connect_to_server(const QString& requested_username, const QString& host,
                                       const QString& port) {
    const BusyCursor busy;
    const auto error = lobby_client.connect(host.toStdString(), port.toStdString(),
                                            requested_username.toStdString());
    if (error) {
        connect_screen->show_error(lobby_error_message(*error));
        return;
    }

    username = requested_username;
    main_menu_screen->set_session(username, host + ":" + port);
    show_screen(main_menu_screen);
}

void LauncherWindow::open_new_game() {
    const BusyCursor busy;
    const auto worlds = lobby_client.list_worlds();
    if (const auto* error = std::get_if<lobby::LobbyError>(&worlds)) {
        main_menu_screen->show_error(lobby_error_message(*error));
        return;
    }

    main_menu_screen->show_error("");
    new_game_screen->reset(std::get<std::vector<lobby::WorldInfo>>(worlds));
    character_screen->reset_character(username);
    show_screen(new_game_screen);
}

void LauncherWindow::choose_world(const QString& game_name, const QString& world_id,
                                  const QString& world_name) {
    pending_game_name = game_name;
    pending_world_id = world_id;
    character_screen->prepare(QString("Partida «%1» · Mundo %2").arg(game_name, world_name),
                              "Crear partida");
    show_screen(character_screen);
}

void LauncherWindow::confirm_character(const lobby::CharacterDraft& character) {
    const BusyCursor busy;
    auto result = lobby_client.create_game(pending_game_name.toStdString(),
                                           pending_world_id.toStdString(), character);
    if (const auto* error = std::get_if<lobby::LobbyError>(&result)) {
        character_screen->show_error(lobby_error_message(*error));
        return;
    }

    finish(std::get<lobby::GameTicket>(std::move(result)));
}

void LauncherWindow::finish(lobby::GameTicket game_ticket) {
    ticket = std::move(game_ticket);
    close();
}

void LauncherWindow::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    paint_landscape(painter, rect());
}

const std::optional<lobby::GameTicket>& LauncherWindow::get_ticket() const { return ticket; }

}
