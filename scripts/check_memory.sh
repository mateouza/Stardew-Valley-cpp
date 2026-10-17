#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

shopt -s nullglob
test_binaries=(build/*_tests)

if (( ${#test_binaries[@]} == 0 )); then
    echo "No se encontraron ejecutables de tests" >&2
    exit 1
fi

for test_binary in "${test_binaries[@]}"; do
    echo "Revisando memoria: ${test_binary}"

    timeout --foreground 60s valgrind \
        --tool=memcheck \
        --leak-check=full \
        --show-leak-kinds=all \
        --errors-for-leak-kinds=definite,indirect \
        --track-origins=yes \
        --track-fds=yes \
        --error-exitcode=99 \
        "$test_binary"
done
