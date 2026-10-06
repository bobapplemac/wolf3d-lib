# wolf3d-lib

`wolf3d-lib` is a portable shared-library adaptation of the original
Wolfenstein 3D v1.4 source. It preserves the original gameplay, fixed-point
math, data formats, 70 Hz logical timing, rendering, and audio behavior while
placing operating-system services behind a compact C99 callback API.

This repository owns the engine only. Ready-to-run Win32, SDL3, and Linux
direct-console applications live in the companion
[`wolf3d-portable`](https://gitlab.moorenet.xyz/personal/wolf3d-portable)
repository. Every portable-host build compiles the exact pinned revision of
this library.

Original Wolfenstein 3D or Spear of Destiny game data is required and is not
distributed here.

## Shareware data

The freely distributed shareware/demo data sets are a convenient way to try a
host built against this library or to supply external regression data:

- [Wolfenstein 3D v1.4 shareware data (`WL1`)](https://download.sourceforge.net/wolfgl/wolfdata.zip)
  - SHA-256: `A32EE97C515B6E182597A06F2326D15CC4C343DDC70558CE5FE76C870B7A0027`
- [Spear of Destiny demo data (`SDM`)](https://download.sourceforge.net/wolfgl/sdmdata.zip)
  - SHA-256: `054590923CD35CE7C0BFAE98C23BE81AB70C28E11FD0E562B5253523FCD7B91F`

Both ZIPs contain only the eight corresponding game-data files. They are
linked from the archived [WolfGL download page](https://wolfgl.sourceforge.net/files.htm)
and are not redistributed by this project. Registered `WL6`, `SOD`, `SD1`,
`SD2`, and `SD3` data must still come from a legitimately obtained game copy.

## Public contract

Hosts include `WOLF3D.h`, fill a `wolf3d_platform_api_t`, then call:

1. `wolf3d_SetPlatform()`
2. `wolf3d_Create()`
3. `wolf3d_Run()`
4. `wolf3d_Shutdown()`

The library exports only those four functions plus the indexed 320x200
`wolf3d_ScreenBuffer` and 256-color `wolf3d_Palette`. The reference Nuked-OPL3
backend remains a separate, replaceable LGPL shared library; pure-C DBOPL and
timing-preserving silent builds are also available.

The in-tree `wolf3d::wolf3d` CMake target is the supported target for a
parent project. The public header is under `include/`; private `src/`
headers are not a host interface.

## Build

On Linux, run the dependency-free guided configurator and choose from the
detected native, portable-glibc, and musl build paths:

```sh
./build.sh
```

Explicit arguments bypass the wizard and are forwarded to GNU Make, so the
established automation interface remains available:

```sh
make                    # native shared-library package
make test               # library, internal headless oracle, and tests
make test CC=clang      # same validation with Clang
make portable JOBS=8    # Debian 10 / glibc 2.28 package in Docker
make musl JOBS=8        # Alpine/musl package in Docker
make test OPL_BACKEND=dbopl
make test AUDIO_BACKEND=silent
make help               # complete command and variable reference
```

CMake can be used directly:

```sh
cmake --preset linux-dev
cmake --build --preset linux-dev
ctest --preset linux-dev
```

On Windows, open `ide/visual-studio/vsYYYY/wolf3d-lib.sln` in the matching
Visual Studio generation (or `ide/visual-studio/vc6/wolf3d-lib.dsw` in VC6),
or use CMake:

```powershell
cmake --preset windows-dev-x64
cmake --build --preset windows-dev-x64 --config Release
ctest --preset windows-dev-x64 -C Release
```

The root `build.ps1` is the guided entry point and detects supported
Visual Studio installations from VS2008/v90 through VS2022/v143 and MSYS2
UCRT64 MinGW. Run it with no arguments for an interactive wizard:

```powershell
.\build.ps1 -List
.\build.ps1
.\build.ps1 -Compiler vs2019 -Architecture x86 -Action test
.\build.ps1 -Compiler vs2008 -Architecture x86 -Action test
.\build.ps1 -Compiler mingw-ucrt64 -Action package
.\build.ps1 -Action package -Opl dbopl -Runtime dynamic
```

Windows XP-era x86 builds use an isolated CMake 3.5 definition and native CMD
dispatcher, preserving compatibility with VC6 SP6 through VS2005 SP1:

```bat
build.cmd
scripts\windows\legacy\build.cmd vc6 Release standard nuked static test
scripts\windows\legacy\build.cmd vs2002 Release standard nuked static package
scripts\windows\legacy\build.cmd vs2003
scripts\windows\legacy\build.cmd vs2005
```

Release presets stage clean SDK folders containing `wolf3d.dll` and the
compiler-appropriate import library on Windows, or the versioned
`libwolf3d.so` SONAME chain on Linux, together with `WOLF3D.h` (and the
legacy-MSVC compatibility header where
applicable), the selected audio backend, licenses, and package notes.
See the [build and compatibility matrix](docs/support-matrix.md) for supported
compilers, build entry points, artifacts, and validated destination operating
systems. [docs/building.md](docs/building.md) contains the complete command and
option reference.

## Validation host

`platforms/headless/` is an internal deterministic oracle, not a production
frontend. It supports frame, palette, audio, demo, archive, and gameplay
regression checks against externally supplied data. The full suite is defined
in CMake and is enabled by setting the documented `WG_TEST_*` data paths.

## Design goals

- Keep original `WL_*` and `ID_*` filenames and structure where practical.
- Use `WG_*` only for genuinely new portable support code.
- Preserve original behavior instead of adding modern gameplay enhancements.
- Make host boundaries explicit and prevent wrappers from depending on private
  engine headers.
- Keep deterministic arithmetic, random-number generation, tic batching, and
  demo playback suitable for preservation-grade comparison.

Architecture, provenance, supported-data fingerprints, and porting guidance
are collected under [docs/](docs/).
