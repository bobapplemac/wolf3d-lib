# Repository layout

The source tree separates the portable engine, host wrappers, public API,
tests, documentation, packaging metadata, and vendored dependencies. Generated
files live under `build/` or `dist/` and are ignored by Git.

| Path | Contents |
| --- | --- |
| `src/` | Portable engine implementation. It remains deliberately flat so original `WL_*` and `ID_*` filenames can be compared directly with the DOS source; genuinely new modules use `WG_*`. `WG_SIGNON_ASSETS.inc` embeds the original executable-linked startup templates, while `WG_VIEW_TABLES.inc` freezes verified renderer and angle tables. |
| `include/` | Public shared-library API. |
| `platforms/` | Thin platform hosts and their callback implementations. |
| `tests/` | Deterministic unit and archive regression suite. |
| `third_party/` | Vendored, independently licensed dependencies. |
| `packaging/` | Staged-package templates and the digest-pinned Debian 10 portable-release container. |
| `tools/` | Maintainer-side generators and release audits, including reproducible SIGNON embedding and the glibc symbol-floor gate. |
| `Makefile` | GNU Make convenience wrapper around the authoritative CMake targets. |
| `VERSION` | Authoritative `1.4.REVISION` product identity consumed by every build and package path. |
| `wolf3dgeneric.sln` | Source-owned Visual Studio entry point for Win32/x64 Debug and Release builds. |
| `ide/visual-studio/` | Maintained Visual Studio project metadata; compilation delegates to CMake. |
| `docs/` | Architecture, porting, provenance, versioning, completed development plan, development log, and release notes. |
| `CHANGELOG.md` | Root-level, user-facing release and post-release history. |
| `THIRD_PARTY.md` | Root-level dependency and licensing index copied into every staged package. |
| `build/` | All local compiler output and generated diagnostic artifacts. |
| `dist/` | Clean folders produced by the Win32, SDL3, Linux-console, and library release targets. |

## Preset build trees

`CMakePresets.json` gives each supported configuration one stable directory:

| Preset | Build directory | Purpose |
| --- | --- | --- |
| `windows-dev-x64` | `build/windows-dev-x64` | Full 64-bit hosts and tests. |
| `windows-dev-x86` | `build/windows-dev-x86` | Full 32-bit hosts and tests. |
| `windows-release-x64` | `build/windows-release-x64` | Minimal 64-bit Win32 runtime package. |
| `windows-release-x86` | `build/windows-release-x86` | Minimal 32-bit Win32 runtime package. |
| `windows-library-x64` | `build/windows-library-x64` | Host-free 64-bit engine package. |
| `windows-library-x86` | `build/windows-library-x86` | Host-free 32-bit engine package. |
| `linux-dev` | `build/linux-dev` | Native Linux engine, headless host, and tests. |
| `linux-library` | `build/linux-library` | Host-free Linux shared-library package. |
| `linux-console` | `build/linux-console` | Minimal DRM/evdev/ALSA console-host package. |
| GNU Make with GCC | `build/linux-gcc` | Native GCC engine, headless host, and tests. |
| GNU Make with Clang | `build/linux-clang` | Native Clang engine, headless host, and tests. |

Generated milestone frames, raw PPM captures, and WAV diagnostics live in
`build/artifacts/`. They can contain copyrighted original-game imagery or
audio, so they remain local and are never committed or distributed. The
development log links to those local files for rendered milestone inspection.

Staged packages are written directly to architecture-labelled folders under
`dist/`; compiler objects, tests, caches, and diagnostic captures never enter
those folders.

The checked-in Visual Studio solution delegates its eight platform/configuration
combinations to the matching CMake development trees. CMake's additional
generated solution and project files remain local products under `build/`.
