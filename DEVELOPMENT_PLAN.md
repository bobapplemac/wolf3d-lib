# wolf3dgeneric development plan

## Completion status

The original Wolfenstein 3D and Spear of Destiny scope is complete. The Apogee
shareware (`WL1`), GT/ID/Activision full (`WL6`), Spear (`SOD`), Spear demo
(`SDM`), and all three mission profiles (`SD1`/`SD2`/`SD3`) pass the
deterministic regression suite. Release 1.1 restored portable joystick support;
release 1.2 added the runtime-selected Spear family without forking the engine.

## Project intent

`wolf3dgeneric` will be a portable, dependency-free Wolfenstein 3D engine core in
the spirit of `doomgeneric`: a host supplies a small set of platform functions,
while the core retains the original game's data formats, rules, timing, rendering,
menus, demos, and audiovisual behavior.

The project is a preservation-oriented port, not an enhancement port. The default
experience will use the original 320x200 indexed-color framebuffer, 70 Hz game
clock, original resource files, original gameplay constants, original random
number sequence, and nearest-neighbor presentation. Widescreen rendering, higher
resolution art, filtering, new effects, altered AI, and other modernizations are
out of scope.

## Source policy

1. Treat id Software's original `WOLFSRC` tree as the behavioral authority.
2. Use Wolf4SDL and Chocolate Wolfenstein 3D to understand how DOS, segmented
   memory, VGA, assembly, input, and audio code can be expressed portably.
3. Use the Game Engine Black Book sources as architectural commentary, not as a
   replacement specification.
4. Use Doom and doomgeneric only for port-boundary and packaging patterns.
5. Use the official Nuked-OPL3 implementation as the OPL behavioral reference
   and initial emulator. Do not use the MAME or DOSBox OPL implementations.
6. Keep porting changes traceable. Nontrivial replacements should name the
   original routine and, when useful, the reference implementation consulted.
7. Do not include commercial game data in the repository.

The working license should be GPL-2.0-only because the usable modern reference
ports and doomgeneric are GPLv2 code. Nuked-OPL3 is LGPL-2.1-or-later and will
remain a clearly identified third-party component with its copyright notices,
license, corresponding source, and any local modifications preserved. Before a
public release, all copied code, notices, and third-party licensing will receive
a dedicated audit.

## Recommended initial scope

The initial scope should include both supplied Wolfenstein 3D v1.4 data sets:
GT/ID/Activision full (`WL6`) and Apogee shareware (`WL1`). The full game remains
the canonical fidelity target, while the shareware build provides a smaller and
more accessible validation/distribution path. Both configurations should be
built from the beginning because their resource identifiers and conditional
content differ. Spear of Destiny should follow only after both Wolf3D v1.4 builds
are deterministic and complete. The version switches will be designed from the
start so Spear support does not require an architectural rewrite.

“Faithful” means matching observable game behavior. We will preserve benign
quirks that affect demos or gameplay, but fix undefined behavior, memory-safety
faults, endian assumptions, and host-size assumptions when necessary. Every such
fix must retain equivalent results for valid original data.

## Proposed architecture

### Engine core

- Portable C (C99-compatible subset), fixed-width integer types, no C++ runtime.
- No mandatory windowing, graphics, audio, or operating-system library.
- Original module boundaries retained where they remain meaningful:
  cache/data (`ID_CA`), page data (`ID_PM`), input (`ID_IN`), sound (`ID_SD`),
  video (`ID_VL`/`ID_VH`), UI (`ID_US`), and gameplay/rendering (`WL_*`).
- DOS memory-manager behavior replaced by normal owned allocations; cache-level
  semantics retained only where gameplay or resource lifetime depends on them.
- VGA planar pages and latches represented by portable memory structures and an
  authoritative 320x200 8-bit framebuffer plus 256-color palette.
- Original self-modifying scaler and 16-bit assembly replaced by straightforward
  C implementations with the same pixel and fixed-point results.
- Explicit little-endian file readers and checked offsets for all original data
  formats.

### Generic host boundary

The exact header will be frozen after a thin vertical prototype. The intended
surface is small and resembles this responsibility set:

