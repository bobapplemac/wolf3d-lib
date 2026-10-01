# Building wolf3d-lib

CMake is authoritative. GNU Make, the checked-in Visual Studio solution, and
the presets are maintained entry points around the same targets.

## Requirements

- CMake 3.16 or newer (3.20 or newer for presets)
- A C99 compiler
- GNU Make for the Linux convenience commands
- Docker only for the optional glibc 2.28 portable package

Nuked-OPL3 is included in `third_party/Nuked-OPL3` and always builds as a
separate shared library.

## Linux with GNU Make

```sh
make help
make                         # package with GCC
make library-release CC=clang
make test
make test CC=clang
```

The native package is staged at
`dist/wolf3d-<version>-library-linux-<architecture>`. Build trees are isolated
by compiler under `build/`.

The portable path builds inside the digest-pinned Debian 10 container and
audits every ELF object against the requested glibc ceiling:

```sh
make portable JOBS=8
```

Its default maximum is `GLIBC_2.28`. Applications linked on that baseline
remain compatible with newer glibc systems under glibc's forward-compatibility
contract.

## Linux with CMake presets

```sh
cmake --preset linux-dev
cmake --build --preset linux-dev
ctest --preset linux-dev

cmake --preset linux-library
cmake --build --preset linux-library
```

`linux-dev` includes the internal headless validation executable and tests.
`linux-library` stages only the redistributable library SDK.

## Windows with Visual Studio

Open `wolf3d-lib.sln`. The project supports Win32/x64, Debug/Release, and
static/dynamic MSVC runtime configurations. It delegates compilation to CMake
and writes into the same `build/windows-dev-*` directories as the CLI.

The static MSVC runtime is the release default. This does not make the engine
a static library: `wolf3d.dll` remains a separate DLL, while its Microsoft C
runtime dependency is embedded.

## Windows with CMake

```powershell
cmake --preset windows-dev-x64
cmake --build --preset windows-dev-x64 --config Release
ctest --preset windows-dev-x64 -C Release

cmake --preset windows-library-x64
cmake --build --preset windows-library-x64
```

Use `windows-*-x86` for 32-bit output. For a dynamic MSVC runtime add
`-DWG_STATIC_MSVC_RUNTIME=OFF` and use a distinct binary directory.

## CMake options

| Option | Default | Purpose |
| --- | --- | --- |
| `WG_BUILD_HEADLESS` | `ON` | Build the internal deterministic validation host and tests. |
| `WG_WARNINGS_AS_ERRORS` | `ON` | Treat project warnings as errors. |
| `WG_STATIC_MSVC_RUNTIME` | `ON` | Embed the MSVC runtime in Windows artifacts. |
| `WG_DIST_ROOT` | `<source>/dist` | Destination root for staged packages. |
| `WG_TEST_WL1_PATH` and related paths | empty | Enable external-data regression groups. |
| `WG_ORIGINAL_SOURCE_PATH` | empty | Original-source input for the optional call-graph audit. |
| `WG_CHOCOLATE_SOURCE_PATH` | empty | Chocolate Wolfenstein input for that audit. |

## Produced library package

A staged package contains:

- `WOLF3D.h`
- `wolf3d.dll` and `wolf3d.lib`, or the `libwolf3d.so` SONAME chain
- the replaceable Nuked-OPL3 shared library
- project and third-party licensing/provenance notes

Production executables and OS-specific dependencies deliberately belong to
the companion `wolf3d-portable` repository.
