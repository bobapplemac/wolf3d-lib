# Changelog

Wolfenstein 3D's historically meaningful `1.4` product identity remains
stable. The third component is a monotonically increasing library revision;
beginning with 1.4.17, every commit to the library main branch advances it.
Revisions 1.4.1 through 1.4.16 are assigned retrospectively to the dated
post-promotion milestones below without rewriting Git history.

## 1.4.56 - 2026-10-08 - Engine-directed mouse capture

- Added an optional platform callback that synchronizes OS mouse capture with
  the engine's live Mouse Enabled configuration.
- Release capture during clean shutdown and immediately when mouse control is
  disabled from the Controls menu; restore it when control is re-enabled.

## 1.4.55 - 2026-10-08 - Host input-device discovery

- Added a platform capability callback for automatic mouse and joystick
  discovery while retaining deterministic command-line overrides.
- Changed `--mouse` and `--joy` to force hardware presence, and added
  `--nomouse` and `--nojoy` to force hardware absence.
- Reject contradictory force-on and force-off options instead of depending on
  argument order.

## 1.4.54 - 2026-10-07 - Public project maintenance

- Added structured issue forms for engine/fidelity defects and compiler or
  platform-portability reports.
- Added public issue-routing guidance while retaining general issues for
  reports that do not fit a predefined form.

## 1.4.53 - 2026-10-07 - Nuked-OPL3 provenance naming

- Renamed the vendored Nuked-OPL3 provenance note to use the public
  `wolf3d-lib` repository identity.
- Corrected its remaining reference to the former `wolf3dgeneric` project
  name without changing the upstream library or its LGPL terms.

## 1.4.52 - 2026-10-07 - DOS integration closure

- Reconciled the library documentation with the completed companion
  `wolf3d-portable` DOS/32A host rather than continuing to describe it as
  planned work.
- Recorded integrated DOSBox runtime validation of the Open Watcom engine with
  native AdLib, DBOPL, silent audio, SB16 PCM, VGA, keyboard, and PIT timing.
- Kept physical DOS hardware explicitly unclaimed until that optional testing
  is performed.

## 1.4.51 - 2026-10-07 - Clean Open Watcom IDE builds

- Excluded `ide2make`, object, diagnostic, response, and library outputs from
  source control while retaining the generated IDE descriptors themselves.
- Completed a release-mode `ide2make` and `wmake` validation of both tracked
  Open Watcom targets with the pinned 2026-10-01 toolchain.

## 1.4.50 - 2026-10-07 - Reproducible Open Watcom descriptors

- Made generated IDE descriptors byte-identical on Windows and Linux.
- Matched the generated library filename casing to Open Watcom's output so
  `ide2make` projects also validate on a case-sensitive maintainer host.
- Documented the native IDE's explicit Pentium/release selection rather than
  implying environment defaults could override its saved processor switch.

## 1.4.49 - 2026-10-07 - Native Open Watcom IDE workspace

- Added a DOS32 Open Watcom IDE workspace with separate `WOLF3D.LIB` and
  replaceable `NUKEDOPL.LIB` targets.
- Added a dependency-free Windows launcher that supplies the canonical include
  paths, Pentium code generation, and all four OPL driver definitions.
- Added deterministic descriptor generation from `WGCoreSources.txt`, avoiding
  manual editing of Open Watcom's opaque project serialization.

## 1.4.48 - 2026-10-07 - Optional native hardware OPL bridge

- Advanced the public platform API to v4 with optional hardware-OPL lifecycle
  and register-write callbacks, leaving raw port I/O entirely to a host.
- Added an `adlib` driver adapter for Open Watcom builds that advances the
  existing deterministic 700 Hz/sample clock while routing OPL2 writes to a
  DOS host and retaining PCM mixing for digitized and PC-speaker effects.
- Kept the hardware driver opt-in so normal Windows and Linux libraries retain
  the established Nuked, DBOPL, and silent defaults.

## 1.4.47 - 2026-10-07 - Clean Open Watcom diagnostics

- Disabled Open Watcom's source-adjacent `.err` report files while retaining
  all diagnostics on the console, preventing tolerated vendor warnings from
  dirtying a canonical checkout during Docker builds.

