#include "launcher_window.h"

#include <QApplication>
#include <QPainter>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "qt/screens/connect_screen.h"
#include "qt/screens/main_menu_screen.h"
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
        main_menu_screen(new MainMenuScreen(screens)) {
    setWindowTitle("Stardew Valley");
    resize(1024, 720);
    setMinimumSize(900, 660);

    screens->addWidget(connect_screen);
    screens->addWidget(main_menu_screen);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(screens);

    connect(connect_screen, &ConnectScreen::connect_requested, this,
            &LauncherWindow::connect_to_server);
    connect(connect_screen, &ConnectScreen::quit_requested, this, &QWidget::close);
    connect(main_menu_screen, &MainMenuScreen::quit_requested, this, &QWidget::close);

    show_screen(connect_screen);
}

void LauncherWindow::show_screen(QWidget* screen) {
    screens->setCurrentWidget(screen);
    screen->setFocus();
}

void LauncherWindow::connect_to_server(const QString& username, const QString& host,
                                       const QString& port) {
    const BusyCursor busy;
    const auto error =
            lobby_client.connect(host.toStdString(), port.toStdString(), username.toStdString());
    if (error) {
        connect_screen->show_error(lobby_error_message(*error));
        return;
    }

    main_menu_screen->set_session(username, host + ":" + port);
    show_screen(main_menu_screen);
}

void LauncherWindow::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    paint_landscape(painter, rect());
}

const std::optional<lobby::GameTicket>& LauncherWindow::get_ticket() const { return ticket; }

}
