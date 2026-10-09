# Building wolf3d-lib

CMake is authoritative. GNU Make, the checked-in Visual Studio solution, and
the presets are maintained entry points around the same targets.

The [build and compatibility matrix](support-matrix.md) is the concise record
of supported compilers, produced artifacts, and runtime-validated destination
operating systems. This document supplies the detailed commands and options.

For cloning and confirmed updates, see [source updates](source-updates.md).

## Requirements

- CMake 3.16 or newer (3.20 or newer for presets) for modern builds
- CMake 3.5 for the isolated VC6--VS2005 legacy build definition
- A C99 compiler
- GNU Make for the Linux convenience commands
- Docker only for the optional glibc 2.28, musl, and Open Watcom DOS32 packages

Default builds include all three runtime-selectable drivers: Nuked-OPL3,
the GPL-compatible PrBoom+ C port of DBOPL, and timing-preserving silence.
Nuked remains the default and builds as a separate shared library.

## Linux with GNU Make

Run `./build.sh` without arguments for a guided scan and build plan. It
reports GCC, Clang, CMake, Make, Docker, and Git availability, presents only
usable native/container choices, prints the reproducible command, and asks
before executing it. It installs nothing. Arguments bypass the wizard and are
forwarded to Make; the commands below therefore remain the automation API.

```sh
make help
make                         # package with GCC
make library-release CC=clang
make test
make test CC=clang
make test OPL_DEFAULT=dbopl
make test OPL_DRIVERS=silent OPL_DEFAULT=silent
```

