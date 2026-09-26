# Changelog

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
