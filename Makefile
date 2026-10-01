# GNU Make convenience entry points. CMake remains the authoritative build
# description so source lists and packaging rules have a single owner.

ifeq ($(origin CC),default)
CC := gcc
endif

COMPILER_NAME := $(notdir $(firstword $(CC)))
PROJECT_VERSION := $(shell awk 'NR == 1 { print; exit }' VERSION)
CMAKE ?= cmake
CTEST ?= ctest
GIT ?= git
DOCKER ?= docker
CMAKE_GENERATOR ?= Unix Makefiles
BUILD_TYPE ?= Release
CMAKE_ARGS ?=
TEST_ARGS ?=
JOBS ?=
USE_SYSTEM_SDL3 ?= OFF

BUILD_DIR ?= build/linux-$(COMPILER_NAME)
LIBRARY_BUILD_DIR ?= build/linux-library-$(COMPILER_NAME)
CONSOLE_BUILD_DIR ?= build/linux-console-$(COMPILER_NAME)
SDL3_BUILD_DIR ?= build/linux-sdl3-$(COMPILER_NAME)
PORTABLE_LIBRARY_BUILD_DIR ?= build/linux-library-portable-debian10-gcc
PORTABLE_CONSOLE_BUILD_DIR ?= build/linux-console-portable-debian10-gcc
PORTABLE_SDL3_BUILD_DIR ?= build/linux-sdl3-portable-debian10-gcc
PORTABLE_LIBRARY_DIST_DIR ?= dist/wolf3dgeneric-$(PROJECT_VERSION)-library-linux-x64
PORTABLE_CONSOLE_DIST_DIR ?= dist/wolf3dgeneric-$(PROJECT_VERSION)-linux-console-x64
PORTABLE_SDL3_DIST_DIR ?= dist/wolf3dgeneric-$(PROJECT_VERSION)-sdl3-linux-x64
PORTABLE_BUILD_IMAGE ?= wolf3dgeneric-build-debian10
PORTABLE_GLIBC_MAX ?= 2.28
DOCKER_RUN_ARGS ?=

CMAKE_COMPILER_ARG := -DCMAKE_C_COMPILER="$(CC)"
PARALLEL_ARG := --parallel $(JOBS)

.DEFAULT_GOAL := all

.PHONY: all help dependencies check-version check-sdl3 dist releases configure build test \
	library library-release console console-release linux-console-release \
	sdl3 sdl3-configure sdl3-build sdl3-release \
	portable portable-library portable-library-release \
	portable-console portable-console-release portable-sdl3 portable-sdl3-release \
	portable-image portable-glibc-audit print-config \
	clean clean-library clean-console clean-sdl3 clean-portable

# The default is the package with the fewest host dependencies. The two
# runnable hosts remain one short, explicit target away.
all: library-release

