cmake_minimum_required(VERSION 3.28)

project(StardewValley LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_CLIENT "Compilar el cliente gráfico (requiere SDL2 y sdl2pp)" ON)
option(BUILD_LAUNCHER "Compilar el launcher Qt (requiere Qt6 Widgets)" ON)

find_package(Threads REQUIRED)

add_library(game_contracts INTERFACE)

target_include_directories(game_contracts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/common)
target_compile_features(game_contracts INTERFACE cxx_std_20)

add_library(course_support STATIC
    common/common_socket.cpp
    common/common_resolver.cpp
    common/common_resolvererror.cpp
    common/common_liberror.cpp
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
)

target_include_directories(client_lobby PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/client)
target_compile_options(client_lobby PRIVATE -Wall -Wextra -Wpedantic)

if(BUILD_LAUNCHER)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)

    add_library(qt_launcher STATIC
        client/qt/launcher.cpp
        client/qt/launcher_style.cpp
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
        client/qt/widgets/ui_helpers.cpp
        client/qt/widgets/world_card.cpp
    )

    set_target_properties(qt_launcher PROPERTIES AUTOMOC ON)
    target_link_libraries(qt_launcher PUBLIC client_lobby Qt6::Widgets)
    target_compile_options(qt_launcher PRIVATE -Wall -Wextra -Wpedantic)

    add_executable(stardew_launcher client/qt/main.cpp)
    target_link_libraries(stardew_launcher PRIVATE qt_launcher)
    target_compile_options(stardew_launcher PRIVATE -Wall -Wextra -Wpedantic)
endif()

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

    add_executable(fake_lobby_client_tests tests/fake_lobby_client_test.cpp)
    target_link_libraries(fake_lobby_client_tests PRIVATE client_lobby GTest::gtest_main)
    target_compile_options(fake_lobby_client_tests PRIVATE -Wall -Wextra -Wpedantic)

    gtest_discover_tests(fake_lobby_client_tests)

    if(BUILD_LAUNCHER)
        add_executable(launcher_window_tests tests/launcher_window_test.cpp)
        target_link_libraries(launcher_window_tests PRIVATE qt_launcher GTest::gtest)
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
        course_support
        game_contracts
        SDL2pp::SDL2pp
    )

    target_compile_options(stardew_client PRIVATE -Wall -Wextra -Wpedantic)
endif()