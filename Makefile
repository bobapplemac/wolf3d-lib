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
OPL_DRIVERS ?= nuked,dbopl,silent
OPL_DEFAULT ?= nuked
SAMPLE_RATE ?= 48000
CMAKE_ARGS ?=
TEST_ARGS ?=
JOBS ?=

comma := ,

BACKEND_SUFFIX :=

AUDIO_SUFFIX :=
ifneq ("$(OPL_DRIVERS)","nuked,dbopl,silent")
AUDIO_SUFFIX := $(AUDIO_SUFFIX)-opl-$(subst $(comma),-,$(OPL_DRIVERS))
endif
ifneq ("$(OPL_DEFAULT)","nuked")
AUDIO_SUFFIX := $(AUDIO_SUFFIX)-default-$(OPL_DEFAULT)
endif
ifneq ("$(SAMPLE_RATE)","48000")
AUDIO_SUFFIX := $(AUDIO_SUFFIX)-$(SAMPLE_RATE)hz
endif

BUILD_DIR ?= build/linux-$(COMPILER_NAME)$(BACKEND_SUFFIX)
RELEASE_BUILD_DIR ?= build/linux-library-$(COMPILER_NAME)$(BACKEND_SUFFIX)
PORTABLE_BUILD_DIR ?= build/linux-library-portable-debian10-gcc
PORTABLE_DIST_DIR ?= dist/wolf3d-$(PROJECT_VERSION)-library-linux-x64$(BACKEND_SUFFIX)$(AUDIO_SUFFIX)
PORTABLE_BUILD_IMAGE ?= wolf3d-lib-build-debian10
PORTABLE_GLIBC_MAX ?= 2.28
MUSL_BUILD_DIR ?= build/linux-library-musl-$(COMPILER_NAME)$(BACKEND_SUFFIX)
MUSL_DIST_DIR ?= dist/wolf3d-$(PROJECT_VERSION)-library-linux-musl-x64$(BACKEND_SUFFIX)$(AUDIO_SUFFIX)
MUSL_BUILD_IMAGE ?= wolf3d-lib-build-alpine-musl
OPENWATCOM_BUILD_DIR ?= build/openwatcom-dos32
OPENWATCOM_DIST_DIR ?= dist/wolf3d-$(PROJECT_VERSION)-library-dos32-openwatcom-x86
OPENWATCOM_BUILD_IMAGE ?= wolf3d-lib-build-openwatcom-20261001
OPENWATCOM_OPL_DRIVERS ?= nuked,dbopl,silent
OPENWATCOM_DEFAULT_OPL ?= nuked
OPENWATCOM_SAMPLE_RATE ?= 44100
WINDOWS_MINGW_BUILD_IMAGE ?= wolf3d-lib-build-windows-mingw-debian12
WINDOWS_LLVM_MINGW_MSVC_IMAGE ?= wolf3d-lib-build-llvm-mingw-20260908-msvcrt
WINDOWS_LLVM_MINGW_UCRT_IMAGE ?= wolf3d-lib-build-llvm-mingw-20260908-ucrt
WINDOWS_LLVM_MINGW_RELEASE ?= 20260908
WINDOWS_LLVM_MINGW_MSVC_SHA256 ?= 4d905bae713182f1a2b4d33875fe5aa544ce9fc04cc153acb47755a90ca62f16
WINDOWS_LLVM_MINGW_UCRT_SHA256 ?= 2258c745e3155870c80793f3e8c80b28fbde11b9ff73c4c78783635b3440b092
WINDOWS_OPENWATCOM_BUILD_DIR ?= build/openwatcom-win9x-x86
WINDOWS_OPENWATCOM_DIST_DIR ?= dist/wolf3d-$(PROJECT_VERSION)-library-windows-x86-openwatcom-win9x
DOCKER_RUN_ARGS ?=

