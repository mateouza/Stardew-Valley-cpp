#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

test_binaries=(
    build/player_tests
    build/map_tests
    build/world_tests
    build/world_config_tests
    build/server_protocol_tests
    build/game_loop_tests
    build/world_game_loop_tests
    build/match_tests
)

for test_binary in "${test_binaries[@]}"; do
    if [[ ! -f "$test_binary" || ! -x "$test_binary" ]]; then
        echo "Falta el ejecutable de tests: ${test_binary}. Ejecuta make build." >&2
        exit 1
    fi
done

for test_binary in "${test_binaries[@]}"; do
    echo "Revisando memoria: ${test_binary}"

    timeout --foreground --kill-after=5s 60s valgrind \
        --tool=memcheck \
        --leak-check=full \
        --show-leak-kinds=all \
        --errors-for-leak-kinds=definite,indirect \
        --track-origins=yes \
        --track-fds=yes \
        --error-exitcode=99 \
        "$test_binary"
done
