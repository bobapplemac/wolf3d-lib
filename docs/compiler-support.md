# Compiler support

wolf3d-lib treats compiler support as a tested compatibility matrix rather
than assuming that one successful modern build establishes portability.

For the user-facing cross-platform build, artifact, and destination-OS table,
see [support-matrix.md](support-matrix.md). This file retains the detailed
compiler-checkpoint policy and evidence.

## Validated Windows checkpoints

| Environment | Toolset | Architectures | Runtime modes | Status |
| --- | --- | --- | --- | --- |
| MSYS2 UCRT64 | MinGW-w64 GCC 16.2 | x64 | static GCC support runtime | Build/package/tests validated; destination runtime pending |
| Visual Studio 2026 18.10 | v145 / MSVC 19.51 | x86, x64 | static, dynamic | Validated |
| Visual Studio 2022 17.14 | v143 / MSVC 19.44 | x86, x64 | static, dynamic | Validated |
| Visual Studio 2019 16.11 | v142 / MSVC 19.29 | x86, x64 | static, dynamic | Validated |
| Visual Studio 2017 15.9 | v141 / MSVC 19.16 | x86, x64 | static, dynamic | Validated |
| Visual Studio 2015 14.0 | v140 / MSVC 19.00.23506 | x86, x64 | static, dynamic | Validated |
| Visual Studio 2015 14.0, XP SDK | v140_xp / MSVC 19.00.23506 | x86, x64 | static, dynamic | Build/test validated; target OS pending |
| Visual Studio 2013 Update 5 | v120 / MSVC 18.00.40629 | x86, x64 | static, dynamic | Validated |
| Visual Studio 2012 Update 5 | v110 / MSVC 17.00.61030 | x86, x64 | static, dynamic | Validated |
| Visual Studio 2010 SP1 | v100 / MSVC 16.00.40219 | x86, x64 | static, dynamic | Validated |
| Visual Studio 2008 SP1 | v90 / MSVC 15.00.30729 | x86, x64 | static, dynamic | Validated |
| Visual Studio 2005 SP1 | MSVC 14.00.50727.762 | x86 | static, dynamic | Build/API runtime validated on XP SP3 |
| Visual Studio .NET 2003 SP1 | MSVC 13.10.6030 | x86 | static, dynamic | Build/API runtime validated on XP SP3 |
| Visual Studio .NET 2002 SP1 | MSVC 13.00.9466 | x86 | static, dynamic | Build/API runtime validated on XP SP3 |
| Visual C++ 6.0 SP6 | MSVC 12.00.8804 | x86 | static, dynamic | Build/API runtime validated on XP SP3 |

Each checkpoint must configure and compile the shared library with warnings as
errors, run the internal tests and the public-header consumer, and stage an SDK
containing the DLL, import library, public header, and license notices.

Each checked-in solution/workspace from VC6 through VS2026 is native to exactly one
IDE generation and selects its matching explicit CMake generator, preset, and
platform toolset. Import/upgrade compatibility is not treated as support. The
PowerShell dispatcher and direct presets cover the complete VS2008--VS2026
command-line matrix plus MinGW UCRT64/GCC x64.
An XP-native CMD dispatcher and separate CMake 3.5 definition cover the
x86-only VC6--VS2005 band. Package directory names include the compiler label,
so artifacts from different checkpoints can coexist.

## Remaining Windows validation

The `v140_xp` profile is compiler- and test-validated, but is not promoted to
runtime-validated support until its packages execute on real XP test hosts.
The native VS2017 and VS2026 files select their exact installed generators;
their command-line, compiler, and solution-project paths are validated on the
Windows 11 compatibility host.

The root `build.ps1` is the stable modern command-line dispatcher. Native IDE
files live under `ide/visual-studio/vsYYYY/` (and `vc6/`); every generation has
its own solution/project pair even where a newer IDE could import an older format.
Every band references the same root source files, so there are no source forks.

PowerShell is the supported human-facing dispatcher for VS2008 and newer and
for MinGW UCRT64. The
XP-era band uses `scripts/windows/legacy/build.cmd` because PowerShell is not a native
VC6-era dependency. Both paths compile the same canonical source files; period
IDE bands will not introduce source forks.

## Linux checkpoints

Native glibc builds are maintained with GCC and Clang. Reproducible Debian 10
packages enforce a GLIBC 2.28 ceiling, and Alpine packages exercise GCC and
Clang against musl. See `building.md` for the corresponding Make targets.
GNU Make is the stable human-facing Linux dispatcher (`make help`); it selects
compiler, backend, test, native package, glibc-portable, and musl workflows
while CMake remains the underlying build graph.
