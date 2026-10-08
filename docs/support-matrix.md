# Build and compatibility matrix

This document is the authoritative user-facing matrix for building
`wolf3d-lib`. It deliberately separates three different claims:

- **Build validated** means the named compiler produced the library and passed
  the applicable tests.
- **Runtime validated** means the produced binaries were also executed on the
  named destination operating system.
- **Planned** means the combination is part of the roadmap, not current
  support.

Normal Windows and Linux profiles build the engine as a shared library
(`wolf3d.dll` or `libwolf3d.so`). On Windows, "static" and "dynamic" describe
the compiler support runtime linked into that DLL; they do not turn wolf3d
into a static library. MinGW still uses the Windows-provided UCRT dynamically.
The Open Watcom cross-profile is the platform-appropriate exception: it emits
an OMF `WOLF3D.LIB` to link into a protected-mode DOS host.

## Build entry points

| Build host | Recommended entry point | Typical release/package command | Result |
| --- | --- | --- | --- |
| Linux | Guided Bash configurator / GNU Make | `./build.sh` or `make library-release CC=clang` | Native glibc SDK under `dist/` |
| Linux + Docker | Guided Bash configurator / GNU Make | `./build.sh` or `make portable` | Debian 10 / glibc 2.28 x86-64 SDK |
| Linux + Docker | Guided Bash configurator / GNU Make | `./build.sh` or `make musl CC=clang` | Alpine/musl x86-64 SDK |
| Linux + Docker | Guided Bash configurator / GNU Make | `./build.sh` or `make openwatcom` | Open Watcom 32-bit DOS OMF SDK |
| Linux | CMake presets | `cmake --preset linux-library` then `cmake --build --preset linux-library` | Native library build/package |
| Windows, VS2008--VS2026 | Guided PowerShell dispatcher | `.\build.ps1` or `.\build.ps1 -Compiler vs2019 -Action package` | Compiler-labelled SDK under `dist/` |
| Windows, MinGW UCRT64 | Guided PowerShell dispatcher | `.\build.ps1 -Compiler mingw-ucrt64 -Action package` | Native x64 SDK with static GCC support runtime |
| Windows, VS2002--VS2026 | Visual Studio | Open `ide/visual-studio/vsYYYY/wolf3d-lib.sln` in the matching IDE | Same CMake-backed SDK |
| Windows, VC6 | Visual C++ 6.0 | Open `ide/visual-studio/vc6/wolf3d-lib.dsw` | Same legacy-CMake SDK |
| Windows, modern CMake | CMake presets | `cmake --preset windows-vs2022-library-x64` then `cmake --build --preset windows-vs2022-library-x64` | Selected x86/x64 SDK |
| Windows XP, VC6--VS2005 | Guided CMD configurator / native dispatcher | `build.cmd` or `scripts\windows\legacy\build.cmd vc6 Release all nuked static package` | XP-era x86 SDK |

Run `./build.sh`, `make help`, `.\build.ps1`, or `build.cmd` without
arguments for the complete option list. See [building.md](building.md) for
backend, test-data, and direct-CMake details.

## Windows compiler matrix

| Compiler environment | Toolset | Architecture | CRT modes | Build status | Destination validation |
| --- | --- | --- | --- | --- | --- |
| MSYS2 UCRT64 | MinGW-w64 GCC 16.2 | x64 | static GCC support runtime | Validated | Package, API, deterministic tests, and all runtime-selectable OPL drivers validated on Windows 11 x64 |
| Visual Studio 2026 18.10 | v145 / MSVC 19.51 | x86, x64 | static, dynamic | Validated | Windows 11 compatibility host |
| Visual Studio 2022 17.14 | v143 / MSVC 19.44 | x86, x64 | static, dynamic | Validated | Current Windows development host |
| Visual Studio 2019 16.11 | v142 / MSVC 19.29 | x86, x64 | static, dynamic | Validated | Current Windows development host |
| Visual Studio 2017 15.9 | v141 / MSVC 19.16 | x86, x64 | static, dynamic | Validated | Windows 11 compatibility host; no legacy-OS minimum claimed |
| Visual Studio 2015 14.0 | v140 / MSVC 19.00.23506 | x86, x64 | static, dynamic | Validated | No legacy-OS minimum claimed |
| Visual Studio 2015 XP SDK | v140_xp / MSVC 19.00.24210 | x86, x64 | static, dynamic | Validated | x86 DLL, API consumer, and deterministic tests validated on Windows XP SP3; x64 destination untested; PE minimum 5.01 x86 / 5.02 x64 |
| Visual Studio 2013 Update 5 | v120 / MSVC 18.00.40629 | x86, x64 | static, dynamic | Validated | No legacy-OS minimum claimed |
| Visual Studio 2012 Update 5 | v110 / MSVC 17.00.61030 | x86, x64 | static, dynamic | Validated | No legacy-OS minimum claimed |
| Visual Studio 2010 SP1 | v100 / MSVC 16.00.40219 | x86, x64 | static, dynamic | Validated | No legacy-OS minimum claimed |
| Visual Studio 2008 SP1 | v90 / MSVC 15.00.30729 | x86, x64 | static, dynamic | Validated | No legacy-OS minimum claimed |
| Visual Studio 2005 SP1 | MSVC 14.00.50727.762 | x86 | static, dynamic | Validated | Runtime/API validated on Windows XP SP3 x86 |
| Visual Studio .NET 2003 SP1 | MSVC 13.10.6030 | x86 | static, dynamic | Validated | Runtime/API validated on Windows XP SP3 x86 |
| Visual Studio .NET 2002 SP1 | MSVC 13.00.9466 | x86 | static, dynamic | Validated | Runtime/API validated on Windows XP SP3 x86 |
| Visual C++ 6.0 SP6 | MSVC 12.00.8804 | x86 | static, dynamic | Validated | Runtime/API validated on Windows XP SP3 x86; VC6 DLL exercised by the Win32/GDI game on Windows 11 x64 under WOW64 |

