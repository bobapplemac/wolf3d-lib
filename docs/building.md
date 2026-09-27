# Building wolf3dgeneric

All supported workflows use the root `CMakeLists.txt` as the authoritative
build graph. The Windows command line, Visual Studio solution, Linux presets,
and GNU Make entry point therefore compile the same sources with the same
warnings and packaging rules.

Generated compiler output belongs under `build/`. Clean redistributable
packages belong under `dist/`. Both directories are ignored by Git.

## Requirements

- CMake 3.20 or newer.
- Windows: Visual Studio 2019 with the Desktop development with C++ workload,
  or a newer Visual Studio installation capable of using/upgrading v142
  projects.
- Linux: GNU Make plus GCC or Clang. Ninja is additionally required when using
  the Linux CMake presets directly. The direct-console host requires pkg-config,
  libdrm headers, and ALSA headers (`pkgconf libdrm-dev libasound2-dev` on
  Debian-family systems).
- Original Wolfenstein 3D data is optional for compilation and required only
  for the asset-backed regression tests or normal gameplay.

## Windows command line

The checked-in presets generate Visual Studio build trees. Build and test x64
Release with:

```text
cmake --preset windows-dev-x64
cmake --build --preset windows-dev-x64
ctest --preset windows-dev-x64
```

Replace `x64` with `x86` for a 32-bit build. Development outputs are written
to `build/windows-dev-x64` or `build/windows-dev-x86`.

To select Debug explicitly, configure with the development preset and invoke
the multi-configuration tree directly:

```text
cmake --preset windows-dev-x64
cmake --build build/windows-dev-x64 --config Debug
ctest --test-dir build/windows-dev-x64 -C Debug --output-on-failure
```

## Visual Studio

Open the checked-in `wolf3dgeneric.sln` at the repository root. Choose:

- `Debug` or `Release` for the recommended embedded MSVC runtime (`/MT`).
- `Debug Dynamic CRT` or `Release Dynamic CRT` for the dynamic runtime (`/MD`).
- `Win32` or `x64` as the platform.

Building the `wolf3dgeneric` project configures and invokes the corresponding
CMake development tree. The IDE and CLI therefore share these output folders:

| Visual Studio platform | Build folder |
| --- | --- |
| `Win32` | `build/windows-dev-x86` |
| `x64` | `build/windows-dev-x64` |

Dynamic-CRT configurations use the corresponding folder with `-dynamic-crt`
appended. The engine remains a separate DLL in every configuration.

The project lists the portable engine, both supplied hosts, tests, and
Nuked-OPL3 sources for browsing. Its debugger command points to the selected
configuration's `wolf3dgeneric-win32.exe`. Original game data is not copied
into a development tree automatically; configure the program arguments with
`--data` in the project's Debugging properties when launching from the IDE.

## Windows runtime packages

Create the minimal Win32 runtime folder with:

```text
cmake --preset windows-release-x64
cmake --build --preset windows-release-x64
```

Replace `x64` with `x86` as required. Output is staged as:

```text
dist/wolf3dgeneric-<version>-win32-x64
dist/wolf3dgeneric-<version>-win32-x86
```

Create a host-independent engine-library package with the
`windows-library-x64` or `windows-library-x86` configure and build presets.

The default `WG_STATIC_MSVC_RUNTIME=ON` uses `/MT`, so official packages do not
require the Visual C++ Redistributable on the destination machine. Set it to
`OFF` for `/MD`; those binaries remain separate DLLs but require the matching
Redistributable to be installed. Their build and package folders add
`-dynamic-crt`, allowing both runtime policies to coexist.

For example, stage the x64 `/MD` runtime package with:

```text
cmake --preset windows-release-x64 -B build/windows-release-x64-dynamic-crt -DWG_STATIC_MSVC_RUNTIME=OFF
cmake --build build/windows-release-x64-dynamic-crt --config Release --target win32-release
```

The result is `dist/wolf3dgeneric-<version>-win32-x64-dynamic-crt`.

## Linux with GNU Make

GCC is the default compiler:

```text
make
make test
```

This writes to `build/linux-gcc`. Select Clang conventionally with
`CC`:

```text
make CC=clang
make test CC=clang
```

This writes to `build/linux-clang`. Each compiler uses a distinct build tree
because CMake does not support changing the compiler in an existing tree.
An explicitly named or versioned compiler also works; its executable name is
used in the default build-folder name:

```text
make CC=gcc-14
make CC=clang-19
```

Override the inferred location when desired:

```text
make CC=clang BUILD_DIR=build/my-clang-tree BUILD_TYPE=Debug
```

The maintained Linux compiler set is GCC and Clang. The engine targets ISO C99
and has no compiler-specific gameplay path, so another CMake-supported C
compiler can be tried with `CC`, but it is not considered supported until its
strict build and test suite pass.

