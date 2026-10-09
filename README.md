# wolf3d-lib

`wolf3d-lib` is a portable shared-library adaptation of the original
Wolfenstein 3D v1.4 source. It preserves the original gameplay, fixed-point
math, data formats, 70 Hz logical timing, rendering, and audio behavior while
placing operating-system services behind a compact C99 callback API.

This repository owns the engine only. Ready-to-run Win32, SDL3, and Linux
direct-console applications live in the companion
[`wolf3d-portable`](https://github.com/bobapplemac/wolf3d-portable)
repository. Portable builds compile their selected engine source; its build scripts offer
the latest compatible engine from `main`.

The project's relationship to id Software's original repository and the
licensing evidence behind this modernization are documented in
[`docs/history.md`](docs/history.md) and
[`docs/licensing-history.md`](docs/licensing-history.md).

Original Wolfenstein 3D or Spear of Destiny game data is required and is not
distributed here.

`--game EXT` selects an exact data extension; short aliases `-WL1`, `-WL6`,
`-SDM`, `-SOD`, `-SD1`, `-SD2`, and `-SD3` are equivalent. Executable names
beginning with `wolf` prefer the Wolf3D family, while the exact basename
`spear` prefers Spear. Recursive directory discovery belongs to the companion
`wolf3d-portable` launchers; `--data PATH` remains an exact library-directory
contract.

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

The library also exports read-only OPL-driver discovery functions plus the
indexed 320x200 `wolf3d_ScreenBuffer` and 256-color `wolf3d_Palette`. Default
builds include Nuked-OPL3, pure-C DBOPL, and timing-preserving silent drivers;
`--opl nuked|dbopl|silent` selects among the compiled choices at runtime.
Nuked-OPL3 remains a separate, replaceable LGPL shared library.

The v4 platform table additionally exposes optional native-OPL lifecycle and
register-write callbacks. They support constrained hardware hosts without
adding raw I/O to the portable engine; ordinary Windows and Linux builds leave
them unset and do not compile the `adlib` adapter.

The in-tree `wolf3d::wolf3d` CMake target is the supported target for a
parent project. The public header is under `include/`; private `src/`
headers are not a host interface.

Runtime audio selection is intentionally host-independent:

```text
--opl nuked|dbopl|silent  Select an implementation compiled into the library
--sample-rate HZ          Request 8000--192000 Hz PCM (default: 48000)
--adlib / -nosb           Emulate an AdLib-only machine
--pc-speaker / -noal      Emulate no sound card; default to PC-speaker effects
--no-sound                Emulate no sound card and select no audio
```

The host may negotiate another application-facing PCM rate. The library then
constructs the selected emulator at that obtained rate while preserving the
exact rational 700 Hz IMF clock and 140 Hz effect clock.
Hardware profiles are independent of the selected OPL implementation and of
whether the host opens a physical audio device.

## Build

The root `build.sh`, `build.ps1`, and `build.cmd` scripts check for newer
published source before building. For a clean older copy, answer **Yes** to
update the source and its required components together, or **No** (the default)
to build your current copy. Local edits, local development commits, and
explicitly selected source versions are preserved. The check follows your
configured branch (normally `main`); it never switches branches.

Portable normally offers to build against the latest compatible `wolf3d-lib`
`main`, including engine fixes published without an application update. The
same confirmation covers application updates and engine updates. The host
independently declares its supported API in `platforms/WG_ENGINE_COMPAT.h`;
an incompatible engine update is skipped with an explanation, and compilation
also rejects an incompatible engine selected manually. Breaking contract
changes must advance the engine API version.

The recorded engine commit remains a reproducible/offline starting point;
third-party components such as SDL3 stay at their recorded versions. An engine
update accepted through these scripts is remembered locally so later checks
can continue updating it. Edits or other custom component selections are
preserved. Build/package versions identify the engine actually selected.

The check needs Git (and Git for Windows or MSYS2 Bash on Windows). Source archives,
unavailable tools, and failed network checks continue with existing sources.
An accepted update that fails stops the build so incomplete dependencies are
not used. Unattended runs never accept updates automatically. Set
`WOLF3D_GIT_CHECK=0` to skip the check entirely, or
`WOLF3D_GIT_INTERACTIVE=0` to check without prompting; PowerShell's
`-NonInteractive` also disables update prompts. Direct Make/CMake and executor
scripts retain their existing behavior. Regression coverage can be run with
`python tests/WG_GIT_PREFLIGHT_TEST.py` (Python 3 and Git/Bash required).


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
make openwatcom         # 32-bit protected-mode DOS OMF SDK in Docker
make windows-cross      # Win9x/XP/Win7/Win10 native PE SDK matrix in Docker
make test OPL_DEFAULT=dbopl
make test OPL_DRIVERS=silent OPL_DEFAULT=silent
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
Visual Studio installations from VS2008/v90 through VS2026/v145 and MSYS2
UCRT64 MinGW. Run it with no arguments for an interactive wizard:

```powershell
.\build.ps1 -List
.\build.ps1
.\build.ps1 -Compiler vs2019 -Architecture x86 -Action test
.\build.ps1 -Compiler vs2008 -Architecture x86 -Action test
.\build.ps1 -Compiler mingw-ucrt64 -Action package
.\build.ps1 -Action package -DefaultOpl dbopl -Runtime dynamic
```

Windows XP-era x86 builds use an isolated CMake 3.5 definition and native CMD
dispatcher, preserving compatibility with VC6 SP6 through VS2005 SP1:

```bat
build.cmd
scripts\windows\legacy\build.cmd vc6 Release all nuked static test
scripts\windows\legacy\build.cmd vs2002 Release all nuked static package
scripts\windows\legacy\build.cmd vs2003
scripts\windows\legacy\build.cmd vs2005
```

Release presets stage clean SDK folders containing `wolf3d.dll` and the
compiler-appropriate import library on Windows, or the versioned
`libwolf3d.so` SONAME chain on Linux, together with `WOLF3D.h` (and the
legacy-MSVC compatibility header where
applicable), the compiled audio drivers, licenses, and package notes.
The Linux-hosted Open Watcom profile instead stages an OMF `WOLF3D.LIB` for
linking into a 32-bit protected-mode DOS host; it is a cross-build SDK, not
itself a playable DOS application. A native IDE workspace is also available
at `ide/open-watcom/wolf3d-lib.wpj`; launch it through the adjacent
`open-ide.cmd` so the canonical include paths and driver definitions are set.
Linux/Docker can also cross-build native Windows DLL SDKs with Open Watcom,
MinGW-w64, and LLVM-MinGW; use `make windows-cross` or an individual
`windows-win9x`, `windows-xp`, `windows-win7`, `windows-llvm-win7`, or
`windows-win10` target. These builds use no Wine and include PE/import audits.
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

DOS and Win9x defaults include `dbopl,silent,adlib` and select native `adlib`.
Nuked remains available as an explicit build choice. Use `--opl dbopl` for
software OPL (the spelling is `dbopl`, not `dbpol`) or `--opl silent`.
Win9x native AdLib requires accessible ISA OPL hardware at 388h/389h; it is
not available on NT-based Windows. The Win9x console-subsystem launcher prints
help/diagnostics in its invoking command prompt while the game opens a GDI window.
