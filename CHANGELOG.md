# Changelog

## 1.4.0 - 2026-09-27

- Restored the original live-versus-demo random initialization: live levels
  use the host clock's hundredths phase, while demos use index zero, and the
  selected index is installed before map actors consume random values.
- Rejected legacy portable saves whose actor coordinates fall outside the
  64x64 occupancy grid instead of using those coordinates as array indices.
- Prevented the native Win32 message pump from filling its translated-event
  queue and dropping later key or button releases during an input burst.
- Made generated DOS data filenames resolve case-insensitively on
  case-sensitive hosts, matching DOS/Windows behavior and the documented
  data-file contract.
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
- Positioned the shell after each exit screen's actual final content row and,
  for Windows GUI hosts, preserved and restored the parent shell's real prompt
  instead of overwriting it or leaving only an unlabelled input cursor.

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
