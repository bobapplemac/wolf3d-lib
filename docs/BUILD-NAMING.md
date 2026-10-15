# Build and distribution naming

This is the shared naming contract for wolf3d-lib and wolf3d-portable. Keep
this document and the distribution helpers identical in both repositories.

## Distribution directories and archives

```text
dist/wolf3d-lib_<version>_<platform>_<arch>_<toolchain>[_configuration][_runtime]/
dist/wolf3d-portable_<version>_<platform>_<arch>_<toolchain>[_configuration][_runtime]/
```

Use lowercase ASCII. Underscores separate fields; hyphens join components
inside a field. Versions may contain dots. Keep packages flat under `dist/`.
Archives use the same basename as the directory they contain. Names describe
the output target, never the build machine, IDE, or invocation method.

The product is `wolf3d-lib` or `wolf3d-portable`. The version is the engine's
`VERSION` value, also used by portable. Both repository revisions are recorded
in `DOCS/BUILD.TXT`; the version alone does not identify an exact source tree.

## Platform vocabulary

| Label | Target |
| --- | --- |
| `dos32` | 32-bit protected-mode DOS |
| `win9x` | Windows 95/98/Me family |
| `win2k` | Windows 2000 baseline; canonical shorthand for identifiers and paths (x86, GDI) |
| `winxp` | Windows XP baseline |
| `win7` | Windows 7 baseline |
| `win10` | Windows 10 baseline |
| `linux-glibc` | Linux with glibc |
| `linux-musl` | Linux with musl |

Windows labels describe a support baseline, not an assertion that the binary
cannot run on anything older. Do not append `+`. Architecture, service packs,
installed runtimes, and dependencies still constrain compatibility. A label
is not a substitute for PE/ELF audits and runtime testing on the target OS.

Default CMake naming policy is conservative: legacy MSVC through v100 and
explicit XP toolsets use `winxp`; v110 through v141 without the XP toolset use
`win7`; v142 and later and native MinGW UCRT use `win10`. Explicit Windows
cross profiles supply their XP/7/10 baseline. Native modern Windows SDL3 profiles use `win10`; explicit Windows 7
cross profiles retain `win7`. Open Watcom Windows packages use `win9x`. These defaults do not
lower linker/SDK requirements or enable features by themselves.

`WG_DIST_PLATFORM` explicitly selects an existing platform label for a custom
profile. Verify all bundled dependencies before lowering a baseline. SDL3 has no supported XP/Win9x profile. `WG_LINUX_LIBC=glibc|musl` selects the
Linux ABI label; absent an explicit selection, CMake recognizes target glibc
and rejects an unknown libc rather than guessing. Musl recipes pass `musl`.

## Architecture vocabulary

| Label | Meaning |
| --- | --- |
| `x86` | 32-bit Intel/AMD |
| `x64` | 64-bit Intel/AMD |
| `arm64` | 64-bit ARM; vocabulary reserved for future supported profiles |

Do not mix synonyms such as `amd64`, `x86_64`, or `i386` into package names.
Docker Linux defaults to `x64`. The portable legacy glibc profile uses `x86`
with an i686/Pentium Pro minimum (no MMX/SSE/SSE2 requirement); `x86` does not
promise original Pentium/i586 compatibility. Record CPU flags and baseline in
BUILD.TXT. The musl Docker profile remains x64 only.
Use the compiler's target architecture, not the build host's. A 64-bit pointer
does not by itself prove x64: unknown architectures must fail naming validation.
CPU instruction requirements belong in build-info unless separate CPU variants
are deliberately added to the distribution matrix.

## Compiled video backend vocabulary (build info and runtime only)

| Label | Implementation |
| --- | --- |
| `gdi` | Windows GDI graphics, Windows input and waveOut audio |
| `sdl1` | SDL 1.2 legacy platform implementation |
| `sdl3` | SDL3 platform implementation |
| `vga` | DOS VGA mode 13h implementation |
| `kms-fbdev` | Linux DRM/KMS with fbdev fallback, evdev input and ALSA audio |

Linux console graphics are not VGA mode 13h and do not render terminal text.
Neither product has a video-backend field in its distribution name. Portable
packages contain one executable with the selected compiled backends; `--backend`
selects among them at runtime. `DOCS/BUILD.TXT` and `--diag` record the selection.
SDL1 and SDL3 are mutually exclusive. There is no `_modular` suffix.

Rebuilding the same product/version/platform/architecture/toolchain/configuration
with a different backend subset replaces that package, including its dependencies.
It does not produce a second sibling folder. Keep copies outside `dist/` when
comparing alternate configurations. Build directories may still identify backend
subsets so their object files and caches remain independent.

The experimental Red Hat 9/i586 package uses the compatibility variant `_rh9`
after the toolchain: its glibc 2.3.2/kernel 2.4 baseline differs from the modern
glibc 2.28/i686 GCC 8 package. This prevents an otherwise identical name collision.

