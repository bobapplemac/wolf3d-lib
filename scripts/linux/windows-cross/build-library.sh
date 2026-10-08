#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)
profile=${WG_WINDOWS_CROSS_PROFILE:?WG_WINDOWS_CROSS_PROFILE is required}
jobs=${WG_WINDOWS_CROSS_JOBS:-}
drivers=${WG_WINDOWS_CROSS_OPL_DRIVERS:-nuked,dbopl,silent}
default_driver=${WG_WINDOWS_CROSS_DEFAULT_OPL:-nuked}
sample_rate=${WG_WINDOWS_CROSS_SAMPLE_RATE:-48000}
version=$(sed -n '1p' "$root/VERSION")

case "$profile" in
    mingw-xp-x86)
        toolchain=mingw-gcc-i686.cmake
        compiler_label=mingw-gcc-xp
        nt_version=0x0501
        arch=x86
        ;;
    mingw-win7-x86)
        toolchain=mingw-gcc-i686.cmake
        compiler_label=mingw-gcc-win7
        nt_version=0x0601
        arch=x86
        ;;
    mingw-win7-x64)
        toolchain=mingw-gcc-x86_64.cmake
        compiler_label=mingw-gcc-win7
        nt_version=0x0601
        arch=x64
        ;;
    llvm-mingw-win7-x86)
        toolchain=llvm-mingw-i686.cmake
        compiler_label=llvm-mingw-msvcrt-win7
        nt_version=0x0601
        arch=x86
        ;;
    llvm-mingw-win7-x64)
        toolchain=llvm-mingw-x86_64.cmake
        compiler_label=llvm-mingw-msvcrt-win7
        nt_version=0x0601
        arch=x64
        ;;
    llvm-mingw-win10-x64)
        toolchain=llvm-mingw-x86_64.cmake
        compiler_label=llvm-mingw-ucrt-win10
        nt_version=0x0a00
        arch=x64
        ;;
    *)
        echo "Unknown Windows cross profile: $profile" >&2
        exit 2
        ;;
esac

build_dir=${WG_WINDOWS_CROSS_BUILD_DIR:-$root/build/windows-cross-$profile}
parallel_args=
if [ -n "$jobs" ]; then
    parallel_args="--parallel $jobs"
fi

cmake -S "$root" -B "$build_dir" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$root/cmake/toolchains/$toolchain" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="-D_WIN32_WINNT=$nt_version -DWINVER=$nt_version" \
    -DBUILD_TESTING=OFF -DWG_BUILD_HEADLESS=OFF \
    -DWG_WARNINGS_AS_ERRORS=ON -DWG_STATIC_GNU_RUNTIME=ON \
    -DWG_ENABLE_OPL_NUKED=$(case ",$drivers," in *,nuked,*) echo ON;; *) echo OFF;; esac) \
    -DWG_ENABLE_OPL_DBOPL=$(case ",$drivers," in *,dbopl,*) echo ON;; *) echo OFF;; esac) \
    -DWG_ENABLE_OPL_SILENT=$(case ",$drivers," in *,silent,*) echo ON;; *) echo OFF;; esac) \
    -DWG_DEFAULT_OPL_DRIVER="$default_driver" \
    -DWG_DEFAULT_SAMPLE_RATE="$sample_rate" \
    -DWG_COMPILER_LABEL="$compiler_label" \
    -DWG_DIST_ROOT="$root/dist"

# shellcheck disable=SC2086
cmake --build "$build_dir" --target library-release $parallel_args

dist_dir="$root/dist/wolf3d-$version-library-windows-$arch-$compiler_label"
sh "$root/tools/WG_WINDOWS_PE_AUDIT.sh" "$profile" "$dist_dir"
