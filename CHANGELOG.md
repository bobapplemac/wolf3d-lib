# Changelog

Wolfenstein 3D's historically meaningful `1.4` product identity remains
stable. The third component is a monotonically increasing library revision;
beginning with 1.4.17, every commit to the library main branch advances it.
Revisions 1.4.1 through 1.4.16 are assigned retrospectively to the dated
post-promotion milestones below without rewriting Git history.

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
