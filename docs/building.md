# Building wolf3dgeneric

All supported workflows use the root `CMakeLists.txt` as the authoritative
build graph. The Windows command line, Visual Studio solution, Linux presets,
and GNU Make entry point therefore compile the same sources with the same
warnings and packaging rules.

Generated compiler output belongs under `build/`. Clean redistributable
packages belong under `dist/`. Both directories are ignored by Git.

## Requirements

- CMake 3.16 or newer for direct and GNU Make builds. The optional checked-in
  CMake presets use schema version 2 and therefore require CMake 3.20.
- Windows: Visual Studio 2019 with the Desktop development with C++ workload,
  or a newer Visual Studio installation capable of using/upgrading v142
  projects.
- Linux: GNU Make plus GCC or Clang. Ninja is additionally required when using
  the Linux CMake presets directly. The direct-console host requires pkg-config,
  libdrm headers, and ALSA headers (`pkgconf libdrm-dev libasound2-dev` on
  Debian-family systems). SDL3 uses the normal X11 or Wayland development
  dependencies used by SDL on that distribution.
- SDL3 GUI builds use the pinned submodule by default; initialize it once with
  `make dependencies` or `git submodule update --init --recursive --
  third_party/SDL3`. Alternatively install SDL 3.2 or newer (for
  example `libsdl3-dev` on Debian 13) and select `USE_SYSTEM_SDL3=ON` under
  GNU Make or `WG_USE_SYSTEM_SDL3=ON` under CMake.
- Docker is required only for the optional Debian 10 portable SDL3 release
  target.
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

The Visual Studio solution and `windows-dev-*` presets also compile the SDL3
GUI target. Stage its minimal package independently with:

```text
cmake --preset windows-sdl3-x64
cmake --build --preset windows-sdl3-x64
```

Replace `x64` with `x86` as needed. The package contains
`wolf3dgeneric-sdl3.exe`, `wolf3dgeneric.dll`, `Nuked-OPL3.dll`, and
`SDL3.dll`. The selected static or dynamic MSVC runtime policy applies to all
locally compiled binaries.

Both Windows GUI hosts accept `--fullscreen` and toggle borderless desktop
fullscreen at runtime with F11 or Alt+Enter. F11 is host-only and cannot
acknowledge an engine "any key" wait. The SDL3 shortcuts and option behave the
same way on Linux desktop systems.

## Linux with GNU Make

Run `make help` for the complete target, variable, path, and example list.
The high-frequency commands all stage clean packages under `dist/`:

```text
make                         # shared-library package; the default
make console-release         # direct DRM/evdev/ALSA host
make sdl3-release            # SDL3 desktop host
```

Before the first pinned-SDL build, `make dependencies` fetches the exact
submodule revision recorded by the repository. SDL targets check for that
revision's checkout and print the recovery command when it is absent; they do
not silently access the network. `USE_SYSTEM_SDL3=ON` does not require the
submodule.

Each uses a compiler-specific intermediate tree under `build/`; those files
are an implementation detail of producing the requested package. `make
releases` stages all three when every optional dependency is installed.

Development and validation remain explicit:

```text
make build
make test
```

These write to `build/linux-gcc`. The development tree intentionally builds
the core/headless test surface without requiring the optional console or SDL3
development packages. Select Clang conventionally with `CC`:

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

Override the inferred development location when desired:

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

The release trees are separately configurable as `LIBRARY_BUILD_DIR`,
`CONSOLE_BUILD_DIR`, and `SDL3_BUILD_DIR`. `make clean`, `clean-library`,
`clean-console`, and `clean-sdl3` clean the corresponding tree without
deleting it. `make clean` also removes the container-generated portable SDL3
intermediate tree and
removes the named `wolf3dgeneric-build-debian10` Docker image when present;
`make clean-portable` performs only that portable cleanup. It deliberately
does not run a global Docker builder prune, which could discard cached layers
belonging to unrelated projects.

Create the Linux direct-console runtime package with:

```text
make console-release
```

This stages `wolf3dgeneric`, `libwolf3dgeneric.so`, the replaceable
`libNuked-OPL3.so`, notices, and runtime instructions under
`dist/wolf3dgeneric-<version>-linux-console-<architecture>`. libdrm, ALSA,
libc, and their transitive dependencies remain system-provided.

Build and stage the desktop SDL3 host with either maintained compiler:

```text
make sdl3-release
make sdl3-release CC=clang
make sdl3-release USE_SYSTEM_SDL3=ON
```

Compiler-specific objects stay in `build/linux-sdl3-gcc` or
`build/linux-sdl3-clang`. The clean package is staged under
`dist/wolf3dgeneric-<version>-sdl3-linux-<architecture>` with the executable
and local wolf3dgeneric, Nuked-OPL3, and SDL3 shared libraries. System mode
uses any compatible installed SDL 3.2-or-newer development package to build,
but still copies that SDL runtime into `dist/`; the destination machine does
not need an SDL development or runtime package installed.

### Pinned versus system SDL3

The pinned SDL 3.4.16 source remains the release default. It gives all hosts
the same reviewed SDL implementation, makes official artifacts reproducible,
and prevents a machine's installed SDL version from silently changing the
package. Its costs are the recursive/submodule checkout and extra SDL build
time.