- initialize and shut down the host;
- present a 320x200 indexed frame and current palette;
- report monotonic milliseconds and sleep/yield;
- supply key up/down, relative mouse, and normalized joystick events;
- set a window title and report fatal errors;
- open an audio sink or accept core-generated signed PCM.

The first implementation should preserve Wolf3D's naturally blocking control
flow and expose `wolf3dgeneric_Create(...)` plus `wolf3dgeneric_Run()`. Unlike
Doom, Wolf3D's attract loop, menus, intermissions, fades, and play loop are deeply
nested blocking state machines. Forcing a public one-frame `Tick()` API at the
start would require a broad behavioral rewrite. Once fidelity is established,
we can add a cooperative stepping API by deliberately converting those state
machines, without making that risky refactor a prerequisite for the port.

### Reference hosts

- `headless`: no window or sound; scripted input, virtual time, frame/palette
  hashes, demo playback, and automated regression tests.
- `win32`: dependency-free Windows window, nearest-neighbor 4:3 presentation,
  keyboard/mouse/XInput input, monotonic timing, and native PCM output.
- A deliberately small SDL or other third-party example may be added later, but
  it must remain optional and outside the engine core.

## Milestones

### 0. Provenance and behavioral baseline

- Add GPLv2 and attribution/provenance documents.
- Record hashes and identify the supplied `WL1` and `WL6` data revisions.
- Preserve an external-data-only test configuration; never copy game assets into
  Git.
- Capture baseline title/menu/demo behavior from the supplied DOS executable in
  its bundled DOSBox.
- Establish formatting, warnings, assertions, and a clean CMake build.

Exit gate: an empty core library, headless executable, and Win32 executable build
reproducibly with warnings treated as errors; baseline data and behavior are
documented.

### 1. Portable foundation and resource loading

- Port fundamental types, argument handling, error paths, and configuration.
- Port Huffman graphics decompression, Carmack expansion, RLEW map expansion,
  audio headers, map headers, and VSWAP page indexing.
- Replace far pointers, segment arithmetic, Borland APIs, and the DOS memory/page
  managers with bounded portable structures.
- Unit-test decompression and resource directory results against both supplied
  data sets.

Exit gate: all required `WL1` and `WL6` assets can be identified and decoded
headlessly, with stable checksums and sanitizer-friendly bounds checks.

### 2. Indexed video and 2D presentation

- Implement the 320x200 indexed framebuffer, palette updates, fades, bars, text,
  pictures, latch-equivalent blits, and fizzle fade.
- Port sign-on, title, credits, high scores, and control-panel drawing.
- Produce deterministic frame and palette hashes in the headless backend.

Exit gate: startup and menu screens match DOS reference captures at the logical
framebuffer/palette level.

### 3. Renderer

- Port fixed-point view setup and ray traversal from the original C/assembly,
  consulting the modern ports for known 32-bit translations.
- Replace generated scaling code with portable column/sprite scaling.
- Port walls, doors, pushwalls, sprites, weapons, visibility, and status bar.
- Retain original resolution, projection, texture selection, and draw order.

Exit gate: scripted scenes and built-in demos produce stable expected frames;
representative frame differences against the reference are explained or zero.

### 4. Input, timing, UI, and complete control flow

- Map host events to original scan-code semantics.
- Reproduce 70 Hz tic calculation, input acknowledgement, mouse behavior,
  palette timing, fades, and menu repeat behavior.
- Port attract mode, menus, read-this screens, game setup, intermissions,
  high-score entry, save/load, and clean shutdown.

Exit gate: the complete front end is usable and deterministic with both scripted
headless input and the Win32 backend.

### 5. Gameplay fidelity

- Port actors, state machine, AI, collision, combat, doors, pushwalls, pickups,
  scoring, secrets, level transitions, death, victory, and demo playback.
- Preserve original PRNG consumption and tic ordering.
- Add focused tests for historically fragile behavior and binary save/load round
  trips where format compatibility is practical.

Exit gate: all four original demos remain synchronized; a full episode can be
played; save/load, death/restart, secret levels, and victory paths work.

### 6. Audio

- Port IMF music scheduling, AdLib effects, digitized effects, priorities,
  positioning, and mode selection.
