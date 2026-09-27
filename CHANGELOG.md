# Changelog

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