## 1.4.46 - 2026-10-07 - Open Watcom DOS32 library groundwork

- Added a pinned Docker-hosted Open Watcom 2 toolchain and Linux Make target
  for producing an OMF `WOLF3D.LIB` SDK for 32-bit protected-mode DOS.
- Made the core engine source inventory shared by modern CMake, legacy CMake,
  and the Open Watcom build so compiler bands cannot silently omit a module.
- Allowed the DOS32 library to compile any combination of Nuked-OPL3, DBOPL,
  and timing-preserving silent drivers, with all three included by default.
- Added a public-header consumer link check, guided Linux wizard integration,
  and an explicit build/runtime support boundary for the forthcoming DOS/32A
  host.

## 1.4.45 - 2026-10-07 - MSVC 2008 warning-clean compatibility

- Applied `_CRT_SECURE_NO_WARNINGS` to modern-CMake MSVC core targets so the
  intentional pre-C99 `_vsnprintf` compatibility wrapper remains warning-clean
  under VS2008 `/W4 /WX`, matching the established XP-era CMake path.

## 1.4.44 - 2026-10-07 - Windows runtime compatibility validation

- Validated the MinGW UCRT64 package, public API, deterministic tests, and all
  three runtime-selectable OPL drivers on the Windows 11 compatibility host.
- Validated the v140_xp x86 DLL, public API consumer, and deterministic test
  suite on Windows XP SP3.
- Replaced obsolete Windows 7 validation-host working copies with clean
  canonical clones after confirming their uncommitted legacy-build drafts were
  fully superseded by the committed compiler-matrix work.

## 1.4.43 - 2026-10-07 - MSYS2 bootstrap documentation

- Documented the complete fresh-install MSYS2 UCRT64 update and package setup
  needed by the MinGW build profile, including its PowerShell detection check.
- Clarified that the library does not require an MSYS2 SDL3 package.

## 1.4.42 - 2026-10-07 - Guided Windows build execution

- Corrected the final guided-build handoff to use named PowerShell parameter
  splatting, so accepting an interactive plan invokes the executor exactly as
  shown by the reproducible command.

## 1.4.41 - 2026-10-07 - Visual Studio 2017 and 2026 validation

- Added native Visual Studio 2026/v145 x86 and x64 presets, dispatcher
  detection, and a generation-specific solution/project pair.
- Corrected native Visual Studio 2017 discovery and made its command-line and
  IDE builds use a newer CMake when the IDE's bundled CMake is too old for
  presets.
- Validated the library and public API with the exact VS2017, VS2019, VS2022,
  and VS2026 compiler generations on the dedicated Windows 11 build host.

## 1.4.40 - 2026-10-06 - Custom audio package auditing

- Matched the GNU Make portable glibc and musl audit paths to the deterministic
  package suffix used by non-default driver sets, default drivers, and sample
  rates.

## 1.4.39 - 2026-10-06 - Collision-free custom audio packages

- Added deterministic package-directory suffixes for non-default compiled
  driver sets, runtime defaults, and preferred sample rates so custom audio
  builds cannot overwrite the canonical all-driver/Nuked/48 kHz package.

## 1.4.38 - 2026-10-06 - Runtime audio-driver selection

- Replaced mutually exclusive audio builds with a compiled-driver registry;
  default builds include Nuked-OPL3, DBOPL, and timing-preserving silence and
  accept `--opl nuked|dbopl|silent` at runtime.
- Added compile-time inclusion controls and default-driver selection to modern
  and legacy CMake, GNU Make, and the guided Windows/Linux build frontends.
- Added `--sample-rate`, a compile-time preferred rate, and platform API v3
  PCM negotiation so OPL synthesis and the exact rational 700 Hz sequencer use
  the application-facing rate actually accepted by the host.
- Exposed read-only driver discovery through the public API and added runtime
  coverage for each compiled driver at both 44.1 and 48 kHz.

## 1.4.37 - 2026-10-06 - Guided cross-platform build configuration

- Split the modern Windows build interface into a dependency-free guided
  configurator and a stable parameter-driven executor without changing
  existing root `build.ps1` automation commands.
- Added guided Bash and Windows XP CMD entry points that detect usable local
  toolchains, filter incompatible choices, print reproducible backend
  commands, and request confirmation before building.
