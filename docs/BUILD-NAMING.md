# Build and distribution naming

This is the shared naming contract for wolf3d-lib and wolf3d-portable. Keep
this document and the distribution helpers identical in both repositories.

## Distribution directories and archives

```text
dist/wolf3d-lib_<version>_<platform>_<arch>_<toolchain>[_configuration][_runtime]/
dist/wolf3d-portable_<version>_<platform>_<arch>_<backend>_<toolchain>[_configuration][_runtime]/
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
Use the compiler's target architecture, not the build host's. A 64-bit pointer
does not by itself prove x64: unknown architectures must fail naming validation.
CPU instruction requirements belong in build-info unless separate CPU variants
are deliberately added to the distribution matrix.

## Backend vocabulary (portable only)

| Label | Implementation |
| --- | --- |
| `gdi` | Windows GDI graphics, Windows input and waveOut audio |
| `sdl3` | SDL3 platform implementation |
| `vga` | DOS VGA mode 13h implementation |
| `kms-fbdev` | Linux DRM/KMS with fbdev fallback, evdev input and ALSA audio |

Linux console graphics are not VGA mode 13h and do not render terminal text.
The library has no backend field. Existing source directories, executable
names, targets (`win32-release`, `console-release`), and command-line options
retain their names: package labels do not rename those public interfaces.

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
wolf3d-portable_1.4.73_win7_x64_gdi_msvc-v120
wolf3d-portable_1.4.73_win10_x64_sdl3_mingw-gcc14-ucrt
wolf3d-portable_1.4.73_win9x_x86_gdi_openwatcom2
wolf3d-portable_1.4.73_dos32_x86_vga_openwatcom2
wolf3d-portable_1.4.73_linux-glibc_x64_kms-fbdev_gcc14
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
