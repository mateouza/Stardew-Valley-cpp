cmake_minimum_required(VERSION 3.28)

project(StardewValley LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

add_library(server_core STATIC
    server/acceptor.cpp
    server/client_session.cpp
    server/client_receiver.cpp
    server/game_loop.cpp
    server/server.cpp
    server/server_protocol.cpp
)

target_include_directories(server_core PUBLIC server)

target_link_libraries(server_core PUBLIC course_support game_contracts)

target_compile_options(server_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(server server/main.cpp)

target_link_libraries(server PRIVATE server_core)

target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic)

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
    target_link_libraries(server_protocol_tests PRIVATE server_core GTest::gtest_main)
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

endif()