- Kept external toolchain installation out of the configurators while adding
  verification and opt-in initialization of recorded Git submodules in the
  companion portable build flow.

## 1.4.36 - 2026-10-06 - MinGW UCRT64 support

- Added first-class MSYS2 UCRT64/GCC x64 CMake presets and PowerShell
  dispatcher support, including custom portable installations through
  `-Msys2Root`.
- Preserved the public Windows DLL names under MinGW and statically linked the
  GCC support runtime while retaining the separate `wolf3d.dll` and
  replaceable `Nuked-OPL3.dll` architecture.
- Built and packaged with GCC 16.2 under UCRT64, passed all three library
  tests, and verified that the artifacts import no MSYS, Cygwin, libgcc,
  libstdc++, or winpthread runtime DLLs.

## 1.4.35 - 2026-10-06 - VC6 Windows 11 runtime validation

- Recorded a successful manual run of the VC6-built x86 library as part of
  the Win32/GDI game package on Windows 11 x64 under WOW64, extending the
  same artifact's runtime evidence beyond its Windows XP SP3 validation.

## 1.4.34 - 2026-10-06 - Native Visual Studio generation matrix

- Replaced the shared compatibility-band solution with one native, toolset-
  pinned solution/project pair for every IDE generation from VS2002 through
  VS2022, plus a period-correct VC6 `.dsw`/`.dsp` workspace.
- Validated the checked-in VS2008--VS2013 projects on Windows 7 and the VC6--
  VS2005 projects on Windows XP through their exact IDE command-line hosts;
  retained the already-validated exact VS2015, VS2019, and VS2022 workflows.
- Pinned the VS2017 IDE project to the actual Visual Studio 15 generator rather
  than hosting v141 under VS2019; native VS2017 IDE smoke testing remains
  pending a matching installation.
- Made the XP-native dispatcher independent of an inherited nonzero
  `ERRORLEVEL`, which prevented makefile projects from rebuilding an existing
  build tree reliably.
- Documented that upgrade/import compatibility is not considered a supported
  substitute for opening each solution in its matching IDE.

## 1.4.33 - 2026-10-06 - Compatibility-banded build layout

- Moved the shared VS2015/VS2019/VS2022 solution and project into an explicit
  `ide/visual-studio/vs2015-vs2022` compatibility band while retaining the
  same CMake presets, build trees, and source browsing.
- Moved the XP-native VC6--VS2005 dispatcher to
  `scripts/windows/legacy/build.cmd` and established `scripts/linux/` as the
  home for Linux-hosted helpers such as the planned Open Watcom cross-build.
- Updated all user and maintainer documentation to identify stable root
  dispatchers separately from OS-specific scripts and IDE compatibility bands.

## 1.4.32 - 2026-10-06 - Build and compatibility matrices

- Added one authoritative user-facing matrix covering every supported build
  entry point, compiler/toolset band, architecture, CRT/libc choice, packaged
  artifact, and validated destination operating system.
- Clearly separated compile validation from destination runtime validation so
  historical compiler success is not mistaken for an untested legacy-OS
  guarantee.
- Documented native glibc, audited glibc 2.28, and musl compatibility models,
  and identified MinGW and Open Watcom/DOS32A work as planned rather than
  currently supported.

## 1.4.31 - 2026-10-05 - Windows XP-era compiler band

- Added an isolated CMake 3.5 legacy build definition and native CMD dispatcher
  for VC6 SP6, VS2002 SP1, VS2003 SP1, and VS2005 SP1 without weakening the
  modern root CMake project.
- Validated x86 shared-library builds and public-API execution on Windows XP
  SP3 with all four compiler generations.
- Validated faithful Nuked-OPL3, embedded DBOPL, timing-preserving silent
  audio, and static/dynamic CRT variants at the oldest VC6 boundary.
- Added clean compiler-labeled SDK staging for legacy builds, including the
  DLL, import library, public headers, text-only notices, and replaceable
  Nuked-OPL3 component.
- Extended the pre-C99 compatibility layer for `SIZE_MAX`, VC6 bounded
  formatting, pre-`fopen_s` runtimes, portable 64-bit constants, and VC6's
  character-array initialization rules without changing game behavior.
