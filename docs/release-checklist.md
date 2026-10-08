# Release 1.4 verification

- [x] Apogee shareware v1.4 (`WL1`) resource corpus validated.
- [x] GT/ID/Activision full v1.4 (`WL6`) resource corpus validated.
- [x] Spear (`SOD`), demo (`SDM`), and all three mission-pack resource corpora
  validated, including all 65 maps across those profiles.
- [x] All four embedded demos complete deterministically in both editions.
- [x] All four SOD demos and the sole SDM demo complete deterministically.
- [x] Renderer, gameplay, control-panel, save/config, and audio fixtures pass.
- [x] Portable two-device joystick scaling, controls, menus, and persistence
  are covered by deterministic tests.
- [x] MSVC x86 and x64 strict-warning builds pass all 148 tests.
- [x] GCC and Clang strict-warning Linux builds pass all 148 tests against the
  complete supported-data corpus.
- [x] Clang AddressSanitizer and UndefinedBehaviorSanitizer pass all 148 tests.
- [x] Live gameplay preserves original 1--10 `CalcTics` batching, while demos
  preserve their authored four-tic command cadence.
- [x] Runtime renderer and gameplay angle math is deterministic and independent
  of host floating-point libraries.
- [x] Static-analyzer findings reviewed and actionable findings corrected.
- [x] Official Nuked-OPL3 source and LGPL terms retained separately.
- [x] Win32 release staging produces a minimal x86/x64 folder with a
  replaceable Nuked-OPL3 DLL, complete license notices, and no build artifacts.
- [x] The generic engine builds as a shared library with a ten-symbol public
  ABI, a versioned host callback table, and its own clean release target.
- [x] The Win32 executable imports the engine DLL instead of embedding it.
- [x] Staged executables use the adjacent folder for game data by default and
  require no non-system Visual C++ runtime DLL.
- [x] SDL3 GUI release staging produces x86/x64 Windows folders and a Linux
  folder with the engine, replaceable Nuked-OPL3, and SDL shared libraries.
- [x] Linux direct-console staging provides DRM/KMS video, evdev input, and
  ALSA audio without requiring a desktop or display server.
- [x] DOS startup/loading and colored exit-screen presentation remain visible
  on fast hosts without changing gameplay, demo, RNG, or audio timing.
- [x] GPL and third-party provenance reviewed.
- [x] No proprietary game data, generated saves, configs, captures, or builds
  are tracked.
- [x] Generic host boundary and exact supported-data hashes documented.

The release covers Wolfenstein 3D v1.4 `WL1` and `WL6`, Spear of Destiny
`SOD`, the `SDM` demo, and mission profiles `SD1`, `SD2`, and `SD3`. Disney
Sound Source remains deliberately outside the project scope.
