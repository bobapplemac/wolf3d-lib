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
AUDIO_BACKEND ?= standard
OPL_BACKEND ?= nuked
CMAKE_ARGS ?=
TEST_ARGS ?=
JOBS ?=

BACKEND_SUFFIX :=
ifeq ($(AUDIO_BACKEND),silent)
BACKEND_SUFFIX := -silent
else ifneq ($(OPL_BACKEND),nuked)
BACKEND_SUFFIX := -$(OPL_BACKEND)
endif

BUILD_DIR ?= build/linux-$(COMPILER_NAME)$(BACKEND_SUFFIX)
RELEASE_BUILD_DIR ?= build/linux-library-$(COMPILER_NAME)$(BACKEND_SUFFIX)
PORTABLE_BUILD_DIR ?= build/linux-library-portable-debian10-gcc
PORTABLE_DIST_DIR ?= dist/wolf3d-$(PROJECT_VERSION)-library-linux-x64$(BACKEND_SUFFIX)
PORTABLE_BUILD_IMAGE ?= wolf3d-lib-build-debian10
PORTABLE_GLIBC_MAX ?= 2.28
MUSL_BUILD_DIR ?= build/linux-library-musl-$(COMPILER_NAME)$(BACKEND_SUFFIX)
MUSL_DIST_DIR ?= dist/wolf3d-$(PROJECT_VERSION)-library-linux-musl-x64$(BACKEND_SUFFIX)
MUSL_BUILD_IMAGE ?= wolf3d-lib-build-alpine-musl
DOCKER_RUN_ARGS ?=

CMAKE_COMPILER_ARG := -DCMAKE_C_COMPILER="$(CC)"
CMAKE_BACKEND_ARGS := -DWG_AUDIO_BACKEND="$(AUDIO_BACKEND)" \
	-DWG_OPL_BACKEND="$(OPL_BACKEND)"
PARALLEL_ARG := --parallel $(JOBS)

.DEFAULT_GOAL := all

.PHONY: all help check-version configure build test dist library \
	library-release release portable portable-library \
	portable-library-release portable-image portable-glibc-audit \
	musl musl-library musl-library-release musl-image musl-audit \
	print-config clean clean-release clean-portable clean-musl

all: library-release

help:
	@printf '%s\n' \
		'wolf3d-lib GNU Make entry points' \
		'' \
		'Release targets:' \
		'  make | make all              Stage the native shared-library package.' \
		'  make library-release         Stage dist/wolf3d-<version>-library-linux-<arch>.' \
		'  make portable-library-release Build and audit the Debian 10/glibc 2.28 package.' \
		'  make musl-library-release    Build and audit the Alpine/musl package.' \
		'' \
		'Short aliases:' \
		'  make dist | make library | make release  Same as library-release.' \
		'  make portable | make portable-library    Same as portable-library-release.' \
		'  make musl | make musl-library            Same as musl-library-release.' \
		'' \
		'Development and validation:' \
		'  make configure               Configure the library and validation tests.' \
		'  make build                   Build the development tree.' \
		'  make test                    Build and run the CTest suite.' \
		'  make check-version           Validate VERSION and its changelog entry.' \
		'  make portable-glibc-audit    Audit an existing portable package.' \
		'  make musl-audit              Audit an existing musl package.' \
		'  make print-config            Print resolved Make settings.' \
		'' \
		'Cleanup:' \
		'  make clean                   Clean development, release, and Docker artifacts.' \
		'  make clean-release           Clean RELEASE_BUILD_DIR.' \
		'  make clean-portable          Clean the portable tree and builder image.' \
		'  make clean-musl              Clean the musl tree and builder image.' \
		'' \
		'Common variables:' \
		'  CC=gcc|clang                 C compiler (default: gcc).' \
		'  BUILD_TYPE=Release|Debug     CMake build type (default: Release).' \
		'  AUDIO_BACKEND=standard|silent Audio output profile (default: standard).' \
		'  OPL_BACKEND=nuked|dbopl      Standard-audio OPL emulator (default: nuked).' \
		'  JOBS=N                       Parallel job limit.' \
		'  CMAKE_ARGS="..."             Extra CMake -D settings.' \
		'  TEST_ARGS="..."              Extra arguments passed to CTest.' \
		'  DOCKER_RUN_ARGS="..."        Extra docker-run options.' \
		'' \
		'Examples:' \
		'  make' \
		'  make test CC=clang JOBS=8' \
		'  make test OPL_BACKEND=dbopl' \
		'  make test AUDIO_BACKEND=silent' \
		'  make portable JOBS=8' \
		'  make musl OPL_BACKEND=dbopl JOBS=8'