help:
	@printf '%s\n' \
		'wolf3dgeneric GNU Make entry points' \
		'' \
		'Common distribution targets:' \
		'  make | make all              Stage the shared-library package (default).' \
		'  make library-release         Stage dist/...-library-linux-<arch>.' \
		'  make console-release         Stage dist/...-linux-console-<arch>.' \
		'  make sdl3-release            Stage dist/...-sdl3-linux-<arch>.' \
		'  make releases                Stage all three packages.' \
		'' \
		'Short aliases:' \
		'  make dist | make library     Same as library-release.' \
		'  make console                 Same as console-release.' \
		'  make sdl3                    Same as sdl3-release.' \
		'' \
		'Portable Debian 10/glibc 2.28 releases:' \
		'  make portable                Stage all three portable packages.' \
		'  make portable-library-release Stage the portable shared-library package.' \
		'  make portable-console-release Stage the portable direct-console package.' \
		'  make portable-sdl3-release   Stage the portable pinned-SDL3 package.' \
		'  make portable-library        Short alias for portable-library-release.' \
		'  make portable-console        Short alias for portable-console-release.' \
		'  make portable-sdl3           Short alias for portable-sdl3-release.' \
		'' \
		'Development and validation targets:' \
		'  make dependencies            Fetch the pinned SDL3 Git submodule.' \
		'  make check-version           Validate VERSION and its changelog entry.' \
		'  make configure               Configure the core/headless development tree.' \
		'  make build                   Build the core/headless development tree.' \
		'  make test                    Build and run its CTest suite.' \
		'  make sdl3-configure          Configure SDL3 without building it.' \
		'  make sdl3-build              Build SDL3 without staging dist/.' \
		'  make portable-glibc-audit    Verify all three portable packages against glibc 2.28.' \
		'  make print-config            Print the resolved Make settings.' \
		'' \
		'Cleanup targets (generated files only):' \
		'  make clean                   Clean BUILD_DIR and portable Docker artifacts.' \
		'  make clean-library           Clean LIBRARY_BUILD_DIR.' \
		'  make clean-console           Clean CONSOLE_BUILD_DIR.' \
		'  make clean-sdl3              Clean SDL3_BUILD_DIR.' \
		'  make clean-portable          Clean all portable trees and remove their builder image.' \
		'' \
		'Frequently used variables:' \
		'  CC=gcc|clang                 C compiler (default: gcc).' \
		'  BUILD_TYPE=Release|Debug     CMake build type (default: Release).' \
		'  JOBS=N                       Parallel job limit (default: CMake chooses).' \
		'  USE_SYSTEM_SDL3=ON|OFF       Use installed SDL >= 3.2 (default: OFF).' \
		'  PORTABLE_BUILD_IMAGE=NAME    Debian 10 builder image tag.' \
		'  PORTABLE_GLIBC_MAX=VERSION   Maximum allowed portable glibc ABI (default: 2.28).' \
		'  DOCKER_RUN_ARGS="..."        Extra docker-run options (for example :Z policy).' \
		'  CMAKE_ARGS="..."             Extra -D settings for configuration.' \
		'  TEST_ARGS="..."              Extra arguments passed to CTest.' \
		'' \
		'Advanced path/tool variables:' \
		'  CMAKE, CTEST, GIT, DOCKER, CMAKE_GENERATOR, BUILD_DIR,' \
		'  LIBRARY_BUILD_DIR, CONSOLE_BUILD_DIR, SDL3_BUILD_DIR,' \
		'  PORTABLE_LIBRARY_BUILD_DIR, PORTABLE_CONSOLE_BUILD_DIR,' \
		'  PORTABLE_SDL3_BUILD_DIR and corresponding *_DIST_DIR variables.' \
		'' \
		'Examples:' \
		'  make' \
		'  make dependencies' \
		'  make sdl3-release' \
		'  make portable JOBS=8' \
		'  make sdl3-release USE_SYSTEM_SDL3=ON' \
		'  make console-release CC=clang JOBS=8' \
		'  make test CMAKE_ARGS="-DWG_TEST_WL1_PATH=/games/WL1"'

print-config:
	@printf '%s\n' \
		'CC=$(CC)' \
		'PROJECT_VERSION=$(PROJECT_VERSION)' \
		'BUILD_TYPE=$(BUILD_TYPE)' \
		'BUILD_DIR=$(BUILD_DIR)' \
		'LIBRARY_BUILD_DIR=$(LIBRARY_BUILD_DIR)' \
		'CONSOLE_BUILD_DIR=$(CONSOLE_BUILD_DIR)' \
		'SDL3_BUILD_DIR=$(SDL3_BUILD_DIR)' \
		'PORTABLE_LIBRARY_BUILD_DIR=$(PORTABLE_LIBRARY_BUILD_DIR)' \
		'PORTABLE_CONSOLE_BUILD_DIR=$(PORTABLE_CONSOLE_BUILD_DIR)' \
		'PORTABLE_SDL3_BUILD_DIR=$(PORTABLE_SDL3_BUILD_DIR)' \
		'PORTABLE_LIBRARY_DIST_DIR=$(PORTABLE_LIBRARY_DIST_DIR)' \
		'PORTABLE_CONSOLE_DIST_DIR=$(PORTABLE_CONSOLE_DIST_DIR)' \
		'PORTABLE_SDL3_DIST_DIR=$(PORTABLE_SDL3_DIST_DIR)' \
		'PORTABLE_BUILD_IMAGE=$(PORTABLE_BUILD_IMAGE)' \
		'PORTABLE_GLIBC_MAX=$(PORTABLE_GLIBC_MAX)' \
		'USE_SYSTEM_SDL3=$(USE_SYSTEM_SDL3)' \
		'CMAKE_ARGS=$(CMAKE_ARGS)' \
		'JOBS=$(JOBS)'

dependencies:
	@if [ -f "third_party/SDL3/CMakeLists.txt" ] && \
	    [ -z "$$($(GIT) -C third_party/SDL3 rev-parse \
	        --show-superproject-working-tree 2>/dev/null)" ]; then \
		printf '%s\n' 'Pinned SDL3 sources are already present.'; \
	else \
		$(GIT) submodule update --init --recursive -- third_party/SDL3; \
	fi

check-version:
	$(CMAKE) -DWG_EXPECTED_VERSION="$(PROJECT_VERSION)" \
		-P cmake/WGVersion.cmake

