#include <QApplication>
#include <cstdlib>
#include <exception>
#include <iostream>

#include "editor/qt/editor_window.h"
#include "qt/launcher_style.h"

int main(int argc, char* argv[]) {
    try {
        QApplication app(argc, argv);
        app.setApplicationName("Editor de mapas");
        app.setStyleSheet(launcher::launcher_stylesheet());

        editor::EditorWindow window;
        window.show();
        return app.exec();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
    } catch (...) {
        std::cerr << "Error desconocido\n";
    }

    return EXIT_FAILURE;
}