CMAKE_COMPILER_ARG := -DCMAKE_C_COMPILER="$(CC)"
CMAKE_BACKEND_ARGS := \
	-DWG_ENABLE_OPL_NUKED=$(if $(findstring nuked,$(OPL_DRIVERS)),ON,OFF) \
	-DWG_ENABLE_OPL_DBOPL=$(if $(findstring dbopl,$(OPL_DRIVERS)),ON,OFF) \
	-DWG_ENABLE_OPL_SILENT=$(if $(findstring silent,$(OPL_DRIVERS)),ON,OFF) \
	-DWG_DEFAULT_OPL_DRIVER="$(OPL_DEFAULT)" \
	-DWG_DEFAULT_SAMPLE_RATE="$(SAMPLE_RATE)"
PARALLEL_ARG := --parallel $(JOBS)

.DEFAULT_GOAL := all

.PHONY: all help check-version configure build test dist library \
	library-release release portable portable-library \
	portable-library-release portable-image portable-glibc-audit \
	musl musl-library musl-library-release musl-image musl-audit \
	openwatcom openwatcom-library-release openwatcom-image \
	windows-cross windows-win9x windows-win9x-library-release \
	windows-xp windows-xp-library-release windows-win7 \
	windows-win7-library-release windows-llvm-win7 \
	windows-llvm-win7-library-release windows-win10 \
	windows-win10-library-release windows-mingw-image \
	windows-llvm-msvcrt-image windows-llvm-ucrt-image \
	print-config clean clean-release clean-portable clean-musl clean-openwatcom \
	clean-windows-cross

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
		'  make openwatcom              Build the 32-bit DOS Open Watcom library SDK.' \
		'  make windows-win9x          Cross-build the Open Watcom Win9x x86 DLL SDK.' \
		'  make windows-xp             Cross-build the MinGW/MSVCRT XP x86 DLL SDK.' \
		'  make windows-win7           Cross-build MinGW/MSVCRT Win7 x86 and x64 SDKs.' \
		'  make windows-llvm-win7      Cross-build independent LLVM/MSVCRT Win7 SDKs.' \
		'  make windows-win10          Cross-build the LLVM/UCRT Win10 x64 DLL SDK.' \
		'  make windows-cross          Build every Linux-hosted Windows DLL SDK.' \
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
		'  make clean-openwatcom        Clean the DOS32 tree and builder image.' \
		'  make clean-windows-cross     Clean Windows cross-build trees and images.' \
		'' \
		'Common variables:' \
		'  CC=gcc|clang                 C compiler (default: gcc).' \
		'  BUILD_TYPE=Release|Debug     CMake build type (default: Release).' \
		'  OPL_DRIVERS=nuked,dbopl,silent Drivers compiled into the library (default: all).' \
		'  OPL_DEFAULT=nuked|dbopl|silent Runtime default (default: nuked).' \
		'  SAMPLE_RATE=Hz               Preferred host PCM rate (default: 48000).' \
		'  JOBS=N                       Parallel job limit.' \
		'  CMAKE_ARGS="..."             Extra CMake -D settings.' \
		'  TEST_ARGS="..."              Extra arguments passed to CTest.' \
		'  DOCKER_RUN_ARGS="..."        Extra docker-run options.' \
		'  OPENWATCOM_OPL_DRIVERS=...   DOS drivers to compile (default: all).' \
		'  OPENWATCOM_DEFAULT_OPL=...   DOS runtime default (default: nuked).' \
		'  OPENWATCOM_SAMPLE_RATE=Hz    DOS preferred PCM rate (default: 44100).' \
		'' \
		'Examples:' \
		'  make' \
		'  make test CC=clang JOBS=8' \
		'  make test OPL_DEFAULT=dbopl' \
		'  make test OPL_DRIVERS=silent OPL_DEFAULT=silent' \
		'  make portable JOBS=8' \
		'  make musl OPL_DEFAULT=dbopl JOBS=8' \
		'  make openwatcom' \
		'  make openwatcom OPENWATCOM_OPL_DRIVERS=silent OPENWATCOM_DEFAULT_OPL=silent'