The native package is staged at
`dist/wolf3d-lib_<version>_linux-glibc_<arch>_gcc<major>`. Build trees are isolated
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
make musl OPL_DEFAULT=dbopl JOBS=8
```

It stages `dist/wolf3d-lib_<version>_linux-musl_<arch>_gcc<major>` and audits
the resulting ELF objects for accidental glibc symbol references. A musl
shared library must be loaded by a musl process; the companion portable
repository combines these artifacts with a bundled musl loader to create a
relocatable application directory.

## 32-bit DOS library with Open Watcom

The DOS library is cross-compiled from Linux through a pinned Docker image;
no native Open Watcom installation is required or supported by this profile:

```sh
make openwatcom
make openwatcom OPENWATCOM_OPL_DRIVERS=silent OPENWATCOM_DEFAULT_OPL=silent
```

The image uses official Open Watcom 2 release `2026-10-01-Build`, verifies
the downloaded Linux x64 archive against its checked-in SHA-256, and emits a
Pentium-targeted OMF SDK at
`dist/wolf3d-lib_<version>_dos32_x86_openwatcom<major>`. The default build
includes DBOPL, timing-preserving silence, and native AdLib, selects AdLib by
default, and requests 44100 Hz PCM. Nuked is opt-in. `WOLF3D.LIB` contains the engine and
adapters; `NUKEDOPL.LIB` remains separate when that driver is selected.

The build also compiles and links the public API consumer into a temporary
DOS/4G-format executable as an ABI/link check. That validation executable is
not shipped. The companion `wolf3d-portable` repository consumes the SDK in
its completed DOS/32A host with direct VGA, IRQ 1 keyboard, 700 Hz PIT timing,
SB16 PCM, and native AdLib output. That integration is interactively validated
under DOSBox; physical DOS hardware remains untested.

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

## Windows with Visual Studio or MinGW

Open `ide/visual-studio/vsYYYY/wolf3d-lib.sln` in the matching Visual Studio
generation. Every IDE from VS2002 through VS2026 has its own native project
format and compiler-pinned build; VC6 uses
`ide/visual-studio/vc6/wolf3d-lib.dsw`. Importing an older solution through a
newer IDE's conversion path is not the supported workflow. VS2008 and newer
delegate to their exact CMake generator; VC6--VS2005 delegate to the same
XP-native CMD workflow documented below.

The static MSVC runtime is the release default. This does not make the engine
a static library: `wolf3d.dll` remains a separate DLL, while its Microsoft C
runtime dependency is embedded.

### Windows build dispatcher

The root `build.ps1` detects supported Visual Studio installations and MSYS2
UCRT64 and is the human-facing entry point for selecting one configuration. It exposes
compiler, architecture, Debug/Release, static/dynamic CRT, compiled OPL driver
set, runtime default, preferred sample rate, build/test/package/clean actions,
parallelism, and dry-run output:

```powershell
.\build.ps1 -List
.\build.ps1
.\build.ps1 -Compiler vs2026 -Architecture x64 -Action test
.\build.ps1 -Compiler vs2022 -Architecture x86 -Action test
.\build.ps1 -Compiler vs2019 -Action package -DefaultOpl dbopl
.\build.ps1 -Compiler vs2017 -Architecture x86 -Action package
.\build.ps1 -Compiler vs2015 -Architecture x86 -Action test
.\build.ps1 -Compiler vs2015-xp -Architecture x86 -Action package
.\build.ps1 -Compiler vs2013 -Architecture x86 -Action test
.\build.ps1 -Compiler vs2012 -Architecture x86 -Action test
.\build.ps1 -Compiler vs2010 -Architecture x86 -Action test
.\build.ps1 -Compiler vs2008 -Architecture x86 -Action test
.\build.ps1 -Compiler mingw-ucrt64 -Action test
.\build.ps1 -Compiler mingw-ucrt64 -Action package
.\build.ps1 -Action test -Drivers silent -DefaultOpl silent -Runtime dynamic
```

With no arguments it runs `scripts/windows/configure-build.ps1`, reports every
recognized toolchain, asks only questions valid for the selected compiler,
prints a reproducible command, and requests confirmation. Explicit arguments
are forwarded to `scripts/windows/invoke-build.ps1`, the stable automation
backend that prints every CMake command and contains no independent build
graph. Run `Get-Help .\scripts\windows\invoke-build.ps1 -Detailed` for its
full interface. Neither frontend installs external tools.

The MinGW profile requires MSYS2's UCRT64 GCC toolchain, native CMake, and
Ninja packages. The default root is `C:\msys64`; pass
`-Msys2Root C:\path\to\msys64` for a portable or non-default installation.
It currently targets x64. The dispatcher modifies `PATH` only for its own
process, and release binaries statically link GCC support code; the resulting
SDK imports Windows UCRT API sets but no MSYS, Cygwin, libgcc, libstdc++, or
winpthread runtime DLLs.

For a fresh MSYS2 installation, open the **MSYS2 UCRT64** terminal and update
the package database and base installation:

```sh
pacman -Syu
```

If MSYS2 asks you to close the terminal, reopen the **MSYS2 UCRT64** terminal
and run `pacman -Syu` again. Then install the required x64 UCRT toolchain and
build tools:

```sh
pacman -S --needed \
  mingw-w64-ucrt-x86_64-toolchain \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja
```

Return to ordinary PowerShell and run `.\build.ps1 -List` to verify that
`mingw-ucrt64` is reported as ready. The project does not require an MSYS2
SDL3 package.

Visual Studio 2017 bundles a CMake older than the preset format used here;
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
build.cmd
scripts\windows\legacy\build.cmd vc6
scripts\windows\legacy\build.cmd vs2002 Release all nuked static test
scripts\windows\legacy\build.cmd vs2003 Release silent silent static package
scripts\windows\legacy\build.cmd vs2005 Release all dbopl dynamic package
```

Run root `build.cmd` without arguments for the XP-compatible guided scanner.
Arguments passed to it are forwarded to the deterministic legacy executor.
Those arguments are compiler, configuration, compiled driver set, default OPL
driver, CRT mode, and action. Run the executor without arguments for its usage
summary.
`build` is the default action, `test` executes a public-header/DLL consumer,
and `package` stages a clean compiler-labeled SDK under `dist/`.

The legacy definition compiles the canonical `src/` tree; it contains no fork
of the engine. SDL3 is deliberately outside this compiler band. Production
applications use the companion repository's Win32/GDI wrapper.

## Windows cross-compilation from Linux

Docker-pinned profiles build native Windows DLL SDKs without Wine:

