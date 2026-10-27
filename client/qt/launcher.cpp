#include "launcher.h"

#include <QApplication>

#include "launcher_style.h"
#include "launcher_window.h"

namespace launcher {

std::optional<lobby::GameTicket> run_launcher(int& argc, char** argv,
                                              lobby::LobbyClient& lobby_client,
                                              const LauncherOptions& options) {
    QApplication app(argc, argv);
    app.setApplicationName("Stardew Valley");
    app.setStyleSheet(launcher_stylesheet());

    LauncherWindow window(lobby_client, options);
    window.show();
    app.exec();

    return window.get_ticket();
}

}
