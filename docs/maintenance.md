# Repository maintenance audit

The 2026-10-09 review covered tracked first-party build scripts, tools, templates,
CMake inputs and test fixtures. Textual reference scanning found no supported
reason to delete an asset. Matching a basename is only evidence of a reference,
not proof that code is reachable; absence of a match is a review lead, not a
removal instruction. Vendored code, ignored outputs and historical proposals
are excluded from the scan.

## Run the checks

```sh
python tools/WG_MAINTENANCE_AUDIT.py --peer ../OTHER_REPOSITORY
python tools/WG_MAINTENANCE_AUDIT.py --peer ../OTHER_REPOSITORY --references
python tests/WG_MAINTENANCE_TEST.py
```

Replace OTHER_REPOSITORY with wolf3d-lib or wolf3d-portable as appropriate.
Use the separate companion checkout at the matching development revision;
portable's intentionally older engine submodule is not the current mirror baseline.
Without --peer, local checks still run and mirror comparison is explicitly skipped.
Nothing is fetched, synchronized, deleted or rewritten by the audit.

Python is required only for these explicitly invoked maintainer checks. No Python
step was added to build.sh, build.ps1, build.cmd, Make, CMake or the game runtime.

## Mirrored files

[maintenance-inventory.json](../tools/maintenance-inventory.json) is the exact
inventory and editing-ownership record. Its mirrors.paths list includes shared
CMake/package helpers, Git preflight helpers, dispatch shims, notice fragments,
regression checks and shared documentation. Edit the declared owner first, copy
the reviewed change to the peer, and commit both. Equality ignores CRLF versus LF
only; differences in content or missing files fail. Repository-specific build
frontends are deliberately not mirrors merely because their filenames match.

The glibc auditors were equivalent apart from formatting and diagnostics. They
now share the library version, which distinguishes an empty package from a
package without glibc symbol versions, and are covered by the mirror inventory.

## Generated and maintained files

| Assets | Source of truth | Check / update |
| --- | --- | --- |
| Open Watcom .wpj/.tgt descriptors | The repository's tools/*GENERATE_OPENWATCOM_IDE.py; library core inputs come from cmake/WGCoreSources.txt | Run the generator with --check; omit --check to regenerate. --output-dir allows isolated test output. |
| Visual Studio .sln/.vcxproj/.vcproj/.dsp/.dsw | Checked-in, version-specific native descriptors; there is no in-repository VS generator | Maintain deliberately; run tests/WG_LEGACY_IDE_TEST.py. Serialized-format warning comments do not establish generator ownership. |
| Library src/WG_SIGNON_ASSETS.inc | tools/GENERATE_SIGNON_ASSETS.ps1 plus five original external SIGNON binaries | Regenerate to a temporary OutputPath with supplied SourceDirectory, then compare; automatic regeneration is explicitly skipped without those inputs. |
| Library src/WG_VIEW_TABLES.inc | Frozen deterministic reference values, documented in architecture/provenance | Preserve and validate through engine regression tests; no checked-in regeneration recipe exists. |
| CMake build files and dist payloads | CMake, packaging templates and scripts | Generated under ignored build/ and dist/; not checked-in mirror candidates. |

Open Watcom checks compare expected serialized content without rewriting even
when drift is found. Regression tests exercise matching, missing and modified
outputs and verify file bytes and modification times are unchanged by --check.
The portable check also compares DOS release source paths with the serialized IDE
source inventory. This caught and corrected a missing platforms/WG_HELP.c entry
in the game target. The .tgt file was regenerated from the corrected generator.

## Reviewed apparent orphans

- packaging/notices fragments are selected dynamically by component name. Some
  are unused by one project but belong to the shared fragment set.
- tests/WG_BUILD_ENV_TEST.ps1 checks dispatcher PATH restoration and requires a
  usable MSYS2 installation; invoke with PowerShell and -Msys2Root when needed.
- tests/WG_LEGACY_IDE_TEST.py checks native project references and uses recording
  stand-ins for legacy dispatchers; Windows-specific cases require CMD/PowerShell.
- Portable tests/WG_WIN32_LEGACY_SDK_TEST.py needs MinGW (W3P_MINGW_CC can select it).
- tests/WG_MAINTENANCE_TEST.py is the explicit generator/mirror audit regression.

These tools remain supported manual checks, recorded in the inventory rather
than deleted for lacking a build-system caller. New unexplained reference leads
fail the audit and require review. Keep this inventory current when adding,
removing or changing ownership of shared helpers or generated assets.