## Toolchain vocabulary

| Pattern | Meaning |
| --- | --- |
| `msvc-6.0`, `msvc-7.0`, `msvc-7.1` | Legacy Microsoft compiler generations |
| `msvc-v80`, `msvc-v90`, `msvc-v100`, `msvc-v110`, `msvc-v120` | Microsoft platform toolsets |
| `msvc-v140`, `msvc-v141`, `msvc-v142`, `msvc-v143`, `msvc-v145` | Microsoft platform toolsets |
| `gcc<N>` | GCC major version, e.g. `gcc14` |
| `clang<N>` | Clang major version, e.g. `clang19` |
| `mingw-gcc<N>-msvcrt` | MinGW GCC major version, MSVCRT |
| `mingw-gcc<N>-ucrt` | MinGW GCC major version, UCRT |
| `llvm-mingw<N>-msvcrt` | LLVM-MinGW with Clang major version, MSVCRT |
| `llvm-mingw<N>-ucrt` | LLVM-MinGW with Clang major version, UCRT |
| `openwatcom<N>` | Open Watcom major version, e.g. `openwatcom2` |

Detect versions from the selected compiler. Never encode the Visual Studio IDE
version: one IDE can select multiple toolsets. An XP toolset's exact identifier
(e.g. `v140_xp`) belongs in build-info, with `winxp` in the platform field.
Record full compiler versions and LLVM-MinGW distribution identifiers inside
the package. `WG_DIST_CRT=msvcrt|ucrt` is required for custom MinGW toolchains
when it cannot be determined from an existing profile label. Existing
`WG_COMPILER_LABEL`/`W3P_COMPILER_LABEL` inputs remain accepted for CRT hints;
they no longer override the detected compiler identity.

### Microsoft version vocabulary

Use the toolset generation when discussing build choices. The IDE's marketing
name, the compiler executable's version, and runtime versions are separate.
Our matrices label their combined column **Toolset / compiler** and retain
exact compiler versions as validation evidence.

| Environment | Toolset generation label | Compiler version family | `_MSC_VER` | Package toolchain field |
| --- | --- | --- | --- | --- |
| Visual C++ 6.0 | `v60` | 12.00 | 1200 | `msvc-6.0` |
| Visual Studio .NET 2002 | `v70` | 13.00 | 1300 | `msvc-7.0` |
| Visual Studio .NET 2003 | `v71` | 13.10 | 1310 | `msvc-7.1` |
| Visual Studio 2005 | `v80` | 14.00 | 1400 | `msvc-v80` |
| Visual Studio 2008 | `v90` | 15.00 | 1500 | `msvc-v90` |
| Visual Studio 2010 | `v100` | 16.00 | 1600 | `msvc-v100` |
| Visual Studio 2012 | `v110` | 17.00 | 1700 | `msvc-v110` |
| Visual Studio 2013 | `v120` | 18.00 | 1800 | `msvc-v120` |
| Visual Studio 2015 | `v140` | 19.00 | 1900 | `msvc-v140` |
| Visual Studio 2017 | `v141` | 19.10-19.16 | 1910-1916 | `msvc-v141` |
| Visual Studio 2019 | `v142` | 19.20-19.29 | 1920-1929 | `msvc-v142` |
| Visual Studio 2022 | `v143` | 19.30-19.44 | 1930-1944 | `msvc-v143` |
| Visual Studio 2026 | `v145` | 19.50+ (14.5x build tools) | 1950+ | `msvc-v145` |

These rows describe the corresponding toolset generation, not every toolset an
IDE can install. VS2026 separates compiler release cadence from IDE releases;
record the actual compiler/build-tools version in BUILD.TXT rather than deriving
it from the IDE year. Future compiler families need explicit mapping updates.


`v60`, `v70`, and `v71` are our shorthand for the legacy VC 6.0, 7.0, and
7.1 generations, not modern MSBuild `PlatformToolset` identifiers. The legacy
VC6--VS2005 build path selects the matching CMake generator and installed
tools; these labels do not imply that `-T v60` or `-T v80` works. Existing
package fields for the first three generations retain their dotted spelling.

- **Visual Studio release** identifies the IDE/product, such as VS2013
  (IDE 12.0). Modern IDEs can use multiple installed toolsets.
- **Toolset** identifies the selected build tools, such as `v120` or `v143`.
  Servicing updates can change the compiler version without changing this label.
- **Compiler version** is the `cl.exe` version, such as 18.00 for VS2013.
  `_MSC_VER` encodes its major/minor numbers as 1800. `_MSC_FULL_VER`
  (the correct macro spelling) adds build detail; `_MSC_BUILD`, where available,
  identifies the revision. Do not assume modern macro availability or digit
  widths in legacy compiler checks.
- **Runtime version** identifies the CRT/runtime libraries and redistributable,
  not the IDE or compiler. Since VS2015, multiple toolset generations share the
  v14 runtime family, subject to Microsoft's compatibility restrictions.
  Static/dynamic runtime linkage is a separate build choice. Record actual
  runtime dependencies and full compiler identity in `DOCS/BUILD.TXT`;
  the target OS still needs its own compatibility checks.

