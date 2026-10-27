cmake_minimum_required(VERSION 3.28)

project(StardewValley LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_CLIENT "Compilar el cliente gráfico (requiere SDL2 y sdl2pp)" ON)
option(BUILD_LAUNCHER "Compilar el launcher Qt (requiere Qt6 Widgets)" ON)
option(BUILD_EDITOR "Compilar el editor de mapas Qt (requiere Qt6 Widgets)" ON)

find_package(Threads REQUIRED)

add_library(game_contracts INTERFACE)

target_include_directories(game_contracts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/common)
target_compile_features(game_contracts INTERFACE cxx_std_20)

add_library(course_support STATIC
    common/common_socket.cpp
    common/common_resolver.cpp
    common/common_resolvererror.cpp
    common/common_liberror.cpp
    common/protocol_base.cpp
)

target_include_directories(course_support PUBLIC common)

target_compile_definitions(course_support PRIVATE _POSIX_C_SOURCE=200809L)

target_link_libraries(course_support PUBLIC Threads::Threads)

target_compile_options(course_support PRIVATE -Wall -Wextra -Wpedantic)

add_library(server_model STATIC
    server/player.cpp
    server/map.cpp
    server/world.cpp
)

target_include_directories(server_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/server)

target_link_libraries(server_model PUBLIC game_contracts)

target_compile_options(server_model PRIVATE -Wall -Wextra -Wpedantic)

add_library(client_lobby STATIC
    client/lobby/fake_lobby_client.cpp
    client/lobby/lobby_validation.cpp
    client/lobby/lobby_worker.cpp
)

target_include_directories(client_lobby PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/client)
target_link_libraries(client_lobby PUBLIC game_contracts Threads::Threads)
target_compile_options(client_lobby PRIVATE -Wall -Wextra -Wpedantic)

