.DEFAULT_GOAL := build

JOBS ?= 2

.PHONY: configure build test format-check lint check valgrind hooks clean

configure:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON

build: configure
	cmake --build build --parallel $(JOBS)

test: build
	ctest --test-dir build --output-on-failure --no-tests=error --parallel 1

format-check:
	pre-commit run clang-format --all-files

lint:
	pre-commit run cppcheck --all-files

check:
	$(MAKE) format-check
	$(MAKE) lint
	$(MAKE) test

valgrind: build
	bash scripts/check_memory.sh

hooks:
	pre-commit install

clean:
	cmake --build build --target clean