print-config:
	@printf '%s\n' \
		'CC=$(CC)' \
		'PROJECT_VERSION=$(PROJECT_VERSION)' \
		'BUILD_TYPE=$(BUILD_TYPE)' \
		'AUDIO_BACKEND=$(AUDIO_BACKEND)' \
		'OPL_BACKEND=$(OPL_BACKEND)' \
		'BUILD_DIR=$(BUILD_DIR)' \
		'RELEASE_BUILD_DIR=$(RELEASE_BUILD_DIR)' \
		'PORTABLE_BUILD_DIR=$(PORTABLE_BUILD_DIR)' \
		'PORTABLE_DIST_DIR=$(PORTABLE_DIST_DIR)' \
		'PORTABLE_BUILD_IMAGE=$(PORTABLE_BUILD_IMAGE)' \
		'PORTABLE_GLIBC_MAX=$(PORTABLE_GLIBC_MAX)' \
		'MUSL_BUILD_DIR=$(MUSL_BUILD_DIR)' \
		'MUSL_DIST_DIR=$(MUSL_DIST_DIR)' \
		'MUSL_BUILD_IMAGE=$(MUSL_BUILD_IMAGE)' \
		'CMAKE_ARGS=$(CMAKE_ARGS)' \
		'JOBS=$(JOBS)'

check-version:
	$(CMAKE) -DWG_EXPECTED_VERSION="$(PROJECT_VERSION)" \
		-P cmake/WGVersion.cmake

configure: check-version
	$(CMAKE) -S . -B "$(BUILD_DIR)" -G "$(CMAKE_GENERATOR)" \
		-DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" \
		-DWG_WARNINGS_AS_ERRORS=ON \
		$(CMAKE_COMPILER_ARG) $(CMAKE_BACKEND_ARGS) $(CMAKE_ARGS)

build: configure
	$(CMAKE) --build "$(BUILD_DIR)" $(PARALLEL_ARG)

test: build
	$(CTEST) --test-dir "$(BUILD_DIR)" --output-on-failure $(TEST_ARGS)

library-release: check-version
	$(CMAKE) -S . -B "$(RELEASE_BUILD_DIR)" -G "$(CMAKE_GENERATOR)" \
		-DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" \
		-DBUILD_TESTING=OFF -DWG_BUILD_HEADLESS=OFF \
		-DWG_WARNINGS_AS_ERRORS=ON \
		$(CMAKE_COMPILER_ARG) $(CMAKE_BACKEND_ARGS) $(CMAKE_ARGS)
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
			AUDIO_BACKEND="$(AUDIO_BACKEND)" OPL_BACKEND="$(OPL_BACKEND)" \
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

musl-image:
	$(DOCKER) build --tag "$(MUSL_BUILD_IMAGE)" packaging/linux-musl

musl-library-release: musl-image
	$(DOCKER) run --rm \
		--user "$$(id -u):$$(id -g)" \
		--volume "$(CURDIR):/src" \
		--workdir /src \
		$(DOCKER_RUN_ARGS) \
		"$(MUSL_BUILD_IMAGE)" \
		make library-release CC="$(CC)" \
			AUDIO_BACKEND="$(AUDIO_BACKEND)" OPL_BACKEND="$(OPL_BACKEND)" \
			RELEASE_BUILD_DIR="$(MUSL_BUILD_DIR)" JOBS="$(JOBS)" \
			CMAKE_ARGS="-DWG_LINUX_LIBC=musl $(CMAKE_ARGS)"
	$(DOCKER) run --rm \
		--volume "$(CURDIR):/src:ro" \
		--workdir /src \
		$(DOCKER_RUN_ARGS) \
		"$(MUSL_BUILD_IMAGE)" \
		sh tools/WG_MUSL_AUDIT.sh "$(MUSL_DIST_DIR)"

musl-audit: musl-image
	$(DOCKER) run --rm \
		--volume "$(CURDIR):/src:ro" \
		--workdir /src \
		$(DOCKER_RUN_ARGS) \
		"$(MUSL_BUILD_IMAGE)" \
		sh tools/WG_MUSL_AUDIT.sh "$(MUSL_DIST_DIR)"

dist library release: library-release
portable portable-library: portable-library-release
musl musl-library: musl-library-release

clean: clean-release clean-portable clean-musl
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

clean-musl:
	@case "$(MUSL_BUILD_DIR)" in \
		build/*) ;; \
		*) printf '%s\n' \
			'Refusing to remove a musl build tree outside build/:' \
			'  $(MUSL_BUILD_DIR)'; exit 2 ;; \
	esac
	@case "$(MUSL_BUILD_DIR)" in \
		*..*) printf '%s\n' \
			'Refusing to remove a musl build tree containing ..:' \
			'  $(MUSL_BUILD_DIR)'; exit 2 ;; \
	esac
	$(CMAKE) -E remove_directory "$(MUSL_BUILD_DIR)"
	@if command -v "$(firstword $(DOCKER))" >/dev/null 2>&1 && \
	    $(DOCKER) image inspect "$(MUSL_BUILD_IMAGE)" >/dev/null 2>&1; then \
		$(DOCKER) image rm "$(MUSL_BUILD_IMAGE)"; \
	fi