print-config:
	@printf '%s\n' \
		'CC=$(CC)' \
		'PROJECT_VERSION=$(PROJECT_VERSION)' \
		'BUILD_TYPE=$(BUILD_TYPE)' \
		'OPL_DRIVERS=$(OPL_DRIVERS)' \
		'OPL_DEFAULT=$(OPL_DEFAULT)' \
		'SAMPLE_RATE=$(SAMPLE_RATE)' \
		'BUILD_DIR=$(BUILD_DIR)' \
		'RELEASE_BUILD_DIR=$(RELEASE_BUILD_DIR)' \
		'PORTABLE_BUILD_DIR=$(PORTABLE_BUILD_DIR)' \
		'PORTABLE_DIST_DIR=$(PORTABLE_DIST_DIR)' \
		'PORTABLE_BUILD_IMAGE=$(PORTABLE_BUILD_IMAGE)' \
		'PORTABLE_GLIBC_MAX=$(PORTABLE_GLIBC_MAX)' \
		'MUSL_BUILD_DIR=$(MUSL_BUILD_DIR)' \
		'MUSL_DIST_DIR=$(MUSL_DIST_DIR)' \
		'MUSL_BUILD_IMAGE=$(MUSL_BUILD_IMAGE)' \
		'OPENWATCOM_BUILD_DIR=$(OPENWATCOM_BUILD_DIR)' \
		'OPENWATCOM_DIST_DIR=$(OPENWATCOM_DIST_DIR)' \
		'OPENWATCOM_BUILD_IMAGE=$(OPENWATCOM_BUILD_IMAGE)' \
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
			OPL_DRIVERS="$(OPL_DRIVERS)" OPL_DEFAULT="$(OPL_DEFAULT)" \
			SAMPLE_RATE="$(SAMPLE_RATE)" \
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
			OPL_DRIVERS="$(OPL_DRIVERS)" OPL_DEFAULT="$(OPL_DEFAULT)" \
			SAMPLE_RATE="$(SAMPLE_RATE)" \
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

openwatcom-image:
	$(DOCKER) build --tag "$(OPENWATCOM_BUILD_IMAGE)" packaging/openwatcom

openwatcom-library-release: openwatcom-image
	$(DOCKER) run --rm \
		--user "$$(id -u):$$(id -g)" \
		--volume "$(CURDIR):/src" \
		--workdir /src \
		$(DOCKER_RUN_ARGS) \
		--env WG_OPENWATCOM_BUILD_DIR="/src/$(OPENWATCOM_BUILD_DIR)" \
		--env WG_OPENWATCOM_DIST_DIR="/src/$(OPENWATCOM_DIST_DIR)" \
		--env WG_OPENWATCOM_OPL_DRIVERS="$(OPENWATCOM_OPL_DRIVERS)" \
		--env WG_OPENWATCOM_DEFAULT_OPL="$(OPENWATCOM_DEFAULT_OPL)" \
		--env WG_OPENWATCOM_SAMPLE_RATE="$(OPENWATCOM_SAMPLE_RATE)" \
		"$(OPENWATCOM_BUILD_IMAGE)" \
		sh scripts/linux/openwatcom/build-library.sh

windows-mingw-image:
	$(DOCKER) build --tag "$(WINDOWS_MINGW_BUILD_IMAGE)" packaging/windows-mingw

windows-llvm-msvcrt-image:
	$(DOCKER) build --tag "$(WINDOWS_LLVM_MINGW_MSVC_IMAGE)" \
		--build-arg LLVM_MINGW_RELEASE="$(WINDOWS_LLVM_MINGW_RELEASE)" \
		--build-arg LLVM_MINGW_CRT=msvcrt \
		--build-arg LLVM_MINGW_SHA256="$(WINDOWS_LLVM_MINGW_MSVC_SHA256)" \
		packaging/windows-llvm-mingw