- Added deterministic explicit exports for Nuked-OPL3 where CMake's oldest
  Visual Studio generator cannot synthesize an import library.
- Re-ran the modern VS2022 warnings-as-errors build and complete unit, smoke,
  and public-consumer suite after the compatibility work.

## 1.4.30 - 2026-10-05 - Legacy MSVC compiler span

- Added first-class PowerShell-dispatcher and CMake-preset support for Visual
  Studio 2013/v120, 2012/v110, 2010/v100, and 2008/v90 on x86 and x64.
- Validated the shared library, public API consumer, deterministic headless
  host, and complete unit suite with warnings as errors under every added
  compiler and architecture.
- Added narrowly scoped pre-C99 MSVC compatibility for fixed-width integers,
  integer constants, `inline`, and bounded formatting without changing game
  logic or arithmetic.
- Made the staged SDK's public header self-contained for VS2008 consumers by
  including its compatibility integer header.
- Validated Nuked-OPL3, pure-C DBOPL, timing-preserving silent audio, and both
  static and dynamic CRT modes at the oldest VS2008 boundary.
- Made root PowerShell launchers independent of the caller's working directory
  and corrected legacy-install detection under Windows PowerShell 5.1.

## 1.4.29 - 2026-10-01 - Windows XP targeting candidate

- Added compiler-qualified v140_xp presets and dispatcher support for x86 and
  x64 library builds with static or dynamic CRT selection.
- Passed the complete internal and public-consumer test suite and staged SDKs
  for both architectures; execution on actual XP systems remains pending.

## 1.4.28 - 2026-10-01 - Visual Studio 2015 and 2017 checkpoints

- Added first-class CMake presets and Windows dispatcher support for native
  Visual Studio 2015/v140 and the v141 compiler toolset hosted by VS2019.
- Validated x86 and x64 library builds, both MSVC runtime modes, internal tests,
  public-header consumption, and compiler-qualified SDK staging with MSVC 19.0
  and MSVC 19.16.

## 1.4.27 - 2026-10-01 - Shareware reference formatting

- Normalized the new archive checksum layout so the documentation remains
  clean under Git whitespace validation.

## 1.4.26 - 2026-10-01 - Shareware data references

- Added stable mirror links for the freely distributed Wolfenstein 3D
  shareware (`WL1`) and Spear of Destiny demo (`SDM`) data-only archives,
  including verified archive contents and SHA-256 hashes.

## 1.4.25 - 2026-10-01 - Interactive build guidance

- Made no-argument Windows builds interactive, with guided compiler,
  architecture, action, configuration, CRT, audio, and OPL choices plus a
  final confirmation; explicit arguments and `-NonInteractive` retain
  deterministic automation.
- Kept the root `build.ps1` as a stable, discoverable launcher while moving
  its implementation under `scripts/windows/` and documenting how that
  structure relates to Linux Make and future legacy solution bands.

## 1.4.24 - 2026-10-01 - Human-facing Windows build dispatcher

- Added a Windows PowerShell dispatcher that detects supported Visual Studio
  installations and selects compiler, architecture, configuration, CRT,
  audio/OPL backend, and build/test/package/clean actions while delegating to
  the authoritative CMake presets and targets.
- Defined the future Visual Studio compatibility-band policy: retain one
  solution while formats remain compatible, then add empirically required
  solution/project sets that all reference the same root source tree.
- Established period-appropriate `.cmd` launchers as the fallback for legacy
  Windows environments where PowerShell or modern CMake is unavailable.

## 1.4.23 - 2026-10-01 - Visual Studio 2022 compiler checkpoint

- Added explicit Visual Studio 2022/v143 x86 and x64 CMake presets while
  retaining the established Visual Studio 2019/v142 preset names.
- Made the checked-in solution select v142 or v143 and isolated build trees
  automatically according to the Visual Studio generation opening it.
- Added a public-header-only consumer executable to every test-enabled build,
  verifying that an external program can compile, link, load, and call the
  shared-library API without access to private engine headers.
- Added compiler-labeled Windows SDK directories, aligned their text-only
  licensing layout with the portable distributions, and documented the
  validated compiler matrix and future checkpoint policy.

## 1.4.22 - 2026-10-01 - Neutral library window title