check-sdl3:
	@if [ "$(USE_SYSTEM_SDL3)" != "ON" ] && \
	    [ ! -f "third_party/SDL3/CMakeLists.txt" ]; then \
		printf '%s\n' \
			'Pinned SDL3 sources are not initialized.' \
			'Run: make dependencies' \
			'Or:  git submodule update --init --recursive -- third_party/SDL3' \
			'Or build against installed SDL >= 3.2 with USE_SYSTEM_SDL3=ON.'; \
		exit 2; \
	fi

configure: check-version
	$(CMAKE) -S . -B "$(BUILD_DIR)" -G "$(CMAKE_GENERATOR)" \
		-DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" \
		-DWG_WARNINGS_AS_ERRORS=ON \
		-DWG_BUILD_LINUX_CONSOLE=OFF \
		-DWG_BUILD_SDL3=OFF \
		$(CMAKE_COMPILER_ARG) $(CMAKE_ARGS)

build: configure
	$(CMAKE) --build "$(BUILD_DIR)" $(PARALLEL_ARG)

test: build
	$(CTEST) --test-dir "$(BUILD_DIR)" --output-on-failure $(TEST_ARGS)

library-release:
	$(MAKE) configure CC="$(CC)" BUILD_TYPE="$(BUILD_TYPE)" \
		BUILD_DIR="$(LIBRARY_BUILD_DIR)" \
		CMAKE_ARGS="$(CMAKE_ARGS) -DBUILD_TESTING=OFF -DWG_BUILD_HEADLESS=OFF"
	$(CMAKE) --build "$(LIBRARY_BUILD_DIR)" --target library-release $(PARALLEL_ARG)

console-release:
	$(MAKE) configure CC="$(CC)" BUILD_TYPE="$(BUILD_TYPE)" \
		BUILD_DIR="$(CONSOLE_BUILD_DIR)" \
		CMAKE_ARGS="$(CMAKE_ARGS) -DBUILD_TESTING=OFF -DWG_BUILD_HEADLESS=OFF -DWG_BUILD_LINUX_CONSOLE=ON"
	$(CMAKE) --build "$(CONSOLE_BUILD_DIR)" --target linux-console-release $(PARALLEL_ARG)

# Compatibility with the original public Make target and the CMake target name.
linux-console-release: console-release

sdl3-configure: check-sdl3
	$(MAKE) configure CC="$(CC)" BUILD_TYPE="$(BUILD_TYPE)" \
		BUILD_DIR="$(SDL3_BUILD_DIR)" \
		CMAKE_ARGS="$(CMAKE_ARGS) -DBUILD_TESTING=OFF -DWG_BUILD_HEADLESS=OFF -DWG_BUILD_SDL3=ON -DWG_USE_SYSTEM_SDL3=$(USE_SYSTEM_SDL3)"

sdl3-build: sdl3-configure
	$(CMAKE) --build "$(SDL3_BUILD_DIR)" $(PARALLEL_ARG)

sdl3-release: sdl3-configure
	$(CMAKE) --build "$(SDL3_BUILD_DIR)" --target sdl3-release $(PARALLEL_ARG)

portable-image:
	$(DOCKER) build \
		--tag "$(PORTABLE_BUILD_IMAGE)" \
		packaging/linux-portable

portable-library-release: portable-image
	$(DOCKER) run --rm \
		--user "$$(id -u):$$(id -g)" \
		--volume "$(CURDIR):/src" \
		--workdir /src \
		$(DOCKER_RUN_ARGS) \
		"$(PORTABLE_BUILD_IMAGE)" \
		make library-release CC=gcc \
			LIBRARY_BUILD_DIR="$(PORTABLE_LIBRARY_BUILD_DIR)" JOBS="$(JOBS)"
	$(DOCKER) run --rm \
		--volume "$(CURDIR):/src:ro" \
		--workdir /src \
		$(DOCKER_RUN_ARGS) \
		"$(PORTABLE_BUILD_IMAGE)" \
		sh tools/WG_GLIBC_AUDIT.sh \
			"$(PORTABLE_LIBRARY_DIST_DIR)" "$(PORTABLE_GLIBC_MAX)"

portable-console-release: portable-image
	$(DOCKER) run --rm \
		--user "$$(id -u):$$(id -g)" \
		--volume "$(CURDIR):/src" \
		--workdir /src \
		$(DOCKER_RUN_ARGS) \
		"$(PORTABLE_BUILD_IMAGE)" \
		make console-release CC=gcc \
			CONSOLE_BUILD_DIR="$(PORTABLE_CONSOLE_BUILD_DIR)" JOBS="$(JOBS)"
	$(DOCKER) run --rm \
		--volume "$(CURDIR):/src:ro" \
		--workdir /src \
		$(DOCKER_RUN_ARGS) \
		"$(PORTABLE_BUILD_IMAGE)" \
		sh tools/WG_GLIBC_AUDIT.sh \
			"$(PORTABLE_CONSOLE_DIST_DIR)" "$(PORTABLE_GLIBC_MAX)"

