# Visual Studio compatibility bands

Each directory contains solution/project files known to share one Visual
Studio file format and CMake-backed workflow. All bands reference the same
root source tree; none contains an engine fork.

| Directory | Supported IDEs/toolsets |
| --- | --- |
| `vs2015-vs2022/` | Visual Studio 2015/v140, 2019/v142, and 2022/v143 |

VS2008--VS2013 are currently supported through the root PowerShell dispatcher
and CMake-generated solutions. VC6--VS2005 use the XP-native dispatcher under
`scripts/windows/legacy/`. A checked-in IDE band is added only when empirical
testing establishes a useful compatible file-format range.