if(BUILD_LAUNCHER OR BUILD_EDITOR)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)

    # Hoja de estilo y helpers que comparten el launcher y el editor.
    add_library(qt_ui STATIC
        client/qt/launcher_style.cpp
        client/qt/widgets/ui_helpers.cpp
    )

    target_link_libraries(qt_ui PUBLIC client_lobby Qt6::Widgets)
    target_compile_options(qt_ui PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_LAUNCHER)
    add_library(qt_launcher STATIC
        client/qt/launcher.cpp
        client/qt/launcher_window.cpp
        client/qt/lobby_error_message.cpp
        client/qt/screens/character_screen.cpp
        client/qt/screens/connect_screen.cpp
        client/qt/screens/join_screen.cpp
        client/qt/screens/main_menu_screen.cpp
        client/qt/screens/new_game_screen.cpp
        client/qt/screens/resume_screen.cpp
        client/qt/widgets/character_preview.cpp
        client/qt/widgets/character_sprite.cpp
        client/qt/widgets/landscape.cpp
        client/qt/widgets/option_selector.cpp
        client/qt/widgets/saved_game_card.cpp
        client/qt/widgets/world_card.cpp
    )

    set_target_properties(qt_launcher PROPERTIES AUTOMOC ON)
    target_link_libraries(qt_launcher PUBLIC qt_ui client_lobby Qt6::Widgets)
    target_compile_options(qt_launcher PRIVATE -Wall -Wextra -Wpedantic)

    add_executable(stardew_launcher client/qt/main.cpp)
    target_link_libraries(stardew_launcher PRIVATE qt_launcher)
    target_compile_options(stardew_launcher PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_EDITOR)
    find_package(tomlplusplus 3.4 CONFIG REQUIRED)

    # Modelo del mapa, sin Qt.
    add_library(editor_model STATIC
        editor/model/map_file.cpp
        editor/model/map_validation.cpp
        editor/model/tile.cpp
        editor/model/world_map.cpp
        editor/model/zone.cpp
    )

    target_include_directories(editor_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(editor_model PUBLIC client_lobby PRIVATE tomlplusplus::tomlplusplus)
    target_compile_options(editor_model PRIVATE -Wall -Wextra -Wpedantic)

    add_library(qt_editor STATIC
        editor/qt/editor_window.cpp
        editor/qt/tile_colors.cpp
    )

    set_target_properties(qt_editor PROPERTIES AUTOMOC ON)
    target_include_directories(qt_editor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(qt_editor PUBLIC editor_model qt_ui Qt6::Widgets)
    target_compile_options(qt_editor PRIVATE -Wall -Wextra -Wpedantic)

    add_executable(stardew_editor editor/main.cpp)
    target_link_libraries(stardew_editor PRIVATE qt_editor)
    target_compile_options(stardew_editor PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_library(server_core STATIC
    server/acceptor.cpp
    server/client_session.cpp
    server/client_receiver.cpp
    server/client_sender.cpp
    server/game_loop.cpp
    server/server.cpp
    server/server_protocol.cpp
    server/match.cpp
    server/tick_scheduler.cpp
    server/match_manager.cpp
    server/client_handler.cpp
)

target_include_directories(server_core PUBLIC server)

target_link_libraries(server_core PUBLIC course_support game_contracts server_model)

target_compile_options(server_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(server
    server/main.cpp
    server/server_application.cpp
)

target_link_libraries(server PRIVATE server_core)

target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic)

add_library(client_network STATIC
    client/network/client_protocol.cpp
    client/network/network_sender.cpp
    client/network/network_receiver.cpp
    client/network/network_client.cpp
)

target_include_directories(client_network PUBLIC client/network)

target_link_libraries(client_network PUBLIC course_support game_contracts)

target_compile_options(client_network PRIVATE -Wall -Wextra -Wpedantic)

include(CTest)

if(BUILD_TESTING)
    find_package(GTest REQUIRED)
    include(GoogleTest)

    add_executable(player_tests tests/player_test.cpp)
    target_link_libraries(player_tests PRIVATE server_model GTest::gtest_main)
    target_compile_options(player_tests PRIVATE -Wall -Wextra -Wpedantic)

    gtest_discover_tests(player_tests)

    add_executable(map_tests tests/map_test.cpp)
    target_link_libraries(map_tests PRIVATE server_model GTest::gtest_main)
    target_compile_options(map_tests PRIVATE -Wall -Wextra -Wpedantic)

    gtest_discover_tests(map_tests)

    add_executable(world_tests tests/world_test.cpp)
    target_link_libraries(world_tests PRIVATE server_model GTest::gtest_main)
    target_compile_options(world_tests PRIVATE -Wall -Wextra -Wpedantic)

    gtest_discover_tests(world_tests)
add_executable(server_protocol_tests tests/server_protocol_test.cpp)
    target_link_libraries(server_protocol_tests PRIVATE
        server_core
        client_network
        GTest::gtest_main
    )
    target_compile_options(server_protocol_tests PRIVATE -Wall -Wextra -Wpedantic)

    gtest_discover_tests(server_protocol_tests
        PROPERTIES
            TIMEOUT 5
            RESOURCE_LOCK protocol_port_18080
    )

    add_executable(game_loop_tests tests/game_loop_test.cpp)
    target_link_libraries(game_loop_tests PRIVATE server_core server_model GTest::gtest_main)
    target_compile_options(game_loop_tests PRIVATE -Wall -Wextra -Wpedantic)

    gtest_discover_tests(game_loop_tests PROPERTIES TIMEOUT 5)

    add_executable(match_tests tests/match_test.cpp)
    target_link_libraries(match_tests PRIVATE server_core GTest::gtest_main)
    target_compile_options(match_tests PRIVATE -Wall -Wextra -Wpedantic)

    add_executable(fake_lobby_client_tests tests/fake_lobby_client_test.cpp)
    target_link_libraries(fake_lobby_client_tests PRIVATE client_lobby GTest::gtest_main)
    target_compile_options(fake_lobby_client_tests PRIVATE -Wall -Wextra -Wpedantic)

    gtest_discover_tests(fake_lobby_client_tests)

    add_executable(lobby_validation_tests tests/lobby_validation_test.cpp)
    target_link_libraries(lobby_validation_tests PRIVATE client_lobby GTest::gtest_main)
    target_compile_options(lobby_validation_tests PRIVATE -Wall -Wextra -Wpedantic)

    gtest_discover_tests(lobby_validation_tests)

    add_executable(lobby_worker_tests tests/lobby_worker_test.cpp)
    target_link_libraries(lobby_worker_tests PRIVATE client_lobby GTest::gtest_main)
    target_compile_options(lobby_worker_tests PRIVATE -Wall -Wextra -Wpedantic)

    gtest_discover_tests(lobby_worker_tests PROPERTIES TIMEOUT 5)

    if(BUILD_LAUNCHER)
        find_package(Qt6 REQUIRED COMPONENTS Test)

        add_executable(launcher_window_tests tests/launcher_window_test.cpp)
        target_link_libraries(launcher_window_tests PRIVATE qt_launcher Qt6::Test GTest::gtest)
        target_compile_options(launcher_window_tests PRIVATE -Wall -Wextra -Wpedantic)

        gtest_discover_tests(launcher_window_tests
            PROPERTIES
                ENVIRONMENT QT_QPA_PLATFORM=offscreen
                TIMEOUT 10
        )

        add_executable(character_widgets_tests tests/character_widgets_test.cpp)
        target_link_libraries(character_widgets_tests PRIVATE qt_launcher GTest::gtest)
        target_compile_options(character_widgets_tests PRIVATE -Wall -Wextra -Wpedantic)

        gtest_discover_tests(character_widgets_tests
            PROPERTIES
                ENVIRONMENT QT_QPA_PLATFORM=offscreen
                TIMEOUT 10
        )
    endif()

    if(BUILD_EDITOR)
        add_executable(editor_model_tests
            tests/editor_map_file_test.cpp
            tests/editor_map_validation_test.cpp
            tests/editor_tile_test.cpp
            tests/editor_world_map_test.cpp
            tests/editor_zone_test.cpp
        )
        target_link_libraries(editor_model_tests
            PRIVATE editor_model tomlplusplus::tomlplusplus GTest::gtest_main)
        target_compile_options(editor_model_tests PRIVATE -Wall -Wextra -Wpedantic)

        gtest_discover_tests(editor_model_tests)

        find_package(Qt6 REQUIRED COMPONENTS Test)

        add_executable(editor_qt_tests
            tests/editor_qt_main.cpp
            tests/editor_tile_colors_test.cpp
        )
        target_link_libraries(editor_qt_tests PRIVATE qt_editor Qt6::Test GTest::gtest)
        target_compile_options(editor_qt_tests PRIVATE -Wall -Wextra -Wpedantic)

        gtest_discover_tests(editor_qt_tests
            PROPERTIES
                ENVIRONMENT QT_QPA_PLATFORM=offscreen
                TIMEOUT 10
        )
    endif()

    gtest_discover_tests(match_tests PROPERTIES TIMEOUT 5)
endif()

if(BUILD_CLIENT)
    find_package(SDL2 REQUIRED)
    find_package(SDL2pp REQUIRED)

    add_executable(stardew_client
        client/sdl/main.cpp
        client/sdl/render/renderer.cpp
        client/sdl/render/sdl_window.cpp
        client/sdl/state/client_state.cpp
        client/sdl/core/input_handler.cpp
    )

    target_link_libraries(stardew_client PRIVATE
        client_network
        course_support
        game_contracts
        SDL2pp::SDL2pp
    )

    target_compile_options(stardew_client PRIVATE -Wall -Wextra -Wpedantic)
endif()