- Vendor the official `nukeykt/Nuked-OPL3` implementation first and treat its
  output as the OPL reference. Configure it as an OPL2-compatible device for the
  register stream produced by Wolfenstein 3D.
- Keep Nuked-OPL3 isolated under `third_party`, retain its LGPL-2.1-or-later
  notices, and make upgrades or local changes independently reviewable.
- Benchmark the official implementation during realistic gameplay and dense
  IMF playback on supported hosts. Pivot to `tgies/Nuked-OPL3-fast` only if
  measured audio deadlines cannot be met. Before pivoting, require sample-exact
  A/B tests for Wolf3D's complete IMF and AdLib-effect corpus as well as the
  project's selected upstream revision.
- Do not integrate the MAME or DOSBox OPL implementations.
- Generate PCM in the core and keep device transport in each host backend.
- Validate event timing and rendered audio hashes independently of the physical
  output device.

Exit gate: music and effects are synchronized, deterministic in headless tests,
and audible through the Win32 host without changing game timing.

### 7. Portability hardening

- Keep the portable C boundary suitable for MSVC, GCC, and Clang.
- Test 32-bit and 64-bit builds, strict warnings, static analysis, and
  big-endian-safe parsing by unit test even if no big-endian runner is available.
- Document how to implement a new host in one small source file.

Exit gate: clean CI, documented backends, no bundled proprietary data, and a new
platform port requires only the generic host contract.

### 8. Release audit

- Run the complete regression suite and manual play matrix.
- Audit licensing, attribution, third-party code, generated files, and asset
  exclusion, including Nuked-OPL3 LGPL source/relinking obligations.
- Finish README, supported-data hash table, build instructions, limitations,
  and porting guide.
- Tag the first release only after reproducible clean builds and gameplay/audio
  acceptance gates pass.

## Verification strategy

Testing will be layered rather than relying only on manual play:

1. Unit tests for endian reads, fixed-point helpers, PRNG, decompression, page
   tables, fizzle sequence, and save serialization.
2. Resource tests against the installed GOG data, referenced by external path.
3. Headless virtual-time tests with scripted scan-code/mouse input.
4. Framebuffer and palette hashes at named checkpoints.
5. Built-in demo synchronization and per-tic state digests.
6. Audio PCM hashes for selected music/effect sequences.
7. DOSBox reference captures for screens and behavioral edge cases.
8. Interactive smoke tests on the dependency-free Win32 host.
9. Cross-compiler and sanitizer checks when a suitable toolchain is available.

Generated configs, saves, screenshots, and recordings will go to the build/test
tree, not the source tree. Tests will never modify the installed GOG files.

## Tooling status

Available and verified on the workstation:

- Git and authenticated SSH read access to the empty GitLab remote;
- Visual Studio 2019 C/C++ tools and MSBuild (newer Visual Studio installations
  are also present);
- CMake 3.20, Ninja 1.10, and CTest from the Visual Studio installation;
- Python with the bundled testing/document tooling;
- Node.js, ripgrep, PowerShell, Poppler, and standard archive tools;
- the supplied Wolfenstein 3D v1.4 full/shareware data and DOS executables, plus
  the installed Wolfenstein 3D/Spear data and GOG DOSBox.

Visual Studio's bundled Clang static analyzer was run locally during release
hardening. SDL is intentionally not required by either the engine or its
reference hosts.

## Review decisions

Unless feedback changes them, development will proceed with these choices:

1. Initial targets: full Wolfenstein 3D v1.4 GT/ID/Activision (`WL6`) and v1.4
   Apogee shareware (`WL1`), with the full build as the canonical fidelity target.
2. Language: portable C, not C++.
3. Initial public control flow: `Create` + blocking `Run`; cooperative `Tick`
   considered after fidelity is proven.
4. First interactive backend: dependency-free Win32; headless backend from the
   beginning.
5. Audio is required for project completion, but follows deterministic video and
   gameplay so it cannot obscure core-port defects. Official Nuked-OPL3 is the
   default; Nuked-OPL3-fast is permitted only as a measured, bit-exact fallback.
6. Both Wolf3D variants are part of the first playable vertical slice; Spear is
   a post-fidelity milestone.