- Replaced the obsolete `wolf3dgeneric` window-title branding with the
  host-neutral `wolf3d` library identity, including the selected data-set
  description once game data is loaded.

## 1.4.21 - 2026-10-01 - Portable audio backends and musl SDK

- Introduced a private OPL boundary while retaining official Nuked-OPL3 as the
  default, independently replaceable reference implementation.
- Added PrBoom+'s GPL-compatible pure-C port of DOSBox DBOPL as an optional
  lower-resource backend with pinned provenance and no SDL dependency.
- Added a timing-preserving silent audio profile: IMF, AdLib-effect,
  PC-speaker, and digitized-sound state advances normally while output samples
  remain silent.
- Added backend-specific Make/CMake selection, isolated package names, and
  regression coverage for reference, DBOPL, and silent configurations.
- Added a digest-pinned Alpine 3.20 musl build environment, musl-specific SDK
  packaging, ELF auditing, and GCC/Clang entry points in preparation for the
  companion repository's relocatable Linux application bundle.

## 1.4.20 - 2026-10-01 - Library and host repository separation

- Completed the rename to `wolf3d-lib` and made this repository exclusively
  responsible for the shared engine, public API, internal deterministic oracle,
  regression suite, and library SDK packages.
- Moved the production Win32, SDL3, and Linux direct-console wrappers to the
  history-preserving companion `wolf3d-portable` repository, where every build
  compiles a pinned `wolf3d-lib` submodule revision.
- Removed SDL3 and OS-wrapper dependencies from the core build, simplified the
  Make/CMake/Visual Studio entry points around library development and release,
  and retained the Debian 10 / glibc 2.28 portable library package.

## 1.4.19 - 2026-10-01 - Wolf3D public library identity

- Renamed the public header to `WOLF3D.h` and the exported functions, types,
  constants, screen buffer, and palette to consistent `wolf3d_`/`WOLF3D_`
  names while leaving historical `WL_*`, `ID_*`, and internal `WG_*` names
  intact.
- Renamed the shared library to `wolf3d.dll`/`libwolf3d.so`, added the in-tree
  `wolf3d::wolf3d` CMake target, and versioned the ELF library with ABI-major
  SONAME 1.
- Updated every bundled host, test, SDK package, and porting document to use
  the new public interface.

## 1.4.18 - 2026-10-01 - Public host boundary

- Removed the private engine source directory from every production host's
  include path and made Win32, SDL3, and Linux-console consume only the public
  library header and its transitive CMake interface.
- Kept private header access explicit and confined to the internal headless
  diagnostic and regression-test targets in preparation for repository
  separation.

## 1.4.17 - 2026-10-01 - Authoritative library revision

- Added a root `VERSION` file as the single source for CMake, GNU Make,
  generated package metadata, and staged distribution names.
- Established the `1.4.REVISION` policy and build-time validation that the
  current revision is well formed and represented in this changelog.
- Retrospectively numbered the sixteen post-v1.4.0 milestones as revisions
  1.4.1 through 1.4.16.

## 1.4.16 - 2026-10-01 - Linux release workflow and SDL 3.2 baseline

- Added comprehensive `make help` output and made plain `make` stage the
  dependency-light shared-library distribution. Library, direct-console, and
  SDL3 releases now use isolated compiler-specific build trees with short
  distribution aliases and configurable parallelism.
- Made core development/tests independent of optional DRM, ALSA, and SDL
  development packages; each release path now requests only its own host
  dependencies.
- Lowered the supported system-SDL baseline to the first stable SDL3 series,
  SDL 3.2, while retaining pinned SDL 3.4.16 as the reproducible default.
- Added an explicit `make dependencies` bootstrap target and an actionable
  pinned-SDL preflight check without implicit network access.
- Added a digest-pinned Debian 10 container release target and automatic ELF
  symbol-version gate. The validated pinned-SDL package requires at most
  `GLIBC_2.27` and runs on both Debian 10 and current Debian releases. A
  checksum-pinned Wayland 1.18 build toolchain enables SDL's native Wayland
  backend without raising that ABI floor.
- Extended `make clean` with project-scoped portable-build cleanup, including
  all portable CMake trees and the named Docker builder image, without pruning
  unrelated Docker caches.
