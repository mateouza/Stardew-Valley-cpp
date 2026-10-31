#include "server_application.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <utility>

#include "demo_map.h"
#include "server.h"
#include "world_config.h"

int ServerApplication::run(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " <puerto>\n";
        return EXIT_FAILURE;
    }

    try {
        auto demo = game::make_demo_map();
        const game::WorldConfig config = game::load_world_config("config/world.toml");

        Server server(argv[1], std::move(demo.map), demo.spawn_position, config);
        server.run();

        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
    } catch (...) {
        std::cerr << "Error desconocido\n";
    }

    return EXIT_FAILURE;
}