Microsoft documents the [compiler and product version mapping](https://learn.microsoft.com/en-us/cpp/overview/compiler-versions),
[predefined macros](https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros),
and [runtime binary compatibility and restrictions](https://learn.microsoft.com/en-us/cpp/porting/binary-compat-2015-2017).

## Optional fields

Configuration comes before runtime. Ordinary `Release` is implicit; other
standard configurations append `_debug`, `_relwithdebinfo`, or `_minsizerel`.
`Publish` is an IDE action that stages Release, not a package configuration.

MSVC's normal static compiler runtime is implicit; dynamic linking appends
`_dynamic-crt`. MinGW's normal static GNU runtime is implicit; disabling that
option appends `_dynamic-gcc-runtime`. These labels describe the selected build
option; exact compiler runtime libraries are recorded in build-info and checked
by the applicable import audit. Do not use ambiguous bare `static` or `shared`.
The engine DLL/shared-library ABI is distinct from compiler-runtime linkage.

**OPL driver selection, default OPL driver, and sample rate never appear in the
folder name.** Record all of them in build-info. The driver vocabulary is
`nuked`, `dbopl`, `silent`, and `adlib`, subject to target support.

Audio-only changes, compiler patch updates, or source changes within the same
release deliberately reuse the same directory. Release staging replaces that
directory's previous contents. Copy/archive an existing package first if you
need to preserve multiple such builds. Staging does not rename or delete older
packages that use the former naming convention.

## Build information

Every staged package includes generated `DOCS/BUILD.TXT`. It records the full
configuration, not just options visible in the folder name:

- Product/version, target platform/architecture/backend, configuration,
  compiler identity/full version, toolset, flags, and runtime linkage.
- Compiled OPL drivers, default OPL driver, and preferred sample rate.
- Available source/dependency revisions and modified tracked-source state.
- For CMake builds, the complete configured `CMakeCache.txt` and target-specific
  compile/link properties, including dependency settings. Generator expressions
  in target properties are retained verbatim; the active configuration is named
  separately. New cache options are automatically included.
- For Open Watcom builds, resolved options, compiler banner, relevant toolchain
  paths, definitions/libraries, and the complete build recipe with fixed
  compiler/linker switches. Portable also embeds the engine's build information.

The cache records local paths. Source archives without Git metadata record
unknown revisions. Build-info does not dump unrelated environment variables.
Musl bundling preserves this file and records the additional bundle recipe.

## Examples

```text
wolf3d-lib_1.4.73_linux-musl_x64_gcc14
wolf3d-lib_1.4.73_winxp_x86_msvc-v140_debug_dynamic-crt
wolf3d-portable_1.4.73_win7_x64_msvc-v120
wolf3d-portable_1.4.73_win10_x64_mingw-gcc14-ucrt
wolf3d-portable_1.4.73_win9x_x86_openwatcom2
wolf3d-portable_1.4.73_dos32_x86_openwatcom2
wolf3d-portable_1.4.73_linux-glibc_x64_gcc14
```

These illustrate vocabulary; they are not a promise that every combination is
a supported build. See the repository's build/support documentation for profiles.

## Build trees and extending the matrix

`build/` contains disposable working trees, not distribution packages. Existing
profile names (e.g. `legacy-vc6-all-nuked-static`) remain stable for IDE helpers,
caches, and developer workflows. Do not infer a distribution name from them.

`cmake/WGDistribution.cmake` owns CMake package identity in both modern and
legacy builds. It emits `<release-variable>-<configuration>.path` in the build
directory, relative to the product's source root. Audits and bundlers consume
these manifests instead of reconstructing compiler-dependent names. Open Watcom
uses `scripts/dist-openwatcom.sh`; its existing explicit output-directory
overrides remain available. No Python is required to build or package.

When adding a platform, architecture, backend, compiler, or package variant:

1. Define its meaning and spelling here before introducing an output name.
2. Update the shared helpers in both repositories. Reject unknown targets
   instead of silently assigning another architecture or compatibility label.
3. Add regression fixtures to `tests/WG_DIST_NAMING_TEST.py`, including unusual
   configurations and compiler hosts that differ from their targets.
4. Preserve complete build-info through staging, copying, bundling, and archives.
5. Exercise the actual package target and its compatibility audit. Check that
   Debug/Release and distinct runtime choices cannot overwrite one another.
6. Update current build/support examples. Historical changelog entries retain
   their original names.

Developer regression command: `python tests/WG_DIST_NAMING_TEST.py` with CMake
and a generator's build tool on PATH (or set `CMAKE` to its executable).

The [distribution contents contract](DISTRIBUTION-CONTENTS.md) defines the
README.TXT and DOCS layout, notices, licenses, and packaging extension rules.