- Added Debian 10/glibc 2.28 release paths for the shared-library and direct-
  console packages alongside SDL3, plus a `make portable` aggregate and
  consistently prefixed `portable-library`, `portable-console`, and
  `portable-sdl3` aliases. The public native console target is now the shorter
  `console-release`; `linux-console-release` remains compatible.
- Documented pinned-versus-system tradeoffs, including system-SDL backend
  dependencies, and the precise portability scope of the locally bundled
  Linux SDL3 distribution.

## 1.4.15 - 2026-09-29 - Original menu sound feedback

- Restored the original two-part `MOVEGUN1SND`/`MOVEGUN2SND` feedback while
  moving through menus, including the eight-tic interval between the cursor's
  half-step and landing sounds without blocking the portable event loop.
- Replaced the in-game pistol report previously used for several confirmations
  with the original menu `SHOOTSND`, and restored feedback in the main,
  load/save, control, sound, episode, difficulty, sensitivity, and control-
  customization paths.
- Restored `ESCPRESSEDSND` when Escape or the equivalent secondary mouse
  button backs out of a menu or specialized control screen.

## 1.4.14 - 2026-09-29 - Prompt-safe F11 fullscreen toggle

- Added F11 alongside Alt+Enter as a fullscreen toggle in both GUI hosts and
  consumed every F11 transition at the host boundary, allowing presentation
  changes without satisfying SIGNON, intermission, cheat-message, or other
  "any key" waits.

## 1.4.13 - 2026-09-29 - Documentation organization

- Split post-release v1.4 work into individually titled changelog sections and
  moved the completed development plan under `docs/` with the other historical
  design material.
- Retained `CHANGELOG.md` and `THIRD_PARTY.md` at the repository root as the
  conventional user-facing history and distribution-facing licensing index.

## 1.4.12 - 2026-09-29 - FM balance and original M-L-I cheat

- Reproduced the original Sound Blaster Pro mixer policy by applying a fixed
  4x gain to the FM bus after unmodified Nuked-OPL3 synthesis. Music and AdLib
  effects now sit at a practical level beside digitized effects while retaining
  measured 16-bit mixing headroom.
- Restored the simultaneous `M` + `L` + `I` gameplay cheat, including 100%
  health, 99 ammo, both keys, chaingun selection, score reset, ten-minute level
  time penalty, and the original modal high-score warning.

## 1.4.11 - 2026-09-29 - .NET follow-on project specification

- Added a standalone development specification for a future `wolf3d-dotnet`
  repository, preserving three independently buildable stages: native-library
  hosts, a fidelity-first C# engine port, and a readable modern-asset engine.

## 1.4.10 - 2026-09-28 - Menu cursor and death-transition fidelity

- Restored the original two-frame menu gun animation: the highlighted cursor
  now briefly shifts to `C_CURSOR2` for nine 70 Hz tics, then holds
  `C_CURSOR1` for 71 tics, with its cadence preserved while moving within a
  menu.
- Restored the complete DOS death/restart presentation. Death still fizzles
  the rendered view to palette-index red over 70 tics, but a surviving player
  now skips both Get Psyched pacing phases and the restarted view fizzles back
  over the retained red field in 20 tics instead of over black.

## 1.4.9 - 2026-09-28 - Persistent floor HUD

- Preserved the loaded floor number across gameplay, intermission, victory,
  and diagnostic HUD reconstruction, so Floor 2 no longer briefly displays or
  returns to Floor 1 around Get Psyched and subsequent redraws.

## 1.4.8 - 2026-09-28 - Activision/GOG WL6 resource profile

- Recognized the supplied GOG/Activision WL6 resource profile independently
  of its folder name and selected the embedded Activision SIGNON automatically.
  Added real-data coverage for its changed scenery sprite and its intentionally
  duplicated, therefore static, intermission portrait frame.
- Kept the level-complete BJ breathing on its original direct portrait-update
  path; confirmed the static GOG result comes from duplicated source artwork,
  not a missed engine animation callback.

## 1.4.7 - 2026-09-27 - Intermission portrait and fullscreen cursor

- Revalidated the original direct two-frame BJ intermission portrait update
  and removed the unnecessary full-screen rebuild used while investigating the
  static GOG artwork.