`USE_SYSTEM_SDL3=ON` is a first-class local and distro-integration workflow.
It configures faster, uses the distribution's security/bug-fix stream, and
avoids compiling SDL. Its output depends on whichever compatible SDL and
build configuration that system supplied, so it is less reproducible and may
inherit a newer libc baseline. It can also inherit direct dependencies on the
distribution's X11, Wayland, audio, and graphics runtime libraries. Both modes
deliberately link SDL dynamically and copy its runtime shared object beside
the executable, but the pinned build is the better choice for a redistributable
package because its SDL configuration loads those optional backends dynamically.

### What “portable Linux package” means

The SDL3 folder is self-contained with respect to wolf3dgeneric, Nuked-OPL3,
and SDL3. Its executables use `$ORIGIN` runtime search paths, so those local
libraries are found after the folder is copied and SDL3 need not be installed
on the destination.

A system-SDL package may still require the destination to provide the same
backend libraries against which that distribution built SDL. This is normally
appropriate for a package intended for the same distribution release. The
pinned build has only the standard glibc-provided runtime libraries (`libc`,
`libm`, `libdl`, and `libpthread`) as direct external dependencies on the
validated Debian configuration and is therefore the preferred portable
tarball source.

It is not a universal Linux binary in the Windows sense. The destination must
have a compatible CPU architecture, ELF loader and glibc at least as new as
the build baseline, plus the ordinary desktop/kernel facilities used by SDL.
Building with the pinned SDL does not statically embed glibc or erase that
baseline. Building on an older supported distribution or a controlled
manylinux-style container broadens compatibility; an AppImage or comparable
container format would be the appropriate future step for a single artifact
targeting a wider set of distributions.

glibc normally preserves older symbol versions, so a binary linked on an old
baseline runs on newer glibc releases; the reverse is not true. The complexity
of wolf3dgeneric's libc use does not determine that floor—the linker's selected
symbol versions do. On the validated Debian 13 build, the SDL host requires
`GLIBC_2.34`, the engine `GLIBC_2.14`, Nuked-OPL3 `GLIBC_2.2.5`, and pinned SDL
`GLIBC_2.38`. SDL alone therefore sets the packaged floor.

For broadly redistributable tarballs, use the maintained Debian 10 container:

```text
make dependencies
make portable JOBS=8
```

The first command explicitly fetches the pinned SDL submodule if it is absent.
The second builds the digest-pinned `debian:10-slim` image, compiles all three
Linux distributions with GCC 8.3 against Debian 10's glibc 2.28, and runs an
ELF symbol-version audit on each. Docker's image and package layers are cached
after the first build. Build only one with `make portable-library`, `make
portable-console`, or `make portable-sdl3`; the corresponding explicit target
adds `-release` to each name. The `portable-` prefix intentionally groups the
container workflows together in help and shell completion.

The container installs a checksum-pinned modern CMake because CMake is a
build-time tool: this satisfies wolf3dgeneric and SDL's truthful CMake 3.16
minimum without changing the generated program's glibc ABI. It similarly
builds checksum-pinned Wayland 1.18 headers and `wayland-scanner` against
Debian 10, supplementing the distribution's older Wayland 1.16 development
package. SDL supplies its own protocol XML and dynamically loads the
destination's normal Wayland runtime libraries, so no Wayland shared library
is added to the staged package.

The validated packages have these actual highest symbol requirements:

| Distribution | Limiting ELF | Highest required glibc symbol |
| --- | --- | --- |
| shared library | `libwolf3dgeneric.so` | `GLIBC_2.14` |
| direct console | `wolf3dgeneric` | `GLIBC_2.17` |
| SDL3 | pinned `libSDL3.so.0` | `GLIBC_2.27` |

The SDL3 package as a whole therefore requires glibc 2.27 or newer, while the
other two have the lower floors shown above. The automated release gate
conservatively permits no symbol newer than the Debian 10 baseline of 2.28.
The SDL3 package was smoke-tested both inside the Debian 10 container and on
Debian 13. Its SDL build includes dynamically loaded native Wayland,
X11/XWayland, KMS/DRM, ALSA, and PulseAudio backends. A destination using the
native Wayland backend must provide its ordinary Wayland client, cursor, EGL,
and xkbcommon runtime libraries; SDL falls back to another available backend
when appropriate. The packaged game was also run with
`SDL_VIDEODRIVER=wayland` forced against a disposable Debian 13 headless
Weston compositor, verifying the native backend rather than an X11 fallback.

Use `make portable-glibc-audit` to repeat the symbol audit on all three staged
packages.
`PORTABLE_BUILD_IMAGE`, the `PORTABLE_*_BUILD_DIR` and
`PORTABLE_*_DIST_DIR` paths, `PORTABLE_GLIBC_MAX`, and `DOCKER_RUN_ARGS` are
available for maintainers; `make help` reports their normal usage. A full
container or sysroot is preferable to forcing old symbol names while linking
against a new host libc. Do not copy or statically bundle glibc beside the
application.

The direct-console package similarly leaves libc, libdrm, ALSA, and their
transitive libraries system-provided, which is conventional for that kind of
Linux host.

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

The SDL3 GUI package preset is:

```text
cmake --preset linux-sdl3
cmake --build --preset linux-sdl3
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
- `WG_TEST_WL6_GOG_PATH`
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
| `WG_BUILD_LINUX_CONSOLE` | `OFF` | Build the DRM/evdev/ALSA console wrapper. Forced off outside Linux. |
| `WG_BUILD_SDL3` | `OFF` | Build the SDL3 GUI wrapper. Enabled by SDL3 and Windows development presets. |
| `WG_USE_SYSTEM_SDL3` | `OFF` | Use installed SDL 3.2 or newer rather than the pinned submodule. |
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
