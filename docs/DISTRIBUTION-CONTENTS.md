# Distribution contents

This contract applies to wolf3d-lib and wolf3d-portable, including modern CMake,
legacy Visual Studio, Open Watcom, glibc and relocatable musl distributions.
See [BUILD-NAMING.md](BUILD-NAMING.md) for directory naming.

## Layout

```text
<primary binaries, libraries and public headers>
README.TXT
DOCS/
    BUILD.TXT
    NOTICES.TXT
    LICENSES/
        GPL-2.TXT
        LGPL-21.TXT    (when applicable)
        SDL3.TXT        (when shipped)
        ...
```

Portable distribution executables are named wolf3d on Unix and wolf3d.exe on
Windows, with WOLF3D.EXE for DOS and Win9x. The package directory identifies
the backend; do not add a backend suffix to the distributed executable.
Musl's top-level wolf3d launcher invokes bin/wolf3d. Internal CMake target and
build-tree artifact names may retain backend suffixes to allow simultaneous
backend builds without collisions. Library/DLL names are unchanged.

Keep runtime loader paths intact. Musl additionally needs bin/, lib/ and share/;
Open Watcom library packages retain include/. DOS packages that statically link
Nuked retain their RELINK/ kit and its instructions. Those are functional payloads.

README.TXT is the starting point for running the application or integrating the
library: data placement, platform requirements, options, and documentation links.
BUILD.TXT is the authoritative build record: product/engine versions, source
revisions, compiler, every configured option, cache/target settings or complete
Open Watcom recipe, and any bundle-specific settings. Do not replace it with an
audio-only summary. Never dump unrelated environment variables or credentials.
NOTICES.TXT identifies included components, copyright, provenance/modifications,
license mapping, source locations and replacement/relinking instructions.
LICENSES contains complete unmodified license texts and component copyright
collections. Share identical standard license texts, but preserve each component's
copyright and map its license explicitly in NOTICES.TXT.

## Rules for adding a dependency or packager

- Root documentation is limited to README.TXT and DOCS/. Do not add independent
  component readmes or duplicate version/build summaries at the root.
- Use uppercase ASCII 8.3 names for distributed documentation on all platforms.
  README.TXT, BUILD.TXT and NOTICES.TXT remain plain text, readable without tools.
- Include notices for actual package contents. A compiler/tool used during a
  build belongs in BUILD.TXT; a runtime library shipped in the bundle also belongs
  in NOTICES.TXT. Disabled optional components must not appear as included.
- Keep runtime configuration with its runtime, never in DOCS/.
- Preserve source availability and LGPL replacement/relinking provisions. Merely
  supplying notices or an upstream URL does not complete release obligations.
- Stage into a freshly recreated package directory so old options cannot leave
  stale binaries or notices. Do not rewrite or clean unrelated distributions.
- Keep cmake/WGPackageDocs.cmake, scripts/package-docs.sh and packaging/notices
  mirrored in the two repositories. Both implementations consume the same notice
  fragments. Shell builds need no Python or CMake dependency for documentation.
- Run tests/WG_PACKAGE_DOCS_TEST.py with Python as a developer regression check;
  Python is not a build/runtime requirement. Validate real packages for affected
  backends and preserve license text bytes when copying.

Repository source documentation and licenses retain their normal source names.
Internal build-tree metadata may retain BUILD-INFO.txt names; the published
package contract is DOCS/BUILD.TXT. WOLF3D-LIB.txt and README.DBOPL.txt no longer
ship separately: their information is merged into BUILD.TXT and NOTICES.TXT.
