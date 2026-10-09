# Repository layout

wolf3d-lib owns the reusable engine, public API and deterministic validation.
Playable hosts belong in wolf3d-portable.

| Path | Responsibility |
| --- | --- |
| `src/` | Private engine implementation; original ID_/WL_ filenames preserve source correspondence. |
| `include/` | Public ABI headers, including legacy fixed-width-type compatibility. |
| `compat/` | Private compiler compatibility support. |
| `platforms/` | Internal headless validation host and diagnostic helpers; playable hosts live in portable. |
| `third_party/` | Vendored Nuked-OPL3 and DBOPL, with their upstream provenance. |
| `LICENSES/` | Historical licensing evidence; distinct from licenses assembled into binary packages. |
| `.github/` | CI and repository automation. |
| `cmake/` | Build graph helpers; legacy/ isolates CMake 3.5-era generators. |
| `docs/` | User and developer guides; README.md is the navigation and ownership index. |
| `ide/` | Version-specific native IDE descriptors; generated workspace caches stay ignored. |
| `packaging/` | Container definitions, package templates, license resources and notice fragments. |
| `scripts/` | Build entry-point implementations and packaging orchestration, grouped by host/toolchain. |
| `tests/` | Runtime, API, build and packaging regression fixtures; external game data remains external. |
| `tools/` | Focused audits and generators; scripts may invoke these during validation. |
| `build/` | Ignored intermediate binaries, logs, local diagnostics and caches. |
| `dist/` | Ignored complete redistributable packages; layout is a separate published contract. |

Root build.sh, build.ps1 and build.cmd are stable user entry points. Makefile,
CMakeLists.txt and CMakePresets.json remain at the root for their tools. README,
CHANGELOG, LICENSE and THIRD_PARTY are discoverable repository metadata.
VERSION is the authoritative engine revision.

## Placement and maintenance rules

- Keep source paths stable: native IDE descriptors, scripts and tools reference
  them. A backend vocabulary change does not require renaming its source directory.
- Put orchestration in scripts/, standalone audits/generators in tools/, and
  test fixtures in tests/. A tool used by packaging may remain in tools/.
- Put Dockerfiles, distribution text and licensing inputs in packaging/; keep
  runtime host implementation in platforms/ and CMake logic in cmake/.
- Do not move vendored dependencies or their upstream documents for visual tidiness.
- Generated outputs belong in ignored build/ or dist/, never among tracked source.
- Keep the root README short. Each topic has an owning guide in the
  [documentation index](README.md); link there instead of duplicating option tables.
- Archive completed plans and checkpoint evidence rather than presenting them as
  current support claims. Keep original source/provenance records intact.

[Build naming](BUILD-NAMING.md) defines output directory identities;
[distribution contents](DISTRIBUTION-CONTENTS.md) defines the package payload.
Source-tree organization and distribution organization serve different readers.
