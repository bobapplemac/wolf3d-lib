# Compiler support

wolf3d-lib treats compiler support as a tested compatibility matrix rather
than assuming that one successful modern build establishes portability.

For the user-facing cross-platform build, artifact, and destination-OS table,
see [support-matrix.md](support-matrix.md). This file retains the detailed
compiler-checkpoint policy and evidence.

## Validated Windows checkpoints

| Environment | Toolset | Architectures | Runtime modes | Status |
| --- | --- | --- | --- | --- |
| Visual Studio 2022 17.14 | v143 / MSVC 19.44 | x86, x64 | static, dynamic | Validated |
| Visual Studio 2019 16.11 | v142 / MSVC 19.29 | x86, x64 | static, dynamic | Validated |
| VS2017 toolset hosted by VS2019 16.11 | v141 / MSVC 19.16 | x86, x64 | static, dynamic | Validated |
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

The checked-in modern solution detects VS2015, VS2019, or VS2022 and selects
the matching explicit CMake preset and platform toolset. The PowerShell
dispatcher and direct presets cover the complete VS2008--VS2022 matrix.
An XP-native CMD dispatcher and separate CMake 3.5 definition cover the
x86-only VC6--VS2005 band. Package directory names include the compiler label,
so artifacts from different checkpoints can coexist.

## Planned Windows checkpoints

The v90 through v120 compilers are validated, but their checked-in native IDE
solution bands remain follow-up work. The `v140_xp` profile is also compiler-
and test-validated, but is not promoted to runtime-validated support until its
packages execute on real XP test hosts.

The root `build.ps1` is the stable modern command-line dispatcher. The root
solution remains while one format is genuinely accepted by the validated IDE
range. When testing finds a real compatibility boundary, solution and project
files will move into explicit bands under `ide/visual-studio/`, for example
`vs2019-vs2022` or `vs2015-vs2017`; every band references the same root source
files. No speculative duplicate solutions or source forks are created.

PowerShell is the supported human-facing dispatcher for VS2008 and newer. The
XP-era band uses `scripts/build-legacy.cmd` because PowerShell is not a native
VC6-era dependency. Both paths compile the same canonical source files; period
IDE bands will not introduce source forks.

## Linux checkpoints

Native glibc builds are maintained with GCC and Clang. Reproducible Debian 10
packages enforce a GLIBC 2.28 ceiling, and Alpine packages exercise GCC and
Clang against musl. See `building.md` for the corresponding Make targets.
GNU Make is the stable human-facing Linux dispatcher (`make help`); it selects
compiler, backend, test, native package, glibc-portable, and musl workflows
while CMake remains the underlying build graph.