- Hid the native pointer while either GUI host is fullscreen and restored it
  on return to windowed mode.

## 1.4.6 - 2026-09-27 - Live Change View projection

- Applied accepted Change View sizes to the active renderer immediately,
  matching the original `NewViewSize` path instead of deferring the new
  projection until another game or level was started.

## 1.4.5 - 2026-09-27 - Toggleable GUI fullscreen

- Added borderless desktop fullscreen to the native Win32 and SDL3 hosts,
  selectable at startup with `--fullscreen` and toggleable with Alt+Enter.

## 1.4.4 - 2026-09-27 - Spear main-menu default

- Restored the original `STARTITEM` behavior for Spear and GOODTIMES data:
  their first main-menu visit now selects New Game, while Apogee Wolf3D retains
  its Read This default.

## 1.4.3 - 2026-09-27 - Windows shell-prompt restoration

- Positioned the shell after each exit screen's actual final content row and,
  for Windows GUI hosts, preserved and restored the parent shell's real prompt
  instead of overwriting it or requiring an extra Enter key.

## 1.4.2 - 2026-09-27 - Live random initialization

- Restored the original live-versus-demo random initialization: live levels
  use the host clock's hundredths phase, while demos use index zero, and the
  selected index is installed before map actors consume random values.

## 1.4.1 - 2026-09-27 - Independent audit corrections

- Rejected legacy portable saves whose actor coordinates fall outside the
  64x64 occupancy grid instead of using those coordinates as array indices.
- Prevented the native Win32 message pump from filling its translated-event
  queue and dropping later key or button releases during an input burst.
- Made generated DOS data filenames resolve case-insensitively on
  case-sensitive hosts, matching DOS/Windows behavior and the documented
  data-file contract.

## 1.4.0 - 2026-09-27

- Restored the data-driven DOS `ORDERSCREEN` and `ERRORSCREEN` console output,
  including CP437 line art and VGA colors through a portable text-cell host
  callback with native Win32 and ANSI terminal implementations.
- Restored case-insensitive, punctuation-tolerant DOS launch options including
  `-GOOBERS`/`-DEBUGMODE`, `-TEDLEVEL`, `-NOWAIT`, and TED difficulty names,
  plus the principal gated debug keys.
- Added a Linux virtual-console host with direct DRM/KMS dumb-buffer video,
  evdev keyboard/mouse/gamepad input, ALSA audio with silent fallback, 4:3
  presentation, device overrides, and a minimal staged runtime package.
- Added an SDL3 GUI host for Windows and Linux with keyboard, opt-in mouse,
  gamepad, audio, aspect-correct 4:3 presentation, and minimal staged packages.
- Added a GNU Make entry point for native Linux configure, build, test, clean,
  and library-package workflows while retaining CMake as the sole build graph.
- Added a checked-in Visual Studio solution supporting Debug/Release and
  Win32/x64 while delegating compilation to the authoritative CMake targets.
- Exposed both static (`/MT`, default) and dynamic (`/MD`) MSVC runtime builds
  while retaining the engine as a separate DLL in both modes.
- Corrected strict ISO C portability findings exposed by GCC and Clang without
  changing engine behavior.
- Restored visible fast-host startup pacing: SIGNON now preserves its initial
  status and green `Working...` beats, while Get Psyched visibly fills its
  preload bar before the original completed-bar hold.
- Kept the DOS exit screen above the returned Windows command prompt so the
  shell no longer overwrites the final line of the original colored output.

## 1.3.0 - 2026-09-27

- Restored DOS `CalcTics` batching for live play: each rendered gameplay frame
  now performs one update with the elapsed 1--10 tic value, while demos retain
  their authored four-tic commands and presentation phases retain one-tic
  service updates.
- Removed runtime floating-point trigonometry. Verified renderer/projection
  tables are frozen for every legal view width, and projectile/death-camera
  angles use a deterministic integer quantizer matching the original
  single-precision truncation.
- Added full-table hashes, sensitive angle-boundary checks, and live-play tests
  across every supported `tics` value; complete x86 and x64 data/demo suites
  remain identical.

## 1.2.0 - 2026-09-26

- Restored BJ's original two-frame breathing animation on the level-completed
  screen, including its initial 10-tic delay and subsequent 35-tic cadence.

