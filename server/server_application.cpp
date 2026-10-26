#include "server_application.h"

#include <cstddef>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <utility>
#include <vector>

#include "server.h"

game::Map ServerApplication::make_initial_map() {
    constexpr std::size_t MAP_WIDTH = 10;
    constexpr std::size_t MAP_HEIGHT = 10;

    return game::Map{std::vector<std::vector<game::TileType>>(
            MAP_HEIGHT, std::vector<game::TileType>(MAP_WIDTH, game::TileType::Walkable))};
}

int ServerApplication::run(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " <puerto>\n";
        return EXIT_FAILURE;
    }

    try {
        auto map = make_initial_map();
        const game::Position spawn_position{0.5, 0.5};
        const game::WorldConfig config{4, 2.0, 0.6, 0.6};

        Server server(argv[1], std::move(map), spawn_position, config);
        server.run();

        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
    } catch (...) {
        std::cerr << "Error desconocido\n";
    }

    return EXIT_FAILURE;
}
