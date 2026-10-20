#include <cstdlib>
#include <exception>
#include <iostream>

#include "server.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " <puerto>\n";
        return EXIT_FAILURE;
    }

    try {
        Server server(argv[1]);
        server.run();
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
    } catch (...) {
        std::cerr << "Error desconocido\n";
    }

    return EXIT_FAILURE;
}