- Refactored the engine into a true shared library with a versioned platform
  callback ABI, a six-symbol public surface, and a clean `library-release`
  package; the Win32 executable now dynamically links that engine.
- Added minimal x86/x64 `win32-release` folders with adjacent-data discovery,
  a replaceable Nuked-OPL3 DLL, license notices, and no external MSVC runtime
  dependency.
- Aspect-corrected the Win32 host's native 320x200 VGA output to a centered
  4:3 viewport while leaving the generic framebuffer contract unchanged.
- Made mouse hardware an explicit `--mouse` opt-in: without it SIGNON leaves
  Mouse unmarked, events are ignored, and mouse menu entries are unavailable.
- Corrected WL1's shifted menu-art chunks so Control and Sound screens use the
  original selection boxes and titles instead of unrelated weapon/disk art.
- Restored the original fractional pushwall ray intersections so secret walls
  translate backward with correct edge and side geometry instead of squeezing.
- Prevented stale actor rotation metadata from rotating non-rotating death
  states into the following sprite range, which could make a dying WL1 guard
  flash as a dog.
- Restored the original per-state `SightPlayer` calls in normal gameplay, so
  connected-area sight and weapon noise now wake standing and patrolling
  enemies after their class-specific reaction delay.
- Restored persistent actor activation, once-per-frame noise lifetime, exact
  door-area connection timing, victory-time chase suspension, and the original
  non-rotating shooting and dog-jump states.
- Added one runtime-selectable engine for Spear of Destiny (`SOD`), its
  two-floor demo (`SDM`), and the `SD1`, `SD2`, and `SD3` mission profiles.
- Added executable-name family preference plus strict `--game` extension
  selection, including historical mixed-extension and isolated GOG layouts.
- Restored Spear's palette, title and menu layouts, map ceiling colors, status
  faces, actors, bosses, projectiles, items, sound mapping, music, secret-floor
  routing, Spear pickup transition, intermissions, victory collapse, ending,
  high scores, attract demos, and SDM conclusion.
- Embedded all five runtime-selectable original SIGNON screens, restored their
  memory/hardware overlays, and added automatic edition-based selection plus
  explicit Wolf/Spear palette overrides.
- Restored Wolf3D's yellow `Press a key` and green `Working...` SIGNON prompts
  plus Spear's original timed three-second SIGNON hold.
- Added an `--adlib` hardware profile for OPL music and effects without
  digitized Sound Blaster playback, including the original SIGNON indication.
- Kept configuration and save files isolated by logical game profile even when
  a GOG mission directory physically names every archive `.SOD`.
- Validated every map and asset reference in SOD, SDM, and all three mission
  packs, and added deterministic visual and full-demo regression coverage.

## 1.1.0 - 2026-09-26

- Added a portable two-device joystick event contract and restored the original
  `ID_IN.C` calibrated axis scaling.
- Restored joystick enable, port selection, Gravis four-button mode, menu
  navigation, gameplay movement, and configurable button bindings.
- Added dependency-free Win32 XInput discovery by dynamic loading, with left
  stick/D-pad movement and A/B/X/Y mapped to the four original buttons.
- Preserved compatibility with version-2 configuration files while persisting
  the new joystick settings in version 3.
- Removed the project-specific GitLab CI configuration.

## 1.0.0 - 2026-09-25

- Completed the portable Wolfenstein 3D v1.4 engine for the Apogee shareware
  (`WL1`) and GT/ID/Activision full (`WL6`) data sets.
- Restored the original renderer, gameplay, enemies and bosses, menus, attract
  loop, all four demos, intermissions, victory flow, high scores, save/load,
  configuration, keyboard, and mouse behavior.
- Added faithful IMF/AdLib output through official Nuked-OPL3, digitized Sound
  Blaster effects, and original PC-speaker synthesis.
- Added dependency-free Win32 and deterministic headless hosts, a documented
  generic platform contract, and 32-bit/64-bit regression coverage.
- Added strict GCC/Clang CI and Clang sanitizer jobs without bundling commercial
  game data.

Spear of Destiny, joystick input, and Disney Sound Source output are outside the
1.0 Wolfenstein 3D scope.