windows-llvm-ucrt-image:
	$(DOCKER) build --tag "$(WINDOWS_LLVM_MINGW_UCRT_IMAGE)" \
		--build-arg LLVM_MINGW_RELEASE="$(WINDOWS_LLVM_MINGW_RELEASE)" \
		--build-arg LLVM_MINGW_CRT=ucrt \
		--build-arg LLVM_MINGW_SHA256="$(WINDOWS_LLVM_MINGW_UCRT_SHA256)" \
		packaging/windows-llvm-mingw

windows-win9x-library-release: openwatcom-image
	$(DOCKER) run --rm --user "$$(id -u):$$(id -g)" \
		--volume "$(CURDIR):/src" --workdir /src $(DOCKER_RUN_ARGS) \
		--env WG_OPENWATCOM_WINDOWS_BUILD_DIR="/src/$(WINDOWS_OPENWATCOM_BUILD_DIR)" \
		--env WG_OPENWATCOM_WINDOWS_DIST_DIR="/src/$(WINDOWS_OPENWATCOM_DIST_DIR)" \
		--env WG_OPENWATCOM_WINDOWS_OPL_DRIVERS="$(OPL_DRIVERS)" \
		--env WG_OPENWATCOM_WINDOWS_DEFAULT_OPL="$(OPL_DEFAULT)" \
		--env WG_OPENWATCOM_WINDOWS_SAMPLE_RATE="$(SAMPLE_RATE)" \
		"$(OPENWATCOM_BUILD_IMAGE)" \
		sh scripts/linux/openwatcom/build-windows-library.sh

windows-xp-library-release: windows-mingw-image
	$(DOCKER) run --rm --user "$$(id -u):$$(id -g)" \
		--volume "$(CURDIR):/src" --workdir /src $(DOCKER_RUN_ARGS) \
		--env WG_WINDOWS_CROSS_PROFILE=mingw-xp-x86 \
		--env WG_WINDOWS_CROSS_JOBS="$(JOBS)" \
		--env WG_WINDOWS_CROSS_OPL_DRIVERS="$(OPL_DRIVERS)" \
		--env WG_WINDOWS_CROSS_DEFAULT_OPL="$(OPL_DEFAULT)" \
		--env WG_WINDOWS_CROSS_SAMPLE_RATE="$(SAMPLE_RATE)" \
		"$(WINDOWS_MINGW_BUILD_IMAGE)" \
		sh scripts/linux/windows-cross/build-library.sh

windows-win7-library-release: windows-mingw-image
	@for profile in mingw-win7-x86 mingw-win7-x64; do \
		$(DOCKER) run --rm --user "$$(id -u):$$(id -g)" \
			--volume "$(CURDIR):/src" --workdir /src $(DOCKER_RUN_ARGS) \
			--env WG_WINDOWS_CROSS_PROFILE="$$profile" \
			--env WG_WINDOWS_CROSS_JOBS="$(JOBS)" \
			--env WG_WINDOWS_CROSS_OPL_DRIVERS="$(OPL_DRIVERS)" \
			--env WG_WINDOWS_CROSS_DEFAULT_OPL="$(OPL_DEFAULT)" \
			--env WG_WINDOWS_CROSS_SAMPLE_RATE="$(SAMPLE_RATE)" \
			"$(WINDOWS_MINGW_BUILD_IMAGE)" \
			sh scripts/linux/windows-cross/build-library.sh || exit $$?; \
	done

windows-llvm-win7-library-release: windows-llvm-msvcrt-image
	@for profile in llvm-mingw-win7-x86 llvm-mingw-win7-x64; do \
		$(DOCKER) run --rm --user "$$(id -u):$$(id -g)" \
			--volume "$(CURDIR):/src" --workdir /src $(DOCKER_RUN_ARGS) \
			--env WG_WINDOWS_CROSS_PROFILE="$$profile" \
			--env WG_WINDOWS_CROSS_JOBS="$(JOBS)" \
			--env WG_WINDOWS_CROSS_OPL_DRIVERS="$(OPL_DRIVERS)" \
			--env WG_WINDOWS_CROSS_DEFAULT_OPL="$(OPL_DEFAULT)" \
			--env WG_WINDOWS_CROSS_SAMPLE_RATE="$(SAMPLE_RATE)" \
			"$(WINDOWS_LLVM_MINGW_MSVC_IMAGE)" \
			sh scripts/linux/windows-cross/build-library.sh || exit $$?; \
	done

