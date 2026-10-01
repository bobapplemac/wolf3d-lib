# Building wolf3d-lib

CMake is authoritative. GNU Make, the checked-in Visual Studio solution, and
the presets are maintained entry points around the same targets.

## Requirements

- CMake 3.16 or newer (3.20 or newer for presets)
- A C99 compiler
- GNU Make for the Linux convenience commands
- Docker only for the optional glibc 2.28 and musl packages

Nuked-OPL3 is the default and builds as a separate shared library. The
GPL-compatible PrBoom+ C port of DBOPL and a timing-preserving silent backend
are selectable alternatives.

## Linux with GNU Make

```sh
make help
make                         # package with GCC
make library-release CC=clang
make test
make test CC=clang
make test OPL_BACKEND=dbopl
make test AUDIO_BACKEND=silent
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

The musl path builds in a digest-pinned Alpine container and labels its SDK
separately from glibc output:

```sh
make musl JOBS=8
make musl CC=clang JOBS=8
make musl OPL_BACKEND=dbopl JOBS=8
```

It stages `dist/wolf3d-<version>-library-linux-musl-<architecture>` and audits
the resulting ELF objects for accidental glibc symbol references. A musl
shared library must be loaded by a musl process; the companion portable
repository combines these artifacts with a bundled musl loader to create a
relocatable application directory.

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

Open `wolf3d-lib.sln` in Visual Studio 2019 or 2022. The same project selects
v142 or v143 to match the IDE and supports Win32/x64, Debug/Release, and
static/dynamic MSVC runtime configurations. It delegates compilation to CMake
and writes into generation-specific directories under `build/`.

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

The historical `windows-*` preset names explicitly use Visual Studio 2019 and
v142 and write to `build/windows-vs2019-*`. For Visual Studio 2022 and v143,
use the parallel names, which write to `build/windows-vs2022-*`:

```powershell
cmake --preset windows-vs2022-dev-x64
cmake --build --preset windows-vs2022-dev-x64 --config Release
ctest --preset windows-vs2022-dev-x64

cmake --preset windows-vs2022-library-x64
cmake --build --preset windows-vs2022-library-x64
```

Use `windows-*-x86` for 32-bit output. For a dynamic MSVC runtime add
`-DWG_STATIC_MSVC_RUNTIME=OFF` and use a distinct binary directory.

## CMake options

| Option | Default | Purpose |
| --- | --- | --- |
| `WG_BUILD_HEADLESS` | `ON` | Build the internal deterministic validation host and tests. |
| `WG_WARNINGS_AS_ERRORS` | `ON` | Treat project warnings as errors. |
| `WG_STATIC_MSVC_RUNTIME` | `ON` | Embed the MSVC runtime in Windows artifacts. |
| `WG_AUDIO_BACKEND` | `standard` | `standard` emits PCM; `silent` advances identical logical audio state while emitting zero samples. |
| `WG_OPL_BACKEND` | `nuked` | Select `nuked` or the pure-C `dbopl` implementation for standard audio. |
| `WG_LINUX_LIBC` | empty | Optional `glibc`/`musl` package label used by reproducible Linux builds. |
| `WG_COMPILER_LABEL` | empty | Optional compiler/toolset label appended to a staged package directory. Windows presets set this automatically. |
| `WG_DIST_ROOT` | `<source>/dist` | Destination root for staged packages. |
| `WG_TEST_WL1_PATH` and related paths | empty | Enable external-data regression groups. |
| `WG_ORIGINAL_SOURCE_PATH` | empty | Original-source input for the optional call-graph audit. |
| `WG_CHOCOLATE_SOURCE_PATH` | empty | Chocolate Wolfenstein input for that audit. |

## Produced library package

A staged package contains:

- `WOLF3D.h`
- `wolf3d.dll` and `wolf3d.lib`, or the `libwolf3d.so` SONAME chain
- the replaceable Nuked-OPL3 shared library for default builds, or embedded
  DBOPL/silent backend selected at configure time
- project and third-party licensing/provenance notes

Production executables and OS-specific dependencies deliberately belong to
the companion `wolf3d-portable` repository.
