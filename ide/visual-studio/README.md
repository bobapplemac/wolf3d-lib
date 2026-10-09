# Visual Studio compatibility bands

Each directory contains the native solution/project pair for exactly one
Visual Studio IDE generation. All projects reference the same root source
tree and invoke the matching CMake generator/toolset; none contains an engine
fork.

| Directory | IDE | Toolset |
| --- | --- | --- |
| `vs2026/` | Visual Studio 2026 | v145 |
| `vs2022/` | Visual Studio 2022 | v143 |
| `vs2019/` | Visual Studio 2019 | v142 |
| `vs2017/` | Visual Studio 2017 | v141 |
| `vs2015/` | Visual Studio 2015 | v140 |
| `vs2013/` | Visual Studio 2013 | v120 |
| `vs2012/` | Visual Studio 2012 | v110 |
| `vs2010/` | Visual Studio 2010 | v100 |
| `vs2008/` | Visual Studio 2008 | v90 |
| `vs2005/` | Visual Studio 2005 | MSVC 14.00 |
| `vs2003/` | Visual Studio .NET 2003 | MSVC 13.10 |
| `vs2002/` | Visual Studio .NET 2002 | MSVC 13.00 |
| `vc6/` | Visual C++ 6.0 | MSVC 12.00 |

"Supported" here means that the solution opens and builds in the named IDE
without an upgrade/conversion prompt. Newer IDEs may be able to import an
older project, but that compatibility path is deliberately not the supported
workflow. VS2008 and newer use `.sln` plus the native project format for that
generation. VC6 uses its period `.dsw`/`.dsp` pair. The pre-VS2008 projects
delegate compilation to the XP-native dispatcher under `scripts/windows/`
while remaining directly buildable from their matching IDE.

Legacy VC6--VS2005 builds require CMake 3.5. The build helpers check the
existing build cache, PATH, and standard CMake installation directories.
For a custom location, set `WOLF3D_LEGACY_CMAKE` to the full `cmake.exe` path
before opening the IDE. This overrides automatic discovery.