The selected toolset, Windows SDK, CRT mode, and imports jointly determine a
Windows binary's actual OS floor. Consequently, a successful historical
compiler build is not presented as an older-OS guarantee unless the matrix
also records a runtime test on that OS.

Native checked-in solutions/workspaces are first-class for VC6--VS2026. Each
is pinned 1:1 to its named IDE and toolset; opening an older solution through
a newer IDE's upgrade path is not the supported workflow. The PowerShell
dispatcher/direct CMake path remains first-class for VS2008--VS2026 and
MinGW UCRT64; the XP-native CMD dispatcher backs the VC6--VS2005 IDE projects.

## Linux compiler and libc matrix

| Build profile | Compilers | Architecture | C library / ABI | Build status | Destination compatibility |
| --- | --- | --- | --- | --- | --- |
| Native | GCC, Clang | Host (currently x86-64) | Build host glibc | Validated | Same or newer compatible glibc; exact floor is the build host |
| Portable glibc | GCC | x86-64 | Debian 10, audited maximum `GLIBC_2.28` | Validated | x86-64 Linux with glibc 2.28 or newer |
| musl | GCC, Clang | x86-64 | Alpine musl | Validated | A musl process with a compatible musl ABI; not loadable into a glibc process |
| Open Watcom DOS32 | Open Watcom 2 `2026-10-01-Build` | Pentium-class x86 | 32-bit protected-mode DOS OMF | Build and public-consumer link validated in pinned Docker image | Integrated runtime validated by the companion DOS/32A host under DOSBox 0.74; physical DOS hardware remains untested |

glibc is forward-compatible in the direction useful here: an artifact limited
to glibc 2.28 symbols is intended to run on newer glibc releases. The audit
cannot guarantee every kernel, loader, CPU, or distribution combination.
The musl output is an SDK, not a standalone executable; the companion
`wolf3d-portable` repository supplies the bundled musl application layout.

## Audio/backend matrix

Every modern compiler profile includes all three drivers by default and can
omit unneeded implementations at compile time:

| Runtime selection | Implementation | Packaging |
| --- | --- | --- |
| `--opl nuked` | Nuked-OPL3 (default) | Separate LGPL shared library |
| `--opl dbopl` | DBOPL | Embedded pure-C implementation |
| `--opl silent` | Silent timing driver | Timing and register activity retained; all PCM is silent |

Emulated hardware is selected separately: the default exposes Sound Blaster
and AdLib-compatible hardware, `--adlib`/`-nosb` exposes AdLib only,
`--pc-speaker`/`-noal` exposes neither card and selects PC-speaker effects, and
`--no-sound` exposes neither card with all in-game sound initially off. Every
profile preserves the internal audio clock.

Use `OPL_DRIVERS`, `OPL_DEFAULT`, and `SAMPLE_RATE` with Make, or the
corresponding dispatcher/CMake options documented in
[building.md](building.md).

## Companion DOS integration

| Target | Integration | Validation |
| --- | --- | --- |
| DOS/32A portable host | `wolf3d-portable` links this OMF library with direct VGA, IRQ 1 keyboard, 700 Hz PIT, SB16 PCM, and DOS/32A | Interactive DOSBox gameplay confirmed; physical DOS hardware remains untested |
| Native AdLib/OPL backend | The v4 platform callbacks keep port I/O in the host while the library's `adlib` adapter preserves the 700 Hz engine clock | Native port-388h operation confirmed under DOSBox; DBOPL and silent fallbacks also confirmed |