windows-win10-library-release: windows-llvm-ucrt-image
	$(DOCKER) run --rm --user "$$(id -u):$$(id -g)" \
		--volume "$(CURDIR):/src" --workdir /src $(DOCKER_RUN_ARGS) \
		--env WG_WINDOWS_CROSS_PROFILE=llvm-mingw-win10-x64 \
		--env WG_WINDOWS_CROSS_JOBS="$(JOBS)" \
		--env WG_WINDOWS_CROSS_OPL_DRIVERS="$(OPL_DRIVERS)" \
		--env WG_WINDOWS_CROSS_DEFAULT_OPL="$(OPL_DEFAULT)" \
		--env WG_WINDOWS_CROSS_SAMPLE_RATE="$(SAMPLE_RATE)" \
		"$(WINDOWS_LLVM_MINGW_UCRT_IMAGE)" \
		sh scripts/linux/windows-cross/build-library.sh

dist library release: library-release
portable portable-library: portable-library-release
musl musl-library: musl-library-release
openwatcom: openwatcom-library-release
windows-win9x: windows-win9x-library-release
windows-xp: windows-xp-library-release
windows-win7: windows-win7-library-release
windows-llvm-win7: windows-llvm-win7-library-release
windows-win10: windows-win10-library-release
windows-cross: windows-win9x-library-release windows-xp-library-release \
	windows-win7-library-release windows-llvm-win7-library-release \
	windows-win10-library-release

clean: clean-release clean-portable clean-musl clean-openwatcom clean-windows-cross
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

clean-openwatcom:
	@case "$(OPENWATCOM_BUILD_DIR)" in \
		build/*) ;; \
		*) printf '%s\n' \
			'Refusing to remove an Open Watcom build tree outside build/:' \
			'  $(OPENWATCOM_BUILD_DIR)'; exit 2 ;; \
	esac
	$(CMAKE) -E remove_directory "$(OPENWATCOM_BUILD_DIR)"
	@if command -v "$(firstword $(DOCKER))" >/dev/null 2>&1 && \
	    $(DOCKER) image inspect "$(OPENWATCOM_BUILD_IMAGE)" >/dev/null 2>&1; then \
		$(DOCKER) image rm "$(OPENWATCOM_BUILD_IMAGE)"; \
	fi

clean-windows-cross:
	$(CMAKE) -E remove_directory "$(WINDOWS_OPENWATCOM_BUILD_DIR)"
	$(CMAKE) -E remove_directory build/windows-cross-mingw-xp-x86
	$(CMAKE) -E remove_directory build/windows-cross-mingw-win7-x86
	$(CMAKE) -E remove_directory build/windows-cross-mingw-win7-x64
	$(CMAKE) -E remove_directory build/windows-cross-llvm-mingw-win7-x86
	$(CMAKE) -E remove_directory build/windows-cross-llvm-mingw-win7-x64
	$(CMAKE) -E remove_directory build/windows-cross-llvm-mingw-win10-x64
	@if command -v "$(firstword $(DOCKER))" >/dev/null 2>&1; then \
		$(DOCKER) image rm "$(WINDOWS_MINGW_BUILD_IMAGE)" >/dev/null 2>&1 || true; \
		$(DOCKER) image rm "$(WINDOWS_LLVM_MINGW_MSVC_IMAGE)" >/dev/null 2>&1 || true; \
		$(DOCKER) image rm "$(WINDOWS_LLVM_MINGW_UCRT_IMAGE)" >/dev/null 2>&1 || true; \
	fi
