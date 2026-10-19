#ifndef CLIENT_QT_LAUNCHER_WINDOW_H_
#define CLIENT_QT_LAUNCHER_WINDOW_H_

#include <QString>
#include <QWidget>
#include <optional>

#include "lobby/lobby_client.h"
#include "lobby/lobby_types.h"

#include "launcher.h"

class QPaintEvent;
class QStackedWidget;

namespace launcher {

class ConnectScreen;
class MainMenuScreen;

// Controlador del launcher: cambia de pantalla, le pide cada operación al lobby y se cierra
// cuando consigue un ticket de partida.
class LauncherWindow: public QWidget {
    Q_OBJECT

private:
    lobby::LobbyClient& lobby_client;
    QStackedWidget* screens;
    ConnectScreen* connect_screen;
    MainMenuScreen* main_menu_screen;
    std::optional<lobby::GameTicket> ticket;

    void show_screen(QWidget* screen);
    void connect_to_server(const QString& username, const QString& host, const QString& port);

protected:
    void paintEvent(QPaintEvent* event) override;

public:
    LauncherWindow(lobby::LobbyClient& lobby_client, const LauncherOptions& options,
                   QWidget* parent = nullptr);

    const std::optional<lobby::GameTicket>& get_ticket() const;
};

}

#endif
