# GNU Make convenience entry points. CMake remains the authoritative build
# description so source lists and packaging rules have a single owner.

ifeq ($(origin CC),default)
CC := gcc
endif

COMPILER_NAME := $(notdir $(firstword $(CC)))
BUILD_DIR ?= build/linux-$(COMPILER_NAME)
BUILD_TYPE ?= Release
CMAKE ?= cmake
CMAKE_ARGS ?=
CMAKE_COMPILER_ARG := -DCMAKE_C_COMPILER="$(CC)"

.PHONY: all configure build test library-release clean

all: build

configure:
	$(CMAKE) -S . -B "$(BUILD_DIR)" -G "Unix Makefiles" \
		-DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" \
		-DWG_WARNINGS_AS_ERRORS=ON \
		$(CMAKE_COMPILER_ARG) $(CMAKE_ARGS)

build: configure
	$(CMAKE) --build "$(BUILD_DIR)" --parallel

test: build
	ctest --test-dir "$(BUILD_DIR)" --output-on-failure

library-release: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target library-release --parallel

clean:
	@if [ -f "$(BUILD_DIR)/CMakeCache.txt" ]; then \
		$(CMAKE) --build "$(BUILD_DIR)" --target clean; \
	fi
