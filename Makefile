# GNU Make convenience entry points. CMake remains the authoritative build
# description so source lists and packaging rules have a single owner.

ifeq ($(origin CC),default)
CC := gcc
endif

COMPILER_NAME := $(notdir $(firstword $(CC)))
PROJECT_VERSION := $(shell awk 'NR == 1 { print; exit }' VERSION)
CMAKE ?= cmake
CTEST ?= ctest
DOCKER ?= docker
CMAKE_GENERATOR ?= Unix Makefiles
BUILD_TYPE ?= Release
CMAKE_ARGS ?=
TEST_ARGS ?=
JOBS ?=

BUILD_DIR ?= build/linux-$(COMPILER_NAME)
RELEASE_BUILD_DIR ?= build/linux-library-$(COMPILER_NAME)
PORTABLE_BUILD_DIR ?= build/linux-library-portable-debian10-gcc
PORTABLE_DIST_DIR ?= dist/wolf3d-$(PROJECT_VERSION)-library-linux-x64
PORTABLE_BUILD_IMAGE ?= wolf3d-lib-build-debian10
PORTABLE_GLIBC_MAX ?= 2.28
DOCKER_RUN_ARGS ?=

CMAKE_COMPILER_ARG := -DCMAKE_C_COMPILER="$(CC)"
PARALLEL_ARG := --parallel $(JOBS)

.DEFAULT_GOAL := all

.PHONY: all help check-version configure build test dist library 	library-release release portable portable-library 	portable-library-release portable-image portable-glibc-audit 	print-config clean clean-release clean-portable

all: library-release

help:
	@printf '%s\n' \
		'wolf3d-lib GNU Make entry points' \
		'' \
		'Release targets:' \
		'  make | make all              Stage the native shared-library package.' \
		'  make library-release         Stage dist/wolf3d-<version>-library-linux-<arch>.' \
		'  make portable-library-release Build and audit the Debian 10/glibc 2.28 package.' \
		'' \
		'Short aliases:' \
		'  make dist | make library | make release  Same as library-release.' \
		'  make portable | make portable-library    Same as portable-library-release.' \
		'' \
		'Development and validation:' \
		'  make configure               Configure the library and validation tests.' \
		'  make build                   Build the development tree.' \
		'  make test                    Build and run the CTest suite.' \
		'  make check-version           Validate VERSION and its changelog entry.' \
		'  make portable-glibc-audit    Audit an existing portable package.' \
		'  make print-config            Print resolved Make settings.' \
		'' \
		'Cleanup:' \
		'  make clean                   Clean development, release, and Docker artifacts.' \
		'  make clean-release           Clean RELEASE_BUILD_DIR.' \
		'  make clean-portable          Clean the portable tree and builder image.' \
		'' \
		'Common variables:' \
		'  CC=gcc|clang                 C compiler (default: gcc).' \
		'  BUILD_TYPE=Release|Debug     CMake build type (default: Release).' \
		'  JOBS=N                       Parallel job limit.' \
		'  CMAKE_ARGS="..."             Extra CMake -D settings.' \
		'  TEST_ARGS="..."              Extra arguments passed to CTest.' \
		'  DOCKER_RUN_ARGS="..."        Extra docker-run options.' \
		'' \
		'Examples:' \
		'  make' \
		'  make test CC=clang JOBS=8' \
		'  make portable JOBS=8'

print-config:
	@printf '%s\n' \
		'CC=$(CC)' \
		'PROJECT_VERSION=$(PROJECT_VERSION)' \
		'BUILD_TYPE=$(BUILD_TYPE)' \
		'BUILD_DIR=$(BUILD_DIR)' \
		'RELEASE_BUILD_DIR=$(RELEASE_BUILD_DIR)' \
		'PORTABLE_BUILD_DIR=$(PORTABLE_BUILD_DIR)' \
		'PORTABLE_DIST_DIR=$(PORTABLE_DIST_DIR)' \
		'PORTABLE_BUILD_IMAGE=$(PORTABLE_BUILD_IMAGE)' \
		'PORTABLE_GLIBC_MAX=$(PORTABLE_GLIBC_MAX)' \
		'CMAKE_ARGS=$(CMAKE_ARGS)' \
		'JOBS=$(JOBS)'

