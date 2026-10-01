# Compiler support

wolf3d-lib treats compiler support as a tested compatibility matrix rather
than assuming that one successful modern build establishes portability.

## Validated Windows checkpoints

| Environment | Toolset | Architectures | Runtime modes | Status |
| --- | --- | --- | --- | --- |
| Visual Studio 2022 17.14 | v143 / MSVC 19.44 | x86, x64 | static, dynamic | Validated |
| Visual Studio 2019 16.11 | v142 / MSVC 19.29 | x86, x64 | static, dynamic | Validated |
| VS2017 toolset hosted by VS2019 16.11 | v141 / MSVC 19.16 | x86, x64 | static, dynamic | Validated |
| Visual Studio 2015 14.0 | v140 / MSVC 19.00.23506 | x86, x64 | static, dynamic | Validated |

Each checkpoint must configure and compile the shared library with warnings as
errors, run the internal tests and the public-header consumer, and stage an SDK
containing the DLL, import library, public header, and license notices.

The checked-in solution detects VS2015, VS2019, or VS2022 and selects the
matching explicit CMake preset and platform toolset. Package directory names
include the toolset label, so artifacts from different checkpoints can coexist.

## Planned Windows checkpoints

Older toolsets and IDEs will be introduced one checkpoint at a time, with
separate project files only when an older MSBuild format actually requires
them. Visual Studio 2015 is currently the oldest validated Windows checkpoint.

The root `build.ps1` is the stable modern command-line dispatcher. The root
solution remains while one format is genuinely accepted by the validated IDE
range. When testing finds a real compatibility boundary, solution and project
files will move into explicit bands under `ide/visual-studio/`, for example
`vs2019-vs2022` or `vs2015-vs2017`; every band references the same root source
files. No speculative duplicate solutions or source forks are created.

Legacy Windows systems are not required to provide PowerShell or modern
CMake. A compatibility band may include a small period-appropriate `.cmd`
launcher that invokes its native `vcbuild`, `devenv`, or `msbuild` workflow.
Those launchers remain local to the band instead of accumulating into one
unmaintainable universal batch file.

## Linux checkpoints

Native glibc builds are maintained with GCC and Clang. Reproducible Debian 10
packages enforce a GLIBC 2.28 ceiling, and Alpine packages exercise GCC and
Clang against musl. See `building.md` for the corresponding Make targets.
GNU Make is the stable human-facing Linux dispatcher (`make help`); it selects
compiler, backend, test, native package, glibc-portable, and musl workflows
while CMake remains the underlying build graph.
