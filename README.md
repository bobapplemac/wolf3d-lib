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

## Public contract

Hosts include `WOLF3D.h`, fill a `wolf3d_platform_api_t`, then call:

1. `wolf3d_SetPlatform()`
2. `wolf3d_Create()`
3. `wolf3d_Run()`
4. `wolf3d_Shutdown()`

The library exports only those four functions plus the indexed 320x200
`wolf3d_ScreenBuffer` and 256-color `wolf3d_Palette`. Nuked-OPL3 remains a
separate, replaceable LGPL shared library.

The in-tree `wolf3d::wolf3d` CMake target is the supported target for a
parent project. The public header is under `include/`; private `src/`
headers are not a host interface.

## Build

On Linux, GNU Make provides the common paths:

```sh
make                    # native shared-library package
make test               # library, internal headless oracle, and tests
make test CC=clang      # same validation with Clang
make portable JOBS=8    # Debian 10 / glibc 2.28 package in Docker
make help               # complete command and variable reference
```

CMake can be used directly:

```sh
cmake --preset linux-dev
cmake --build --preset linux-dev
ctest --preset linux-dev
```

On Windows, open `wolf3d-lib.sln`, or use CMake:

```powershell
cmake --preset windows-dev-x64
cmake --build --preset windows-dev-x64 --config Release
ctest --preset windows-dev-x64 -C Release
```

Release presets stage clean SDK folders containing `wolf3d.dll` and
`wolf3d.lib` on Windows, or the versioned `libwolf3d.so` SONAME chain on
Linux, together with `WOLF3D.h`, Nuked-OPL3, licenses, and package notes.
See [docs/building.md](docs/building.md) for all supported configurations.

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
