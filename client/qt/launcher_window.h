#ifndef CLIENT_QT_LAUNCHER_WINDOW_H_
#define CLIENT_QT_LAUNCHER_WINDOW_H_

#include <QString>
#include <QWidget>
#include <optional>

#include "lobby/lobby_client.h"
#include "lobby/lobby_requests.h"
#include "lobby/lobby_types.h"
#include "lobby/lobby_worker.h"

#include "launcher.h"

class QPaintEvent;
class QStackedWidget;
class QTimer;

namespace launcher {

class CharacterScreen;
class ConnectScreen;
class JoinScreen;
class MainMenuScreen;
class NewGameScreen;
class ResumeScreen;

// Controlador del launcher: cambia de pantalla, le pide cada operación al lobby y se cierra
// cuando consigue un ticket de partida. Los pedidos los ejecuta LobbyWorker en otro hilo; un
// QTimer consulta si llegó la respuesta, así que el hilo de Qt nunca espera al servidor.
class LauncherWindow: public QWidget {
    Q_OBJECT

private:
    // La pantalla de personaje se comparte: esto indica si al confirmar se crea o se une.
    enum class CharacterPurpose { NewGame, JoinGame };

    lobby::LobbyWorker lobby_worker;
    QStackedWidget* screens;
    ConnectScreen* connect_screen;
    MainMenuScreen* main_menu_screen;
    NewGameScreen* new_game_screen;
    JoinScreen* join_screen;
    CharacterScreen* character_screen;
    ResumeScreen* resume_screen;
    QTimer* response_timer;

    QString username;
    QString server_address;
    CharacterPurpose character_purpose = CharacterPurpose::NewGame;
    QString pending_game_name;
    QString pending_world_id;
    QString pending_invite_code;
    std::optional<lobby::GameTicket> ticket;

    void show_screen(QWidget* screen);
    void connect_to_server(const QString& requested_username, const QString& host,
                           const QString& port);
    void open_new_game();
    void choose_world(const QString& game_name, const QString& world_id, const QString& world_name);
    void open_join();
    void look_up_invite_code(const QString& invite_code);
    void open_resume();
    void resume_game(const QString& game_id);
    void leave_character_screen();
    void confirm_character(const lobby::CharacterDraft& character);
    void finish(lobby::GameTicket game_ticket);

    // Deshabilita las pantallas hasta que llegue la respuesta, para no mandar otro pedido.
    void send_to_lobby(const lobby::LobbyRequest& request);
    void poll_lobby();
    void handle(const lobby::ConnectResponse& response);
    void handle(const lobby::ListWorldsResponse& response);
    void handle(const lobby::ListSavedGamesResponse& response);
    void handle(const lobby::CreateGameResponse& response);
    void handle(const lobby::FindGameResponse& response);
    void handle(const lobby::JoinGameResponse& response);
    void handle(const lobby::ResumeGameResponse& response);

protected:
    void paintEvent(QPaintEvent* event) override;

public:
    LauncherWindow(lobby::LobbyClient& lobby_client, const LauncherOptions& options,
                   QWidget* parent = nullptr);

    const std::optional<lobby::GameTicket>& get_ticket() const;

    bool is_waiting_for_lobby() const;
};

}

#endif