Create the Linux shared-library package with:

```text
make library-release
```

or `make library-release CC=clang`. Both compilers stage the same portable
result under `dist/wolf3dgeneric-<version>-library-linux-<architecture>`;
staging one replaces the previous package for that platform. Compiler names
belong in `build/` because those trees contain compiler-specific objects and
caches. They are intentionally omitted from `dist/`: the staged library uses
the platform C ABI, and `dist/` names describe consumer compatibility rather
than the tool used to produce it.

`make clean` cleans the selected compiler tree without deleting the tree.

Create the Linux direct-console runtime package with:

```text
make linux-console-release
```

This stages `wolf3dgeneric`, `libwolf3dgeneric.so`, the replaceable
`libNuked-OPL3.so`, notices, and runtime instructions under
`dist/wolf3dgeneric-<version>-linux-console-<architecture>`. libdrm, ALSA,
libc, and their transitive dependencies remain system-provided.

## Linux with CMake and Ninja

The direct preset workflow remains available:

```text
cmake --preset linux-dev
cmake --build --preset linux-dev
ctest --preset linux-dev
```

It writes to `build/linux-dev`. The preset uses the system's default C
compiler. Use GNU Make when choosing GCC and Clang explicitly, or configure a
separate CMake tree with `-DCMAKE_C_COMPILER=<compiler>`.

The host-free package preset is:

```text
cmake --preset linux-library
cmake --build --preset linux-library
```

The direct-console package preset is:

```text
cmake --preset linux-console
cmake --build --preset linux-console
```

The console host runs directly from an active Linux virtual terminal. It does
not use X11, Wayland, or SDL. By default it discovers a connected
`/dev/dri/card*`, usable `/dev/input/event*` devices, and the ALSA `default`
PCM. Run `wolf3dgeneric-linux-console --linux-console-help` for explicit DRM,
input, and ALSA selection. Missing audio degrades to silent operation; missing
DRM output or a keyboard is a startup error.

## Asset-backed tests

Compilation and the self-contained tests do not require proprietary game
files. To enable a supported external corpus, pass its directory during CMake
configuration. Through GNU Make, use `CMAKE_ARGS`:

```text
make test CMAKE_ARGS="-DWG_TEST_WL1_PATH=/data/WL1 -DWG_TEST_WL6_PATH=/data/WL6"
```

Available cache variables are:

- `WG_TEST_WL1_PATH`
- `WG_TEST_WL6_PATH`
- `WG_TEST_WL6_APOGEE_PATH`
- `WG_TEST_SOD_PATH`
- `WG_TEST_SDM_PATH`
- `WG_TEST_SD1_PATH`
- `WG_TEST_SD2_PATH`
- `WG_TEST_SD3_PATH`

Paths may likewise be supplied after a Windows preset, for example:

```text
cmake --preset windows-dev-x64 -DWG_TEST_WL1_PATH=C:\games\WL1
```

The game data remains external and must never be committed or packaged.

## Useful CMake options

| Option | Default | Purpose |
| --- | --- | --- |
| `WG_BUILD_HEADLESS` | `ON` | Build the deterministic headless host and test support. |
| `WG_BUILD_WIN32` | `ON` on Windows | Build the native Win32 wrapper. Forced off elsewhere. |
| `WG_BUILD_LINUX_CONSOLE` | `ON` on Linux | Build the DRM/evdev/ALSA console wrapper. Forced off elsewhere. |
| `WG_WARNINGS_AS_ERRORS` | `ON` in presets/Makefile | Treat the maintained warning set as errors. |
| `WG_STATIC_MSVC_RUNTIME` | `ON` | Use `/MT` instead of the Redistributable-backed `/MD`; Windows/MSVC only. |
| `BUILD_TESTING` | `ON` | Generate the CTest suite. |

Pass additional settings with `CMAKE_ARGS` under GNU Make or as `-D` arguments
to the initial CMake configure command.

## C-runtime linkage

The wolf3dgeneric engine always remains a separate shared library:
`wolf3dgeneric.dll` on Windows and `libwolf3dgeneric.so` on Linux.

MSVC supports a meaningful deployment choice. `WG_STATIC_MSVC_RUNTIME=ON` is
the recommended default and links `/MT`, avoiding a separate Visual C++
Redistributable installation. `OFF` links `/MD`, reducing duplicated runtime
code when an application uses many MSVC DLLs but requiring the matching
Redistributable. Use a separate `-dynamic-crt` build tree when comparing them.

Linux shared objects conventionally resolve libc from the host. Folding glibc
into a `.so` is not a supported portability strategy and can duplicate process
state, break NSS/dynamic-loading behavior, and introduce ABI problems. Linux
portability is instead checked with GCC and Clang; release binaries should be
built against an appropriately old libc baseline (or a chosen alternate libc)
for the distributions they intend to support.