portable-sdl3-release: check-sdl3 portable-image
	$(DOCKER) run --rm \
		--user "$$(id -u):$$(id -g)" \
		--volume "$(CURDIR):/src" \
		--workdir /src \
		$(DOCKER_RUN_ARGS) \
		"$(PORTABLE_BUILD_IMAGE)" \
		make sdl3-release CC=gcc \
			SDL3_BUILD_DIR="$(PORTABLE_SDL3_BUILD_DIR)" \
			JOBS="$(JOBS)" USE_SYSTEM_SDL3=OFF
	$(DOCKER) run --rm \
		--volume "$(CURDIR):/src:ro" \
		--workdir /src \
		$(DOCKER_RUN_ARGS) \
		"$(PORTABLE_BUILD_IMAGE)" \
		sh tools/WG_GLIBC_AUDIT.sh \
			"$(PORTABLE_SDL3_DIST_DIR)" "$(PORTABLE_GLIBC_MAX)"

portable-glibc-audit: portable-image
	$(DOCKER) run --rm \
		--volume "$(CURDIR):/src:ro" \
		--workdir /src \
		$(DOCKER_RUN_ARGS) \
		"$(PORTABLE_BUILD_IMAGE)" \
		sh tools/WG_GLIBC_AUDIT.sh \
			"$(PORTABLE_LIBRARY_DIST_DIR)" "$(PORTABLE_GLIBC_MAX)"
	$(DOCKER) run --rm \
		--volume "$(CURDIR):/src:ro" \
		--workdir /src \
		$(DOCKER_RUN_ARGS) \
		"$(PORTABLE_BUILD_IMAGE)" \
		sh tools/WG_GLIBC_AUDIT.sh \
			"$(PORTABLE_CONSOLE_DIST_DIR)" "$(PORTABLE_GLIBC_MAX)"
	$(DOCKER) run --rm \
		--volume "$(CURDIR):/src:ro" \
		--workdir /src \
		$(DOCKER_RUN_ARGS) \
		"$(PORTABLE_BUILD_IMAGE)" \
		sh tools/WG_GLIBC_AUDIT.sh \
			"$(PORTABLE_SDL3_DIST_DIR)" "$(PORTABLE_GLIBC_MAX)"

dist library: library-release
console: console-release
sdl3: sdl3-release
releases: library-release console-release sdl3-release
portable-library: portable-library-release
portable-console: portable-console-release
portable-sdl3: portable-sdl3-release
portable: portable-library-release portable-console-release portable-sdl3-release

clean: clean-portable
	@if [ -f "$(BUILD_DIR)/CMakeCache.txt" ]; then \
		$(CMAKE) --build "$(BUILD_DIR)" --target clean; \
	fi

clean-library:
	@if [ -f "$(LIBRARY_BUILD_DIR)/CMakeCache.txt" ]; then \
		$(CMAKE) --build "$(LIBRARY_BUILD_DIR)" --target clean; \
	fi

clean-console:
	@if [ -f "$(CONSOLE_BUILD_DIR)/CMakeCache.txt" ]; then \
		$(CMAKE) --build "$(CONSOLE_BUILD_DIR)" --target clean; \
	fi

clean-sdl3:
	@if [ -f "$(SDL3_BUILD_DIR)/CMakeCache.txt" ]; then \
		$(CMAKE) --build "$(SDL3_BUILD_DIR)" --target clean; \
	fi

clean-portable:
	@for portable_dir in \
		'$(PORTABLE_LIBRARY_BUILD_DIR)' \
		'$(PORTABLE_CONSOLE_BUILD_DIR)' \
		'$(PORTABLE_SDL3_BUILD_DIR)'; do \
		case "$$portable_dir" in \
			build/*) ;; \
			*) printf '%s\n' \
				'Refusing to remove a portable build tree outside build/:' \
				"  $$portable_dir"; exit 2 ;; \
		esac; \
		case "$$portable_dir" in \
			*..*) printf '%s\n' \
				'Refusing to remove a portable build tree containing ..:' \
				"  $$portable_dir"; exit 2 ;; \
		esac; \
		$(CMAKE) -E remove_directory "$$portable_dir"; \
	done
	@if command -v "$(firstword $(DOCKER))" >/dev/null 2>&1 && \
	    $(DOCKER) image inspect "$(PORTABLE_BUILD_IMAGE)" >/dev/null 2>&1; then \
		$(DOCKER) image rm "$(PORTABLE_BUILD_IMAGE)"; \
	fi