```text
make windows-win9x       # Open Watcom, x86, Windows 95 profile
make windows-xp          # MinGW-w64/GCC + MSVCRT, x86
make windows-win7        # MinGW-w64/GCC + MSVCRT, x86 and x64
make windows-llvm-win7   # LLVM-MinGW + MSVCRT, x86 and x64
make windows-win10       # LLVM-MinGW + UCRT, x64
make windows-cross       # all of the above
```

The first invocation downloads the pinned Open Watcom or LLVM-MinGW archive
and verifies its SHA-256, or installs Debian's versioned MinGW packages into a
project-specific image. Subsequent builds reuse those images. Each package is
checked with a PE architecture/import audit; Wine is neither installed nor
used. `make clean-windows-cross` removes the corresponding build trees and
project images. Destination-OS gameplay remains a separate validation step.

## Windows with CMake

```powershell
cmake --preset windows-dev-x64
cmake --build --preset windows-dev-x64 --config Release
ctest --preset windows-dev-x64 -C Release

cmake --preset windows-library-x64
cmake --build --preset windows-library-x64
```

The historical `windows-*` preset names explicitly use Visual Studio 2019 and
v142 and write to `build/windows-vs2019-*`. Visual Studio 2022/v143 and
Visual Studio 2026/v145 use parallel generation-qualified names and distinct
build trees:

```powershell
cmake --preset windows-vs2022-dev-x64
cmake --build --preset windows-vs2022-dev-x64 --config Release
ctest --preset windows-vs2022-dev-x64

cmake --preset windows-vs2022-library-x64
cmake --build --preset windows-vs2022-library-x64

cmake --preset windows-vs2026-dev-x64
cmake --build --preset windows-vs2026-dev-x64 --config Release
ctest --preset windows-vs2026-dev-x64

cmake --preset windows-vs2026-library-x64
cmake --build --preset windows-vs2026-library-x64
```

From an MSYS2 UCRT64 shell, the corresponding native-GCC presets are:

```text
cmake --preset windows-mingw-ucrt64-dev-x64
cmake --build --preset windows-mingw-ucrt64-dev-x64
ctest --preset windows-mingw-ucrt64-dev-x64

cmake --preset windows-mingw-ucrt64-library-x64
cmake --build --preset windows-mingw-ucrt64-library-x64
```

Use `windows-*-x86` for 32-bit output. For a dynamic MSVC runtime add
`-DWG_STATIC_MSVC_RUNTIME=OFF` and use a distinct binary directory.

## CMake options

| Option | Default | Purpose |
| --- | --- | --- |
| `WG_BUILD_HEADLESS` | `ON` | Build the internal deterministic validation host and tests. |
| `WG_WARNINGS_AS_ERRORS` | `ON` | Treat project warnings as errors. |
| `WG_STATIC_MSVC_RUNTIME` | `ON` | Embed the MSVC runtime in Windows artifacts. |
| `WG_STATIC_GNU_RUNTIME` | `ON` | Statically link GCC support code in MinGW artifacts. |
| `WG_ENABLE_OPL_NUKED` | `ON` | Compile the faithful Nuked-OPL3 runtime driver. |
| `WG_ENABLE_OPL_DBOPL` | `ON` | Compile the lower-resource pure-C DBOPL runtime driver. |
| `WG_ENABLE_OPL_SILENT` | `ON` | Compile timing-preserving silence. |
| `WG_DEFAULT_OPL_DRIVER` | `nuked` | Select the initial driver; `--opl` may select any compiled driver at runtime. |
| `WG_DEFAULT_SAMPLE_RATE` | `48000` | Preferred host PCM rate; `--sample-rate` overrides it and the host reports the rate obtained. |
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
- the replaceable Nuked-OPL3 shared library when compiled, plus embedded
  DBOPL and silent drivers enabled for that build
- project and third-party licensing/provenance notes

The Open Watcom package is the deliberate exception to the shared-library
layout: protected-mode DOS consumes the engine as `WOLF3D.LIB` plus the
optional replaceable `NUKEDOPL.LIB`.

Production executables and OS-specific dependencies deliberately belong to
the companion `wolf3d-portable` repository.
