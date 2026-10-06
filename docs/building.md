# Building wolf3d-lib

CMake is authoritative. GNU Make, the checked-in Visual Studio solution, and
the presets are maintained entry points around the same targets.

The [build and compatibility matrix](support-matrix.md) is the concise record
of supported compilers, produced artifacts, and runtime-validated destination
operating systems. This document supplies the detailed commands and options.

## Requirements

- CMake 3.16 or newer (3.20 or newer for presets) for modern builds
- CMake 3.5 for the isolated VC6--VS2005 legacy build definition
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

Open `wolf3d-lib.sln` in Visual Studio 2015, 2019, or 2022. This modern solution
band selects v140, v142, or v143 to match the IDE and supports Win32/x64,
Debug/Release, and static/dynamic MSVC runtime configurations. It delegates
compilation to CMake and writes into generation-specific directories under
`build/`.

The PowerShell dispatcher and CMake presets additionally support native
VS2013/v120, VS2012/v110, VS2010/v100, and VS2008/v90 builds. Checked-in IDE
solutions for those older format bands are tracked separately from compiler
support; until those solution bands are added, use the dispatcher or open the
CMake-generated solution in its compiler-specific `build/` directory.

The static MSVC runtime is the release default. This does not make the engine
a static library: `wolf3d.dll` remains a separate DLL, while its Microsoft C
runtime dependency is embedded.

### Windows build dispatcher

The root `build.ps1` detects the supported Visual Studio installations and is
the human-facing entry point for selecting one configuration. It exposes
compiler, architecture, Debug/Release, static/dynamic CRT, standard/silent
audio, Nuked-OPL3/DBOPL, build/test/package/clean actions, parallelism, and
dry-run output:

```powershell
.\build.ps1 -List
.\build.ps1
.\build.ps1 -Compiler vs2022 -Architecture x86 -Action test
.\build.ps1 -Compiler vs2019 -Action package -Opl dbopl
.\build.ps1 -Compiler vs2017 -Architecture x86 -Action package
.\build.ps1 -Compiler vs2015 -Architecture x86 -Action test
.\build.ps1 -Compiler vs2015-xp -Architecture x86 -Action package
.\build.ps1 -Compiler vs2013 -Architecture x86 -Action test
.\build.ps1 -Compiler vs2012 -Architecture x86 -Action test
.\build.ps1 -Compiler vs2010 -Architecture x86 -Action test
.\build.ps1 -Compiler vs2008 -Architecture x86 -Action test
.\build.ps1 -Action test -Audio silent -Runtime dynamic
```

With no arguments it interactively prompts for each relevant choice and asks
for confirmation. Explicit arguments remain suitable for automation;
`-NonInteractive` applies documented defaults without prompting. The root
launcher delegates to `scripts/windows/build.ps1`, which prints every CMake
command and contains no independent build graph. Run
`Get-Help .\scripts\windows\build.ps1 -Detailed` for the full interface.

Visual Studio 2015 and older do not bundle CMake. The dispatcher uses a CMake
3.20-or-newer installation from `PATH`, or the CMake bundled with a newer
installed Visual Studio, while still generating projects for the selected
native compiler and MSBuild generation.

### Windows XP-era compiler band

VC6 SP6, VS2002 SP1, VS2003 SP1, and VS2005 SP1 are supported as x86-only
historical checkpoints. They use `cmake/legacy/CMakeLists.txt` with CMake 3.5
and the XP-native CMD dispatcher; the modern root CMake project and PowerShell
dispatcher remain unchanged.

```bat
scripts\build-legacy.cmd vc6
scripts\build-legacy.cmd vs2002 Release standard nuked static test
scripts\build-legacy.cmd vs2003 Release silent nuked static package
scripts\build-legacy.cmd vs2005 Release standard dbopl dynamic package
```

Arguments are compiler, configuration, audio mode, OPL backend, CRT mode, and
action. Run the command without arguments for its complete usage summary.
`build` is the default action, `test` executes a public-header/DLL consumer,
and `package` stages a clean compiler-labeled SDK under `dist/`.

The legacy definition compiles the canonical `src/` tree; it contains no fork
of the engine. SDL3 is deliberately outside this compiler band. Production
applications use the companion repository's Win32/GDI wrapper.

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
- `WOLF3D_STDINT.h` for public-header consumers using VS2008 and older MSVC
- `wolf3d.dll` and `wolf3d.lib`, or the `libwolf3d.so` SONAME chain
- the replaceable Nuked-OPL3 shared library for default builds, or embedded
  DBOPL/silent backend selected at configure time
- project and third-party licensing/provenance notes

Production executables and OS-specific dependencies deliberately belong to
the companion `wolf3d-portable` repository.
