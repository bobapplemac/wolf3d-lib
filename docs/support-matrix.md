# Build and compatibility matrix

This document is the authoritative user-facing matrix for building
`wolf3d-lib`. It deliberately separates three different claims:

- **Build validated** means the named compiler produced the library and passed
  the applicable tests.
- **Runtime validated** means the produced binaries were also executed on the
  named destination operating system.
- **Planned** means the combination is part of the roadmap, not current
  support.

The engine is always built as a shared library (`wolf3d.dll` or
`libwolf3d.so`). On Windows, "static" and "dynamic" describe the compiler
support runtime linked into that DLL; they do not turn wolf3d into a static
library. MinGW still uses the Windows-provided UCRT dynamically.

## Build entry points

| Build host | Recommended entry point | Typical release/package command | Result |
| --- | --- | --- | --- |
| Linux | Guided Bash configurator / GNU Make | `./build.sh` or `make library-release CC=clang` | Native glibc SDK under `dist/` |
| Linux + Docker | Guided Bash configurator / GNU Make | `./build.sh` or `make portable` | Debian 10 / glibc 2.28 x86-64 SDK |
| Linux + Docker | Guided Bash configurator / GNU Make | `./build.sh` or `make musl CC=clang` | Alpine/musl x86-64 SDK |
| Linux | CMake presets | `cmake --preset linux-library` then `cmake --build --preset linux-library` | Native library build/package |
| Windows, VS2008--VS2022 | Guided PowerShell dispatcher | `.\build.ps1` or `.\build.ps1 -Compiler vs2019 -Action package` | Compiler-labelled SDK under `dist/` |
| Windows, MinGW UCRT64 | Guided PowerShell dispatcher | `.\build.ps1 -Compiler mingw-ucrt64 -Action package` | Native x64 SDK with static GCC support runtime |
| Windows, VS2002--VS2022 | Visual Studio | Open `ide/visual-studio/vsYYYY/wolf3d-lib.sln` in the matching IDE | Same CMake-backed SDK |
| Windows, VC6 | Visual C++ 6.0 | Open `ide/visual-studio/vc6/wolf3d-lib.dsw` | Same legacy-CMake SDK |
| Windows, modern CMake | CMake presets | `cmake --preset windows-vs2022-library-x64` then `cmake --build --preset windows-vs2022-library-x64` | Selected x86/x64 SDK |
| Windows XP, VC6--VS2005 | Guided CMD configurator / native dispatcher | `build.cmd` or `scripts\windows\legacy\build.cmd vc6 Release all nuked static package` | XP-era x86 SDK |

Run `./build.sh`, `make help`, `.\build.ps1`, or `build.cmd` without
arguments for the complete option list. See [building.md](building.md) for
backend, test-data, and direct-CMake details.

## Windows compiler matrix

| Compiler environment | Toolset | Architecture | CRT modes | Build status | Destination validation |
| --- | --- | --- | --- | --- | --- |
| MSYS2 UCRT64 | MinGW-w64 GCC 16.2 | x64 | static GCC support runtime | Validated | Build, package, API, and deterministic tests on current Windows host; destination runtime pending |
| Visual Studio 2022 17.14 | v143 / MSVC 19.44 | x86, x64 | static, dynamic | Validated | Current Windows development host |
| Visual Studio 2019 16.11 | v142 / MSVC 19.29 | x86, x64 | static, dynamic | Validated | Current Windows development host |
| VS2017 toolset hosted by VS2019 | v141 / MSVC 19.16 | x86, x64 | static, dynamic | Validated | No legacy-OS minimum claimed |
| Visual Studio 2015 14.0 | v140 / MSVC 19.00.23506 | x86, x64 | static, dynamic | Validated | No legacy-OS minimum claimed |
| Visual Studio 2015 XP SDK | v140_xp / MSVC 19.00.23506 | x86, x64 | static, dynamic | Validated | Pending on actual XP; PE minimum 5.01 x86 / 5.02 x64 |
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

Native checked-in solutions/workspaces are first-class for VC6--VS2022. Each
is pinned 1:1 to its named IDE and toolset; opening an older solution through
a newer IDE's upgrade path is not the supported workflow. The PowerShell
dispatcher/direct CMake path remains first-class for VS2008--VS2022 and
MinGW UCRT64; the XP-native CMD dispatcher backs the VC6--VS2005 IDE projects.

## Linux compiler and libc matrix

| Build profile | Compilers | Architecture | C library / ABI | Build status | Destination compatibility |
| --- | --- | --- | --- | --- | --- |
| Native | GCC, Clang | Host (currently x86-64) | Build host glibc | Validated | Same or newer compatible glibc; exact floor is the build host |
| Portable glibc | GCC | x86-64 | Debian 10, audited maximum `GLIBC_2.28` | Validated | x86-64 Linux with glibc 2.28 or newer |
| musl | GCC, Clang | x86-64 | Alpine musl | Validated | A musl process with a compatible musl ABI; not loadable into a glibc process |

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

Use `OPL_DRIVERS`, `OPL_DEFAULT`, and `SAMPLE_RATE` with Make, or the
corresponding dispatcher/CMake options documented in
[building.md](building.md).

## Planned, not currently supported

| Target | Intended direction | Current status |
| --- | --- | --- |
| Open Watcom DOS extender | Linux/Docker cross-build targeting 32-bit DOS with DOS/32A | Planned; no supported build command or runtime claim yet |
| Real AdLib/OPL hardware backend | Replaceable hardware register-output component | Design discussion only |

