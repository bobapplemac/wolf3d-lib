# Repository layout

`wolf3d-lib` owns the portable engine, public ABI, deterministic oracle,
tests, and library packaging. Production host wrappers live in the companion
`wolf3d-portable` repository.

| Path | Contents |
| --- | --- |
| `src/` | Portable engine. Original `WL_*` and `ID_*` filenames remain comparable with the DOS source; genuinely new modules use `WG_*`. |
| `include/` | Public C99 ABI, currently `WOLF3D.h`. |
| `platforms/headless/` | Internal deterministic validation host. |
| `platforms/WG_TEXT_OUTPUT.*` | Shared diagnostic text helper used by internal validation. |
| `tests/` | Unit, archive, demo, rendering, timing, and data regression suite. |
| `third_party/Nuked-OPL3/` | Replaceable LGPL OPL emulator source. |
| `packaging/` | Library-package template and portable Linux builder. |
| `tools/` | Maintainer generators and release/audit tools. |
| `docs/` | Architecture, building, porting, provenance, supported-data, and historical development records. |
| `VERSION` | Authoritative `1.4.REVISION` library identity. |
| `wolf3d-lib.sln` | Current shared Visual Studio entry point; compatibility-banded solutions move under `ide/visual-studio/` only when empirically required. |
| `build.ps1` | Modern Windows toolchain detector and human-facing build dispatcher. |
| `build/` | Ignored compiler output and local diagnostics. |
| `dist/` | Ignored clean library SDK packages. |

## Stable build trees

| Preset or command | Directory |
| --- | --- |
| `windows-dev-x64` | `build/windows-vs2019-dev-x64` |
| `windows-dev-x86` | `build/windows-vs2019-dev-x86` |
| `windows-library-x64` | `build/windows-vs2019-library-x64` |
| `windows-library-x86` | `build/windows-vs2019-library-x86` |
| `windows-vs2022-dev-x64` | `build/windows-vs2022-dev-x64` |
| `windows-vs2022-dev-x86` | `build/windows-vs2022-dev-x86` |
| `windows-vs2022-library-x64` | `build/windows-vs2022-library-x64` |
| `windows-vs2022-library-x86` | `build/windows-vs2022-library-x86` |
| `linux-dev` | `build/linux-dev` |
| `linux-library` | `build/linux-library` |
| `make ... CC=gcc` | `build/linux-gcc` or `build/linux-library-gcc` |
| `make ... CC=clang` | `build/linux-clang` or `build/linux-library-clang` |

Game-derived PPM/WAV diagnostics stay in ignored build folders and are never
part of a library distribution.