check-version:
	$(CMAKE) -DWG_EXPECTED_VERSION="$(PROJECT_VERSION)" \
		-P cmake/WGVersion.cmake

configure: check-version
	$(CMAKE) -S . -B "$(BUILD_DIR)" -G "$(CMAKE_GENERATOR)" \
		-DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" \
		-DWG_WARNINGS_AS_ERRORS=ON \
		$(CMAKE_COMPILER_ARG) $(CMAKE_ARGS)

build: configure
	$(CMAKE) --build "$(BUILD_DIR)" $(PARALLEL_ARG)

test: build
	$(CTEST) --test-dir "$(BUILD_DIR)" --output-on-failure $(TEST_ARGS)

library-release: check-version
	$(CMAKE) -S . -B "$(RELEASE_BUILD_DIR)" -G "$(CMAKE_GENERATOR)" \
		-DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" \
		-DBUILD_TESTING=OFF -DWG_BUILD_HEADLESS=OFF \
		-DWG_WARNINGS_AS_ERRORS=ON \
		$(CMAKE_COMPILER_ARG) $(CMAKE_ARGS)
	$(CMAKE) --build "$(RELEASE_BUILD_DIR)" --target library-release $(PARALLEL_ARG)

portable-image:
	$(DOCKER) build --tag "$(PORTABLE_BUILD_IMAGE)" packaging/linux-portable

portable-library-release: portable-image
	$(DOCKER) run --rm \
		--user "$$(id -u):$$(id -g)" \
		--volume "$(CURDIR):/src" \
		--workdir /src \
		$(DOCKER_RUN_ARGS) \
		"$(PORTABLE_BUILD_IMAGE)" \
		make library-release CC=gcc \
			RELEASE_BUILD_DIR="$(PORTABLE_BUILD_DIR)" JOBS="$(JOBS)"
	$(DOCKER) run --rm \
		--volume "$(CURDIR):/src:ro" \
		--workdir /src \
		$(DOCKER_RUN_ARGS) \
		"$(PORTABLE_BUILD_IMAGE)" \
		sh tools/WG_GLIBC_AUDIT.sh \
			"$(PORTABLE_DIST_DIR)" "$(PORTABLE_GLIBC_MAX)"

portable-glibc-audit: portable-image
	$(DOCKER) run --rm \
		--volume "$(CURDIR):/src:ro" \
		--workdir /src \
		$(DOCKER_RUN_ARGS) \
		"$(PORTABLE_BUILD_IMAGE)" \
		sh tools/WG_GLIBC_AUDIT.sh \
			"$(PORTABLE_DIST_DIR)" "$(PORTABLE_GLIBC_MAX)"

dist library release: library-release
portable portable-library: portable-library-release

clean: clean-release clean-portable
	@if [ -f "$(BUILD_DIR)/CMakeCache.txt" ]; then \
		$(CMAKE) --build "$(BUILD_DIR)" --target clean; \
	fi

clean-release:
	@if [ -f "$(RELEASE_BUILD_DIR)/CMakeCache.txt" ]; then \
		$(CMAKE) --build "$(RELEASE_BUILD_DIR)" --target clean; \
	fi

clean-portable:
	@case "$(PORTABLE_BUILD_DIR)" in \
		build/*) ;; \
		*) printf '%s\n' \
			'Refusing to remove a portable build tree outside build/:' \
			'  $(PORTABLE_BUILD_DIR)'; exit 2 ;; \
	esac
	@case "$(PORTABLE_BUILD_DIR)" in \
		*..*) printf '%s\n' \
			'Refusing to remove a portable build tree containing ..:' \
			'  $(PORTABLE_BUILD_DIR)'; exit 2 ;; \
	esac
	$(CMAKE) -E remove_directory "$(PORTABLE_BUILD_DIR)"
	@if command -v "$(firstword $(DOCKER))" >/dev/null 2>&1 && \
	    $(DOCKER) image inspect "$(PORTABLE_BUILD_IMAGE)" >/dev/null 2>&1; then \
		$(DOCKER) image rm "$(PORTABLE_BUILD_IMAGE)"; \
	fi
