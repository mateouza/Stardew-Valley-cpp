#include "launcher_window.h"

#include <QPainter>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <chrono>
#include <initializer_list>
#include <utility>
#include <variant>
#include <vector>

#include "qt/screens/character_screen.h"
#include "qt/screens/connect_screen.h"
#include "qt/screens/join_screen.h"
#include "qt/screens/main_menu_screen.h"
#include "qt/screens/new_game_screen.h"
#include "qt/screens/resume_screen.h"
#include "qt/widgets/landscape.h"

#include "lobby_error_message.h"

namespace launcher {

namespace {

// Cada cuánto se consulta, mientras se espera, si llegó la respuesta del lobby.
constexpr std::chrono::milliseconds RESPONSE_POLL_INTERVAL{20};

// Si el lobby devolvió un error, lo muestra en la pantalla que hizo el pedido.
template <typename Screen>
std::optional<lobby::GameTicket> ticket_or_show_error(
        const lobby::LobbyResult<lobby::GameTicket>& result, Screen* screen) {
    if (const auto* error = std::get_if<lobby::LobbyError>(&result)) {
        screen->show_error(lobby_error_message(*error));
        return std::nullopt;
    }
    return std::get<lobby::GameTicket>(result);
}

}

LauncherWindow::LauncherWindow(lobby::LobbyClient& lobby_client, const LauncherOptions& options,
                               QWidget* parent):
        QWidget(parent),
        lobby_worker(lobby_client),
        screens(new QStackedWidget(this)),
        connect_screen(new ConnectScreen(options, screens)),
        main_menu_screen(new MainMenuScreen(screens)),
        new_game_screen(new NewGameScreen(screens)),
        join_screen(new JoinScreen(screens)),
        character_screen(new CharacterScreen(screens)),
        resume_screen(new ResumeScreen(screens)),
        response_timer(new QTimer(this)) {
    setWindowTitle("Stardew Valley");
    resize(1024, 720);
    setMinimumSize(900, 660);

    for (QWidget* screen:
         std::initializer_list<QWidget*>{connect_screen, main_menu_screen, new_game_screen,
                                         join_screen, character_screen, resume_screen}) {
        screens->addWidget(screen);
    }

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(screens);

    response_timer->setInterval(RESPONSE_POLL_INTERVAL);
    connect(response_timer, &QTimer::timeout, this, &LauncherWindow::poll_lobby);

    auto back_to_menu = [this] { show_screen(main_menu_screen); };

    connect(connect_screen, &ConnectScreen::connect_requested, this,
            &LauncherWindow::connect_to_server);
    connect(connect_screen, &ConnectScreen::quit_requested, this, &QWidget::close);
    connect(main_menu_screen, &MainMenuScreen::new_game_requested, this,
            &LauncherWindow::open_new_game);
    connect(main_menu_screen, &MainMenuScreen::join_requested, this, &LauncherWindow::open_join);
    connect(main_menu_screen, &MainMenuScreen::resume_requested, this,
            &LauncherWindow::open_resume);
    connect(main_menu_screen, &MainMenuScreen::quit_requested, this, &QWidget::close);
    connect(new_game_screen, &NewGameScreen::back_requested, this, back_to_menu);
    connect(new_game_screen, &NewGameScreen::continue_requested, this,
            &LauncherWindow::choose_world);
    connect(join_screen, &JoinScreen::back_requested, this, back_to_menu);
    connect(join_screen, &JoinScreen::invite_code_submitted, this,
            &LauncherWindow::look_up_invite_code);
    connect(resume_screen, &ResumeScreen::back_requested, this, back_to_menu);
    connect(resume_screen, &ResumeScreen::game_selected, this, &LauncherWindow::resume_game);
    connect(character_screen, &CharacterScreen::back_requested, this,
            &LauncherWindow::leave_character_screen);
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
    username = requested_username;
    server_address = host + ":" + port;
    send_to_lobby(lobby::ConnectRequest{host.toStdString(), port.toStdString(),
                                        requested_username.toStdString()});
}

void LauncherWindow::handle(const lobby::ConnectResponse& response) {
    if (response.error) {
        connect_screen->show_error(lobby_error_message(*response.error));
        return;
    }

    main_menu_screen->set_session(username, server_address);
    show_screen(main_menu_screen);
}

void LauncherWindow::open_new_game() { send_to_lobby(lobby::ListWorldsRequest{}); }

void LauncherWindow::handle(const lobby::ListWorldsResponse& response) {
    if (const auto* error = std::get_if<lobby::LobbyError>(&response.result)) {
        main_menu_screen->show_error(lobby_error_message(*error));
        return;
    }

    main_menu_screen->show_error("");
    new_game_screen->reset(std::get<std::vector<lobby::WorldInfo>>(response.result));
    character_screen->reset_character(username);
    show_screen(new_game_screen);
}

void LauncherWindow::choose_world(const QString& game_name, const QString& world_id,
                                  const QString& world_name) {
    character_purpose = CharacterPurpose::NewGame;
    pending_game_name = game_name;
    pending_world_id = world_id;
    character_screen->prepare(QString("Partida «%1» · Mundo %2").arg(game_name, world_name),
                              "Crear partida");
    show_screen(character_screen);
}

void LauncherWindow::open_join() {
    main_menu_screen->show_error("");
    join_screen->reset();
    character_screen->reset_character(username);
    show_screen(join_screen);
}

void LauncherWindow::look_up_invite_code(const QString& invite_code) {
    pending_invite_code = invite_code;
    send_to_lobby(lobby::FindGameRequest{invite_code.toStdString()});
}

void LauncherWindow::handle(const lobby::FindGameResponse& response) {
    if (const auto* error = std::get_if<lobby::LobbyError>(&response.result)) {
        join_screen->show_error(lobby_error_message(*error));
        return;
    }

    const auto& game = std::get<lobby::JoinableGameInfo>(response.result);
    character_purpose = CharacterPurpose::JoinGame;
    character_screen->prepare(
            QString("Te unís a «%1», de %2 · %3/%4 jugadores")
                    .arg(QString::fromStdString(game.game_name), QString::fromStdString(game.owner),
                         QString::number(game.player_count), QString::number(game.max_players)),
            "Unirme a la partida");
    show_screen(character_screen);
}

void LauncherWindow::open_resume() { send_to_lobby(lobby::ListSavedGamesRequest{}); }

void LauncherWindow::handle(const lobby::ListSavedGamesResponse& response) {
    if (const auto* error = std::get_if<lobby::LobbyError>(&response.result)) {
        main_menu_screen->show_error(lobby_error_message(*error));
        return;
    }

    main_menu_screen->show_error("");
    resume_screen->set_games(std::get<std::vector<lobby::SavedGameInfo>>(response.result));
    show_screen(resume_screen);
}

void LauncherWindow::resume_game(const QString& game_id) {
    send_to_lobby(lobby::ResumeGameRequest{game_id.toStdString()});
}

void LauncherWindow::handle(const lobby::ResumeGameResponse& response) {
    if (auto game_ticket = ticket_or_show_error(response.result, resume_screen)) {
        finish(std::move(*game_ticket));
    }
}

void LauncherWindow::leave_character_screen() {
    if (character_purpose == CharacterPurpose::NewGame) {
        show_screen(new_game_screen);
    } else {
        show_screen(join_screen);
    }
}

void LauncherWindow::confirm_character(const lobby::CharacterDraft& character) {
    if (character_purpose == CharacterPurpose::NewGame) {
        send_to_lobby(lobby::CreateGameRequest{pending_game_name.toStdString(),
                                               pending_world_id.toStdString(), character});
    } else {
        send_to_lobby(lobby::JoinGameRequest{pending_invite_code.toStdString(), character});
    }
}

void LauncherWindow::handle(const lobby::CreateGameResponse& response) {
    if (auto game_ticket = ticket_or_show_error(response.result, character_screen)) {
        finish(std::move(*game_ticket));
    }
}

void LauncherWindow::handle(const lobby::JoinGameResponse& response) {
    if (auto game_ticket = ticket_or_show_error(response.result, character_screen)) {
        finish(std::move(*game_ticket));
    }
}

void LauncherWindow::finish(lobby::GameTicket game_ticket) {
    ticket = std::move(game_ticket);
    close();
}

void LauncherWindow::send_to_lobby(const lobby::LobbyRequest& request) {
    screens->setEnabled(false);
    setCursor(Qt::BusyCursor);
    lobby_worker.submit(request);
    response_timer->start();
}

void LauncherWindow::poll_lobby() {
    lobby::LobbyResponse response;
    if (!lobby_worker.try_pop_response(response)) {
        return;
    }

    response_timer->stop();
    unsetCursor();
    screens->setEnabled(true);
    screens->currentWidget()->setFocus();
    std::visit([this](const auto& lobby_response) { handle(lobby_response); }, response);
}

void LauncherWindow::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    paint_landscape(painter, rect());
}

const std::optional<lobby::GameTicket>& LauncherWindow::get_ticket() const { return ticket; }

bool LauncherWindow::is_waiting_for_lobby() const { return response_timer->isActive(); }

}
