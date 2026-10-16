cmake_minimum_required(VERSION 3.28)

project(StardewValley LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2pp REQUIRED)

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

add_executable(stardew_client
    client/sdl/main_test.cpp
    client/sdl/render/renderer.cpp
    client/sdl/state/client_state.cpp
    client/sdl/core/input_handler.cpp
)

target_link_libraries(stardew_client PRIVATE
    course_support
    SDL2pp::SDL2pp
)

target_compile_options(stardew_client PRIVATE -Wall -Wextra -Wpedantic)
add_library(server_model STATIC
    server/player.cpp
    server/map.cpp
    server/world.cpp
)

target_include_directories(server_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/server)
target_link_libraries(server_model PUBLIC game_contracts)
target_compile_options(server_model PRIVATE -Wall -Wextra -Wpedantic)

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

endif()
