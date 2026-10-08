# Reference corpus index

This index describes the external reference corpus used during development.
It is an aid for documenting provenance and research coverage; the corpus and
its reference projects are not vendored into `wolf3d-lib`.

## Inventory and review status

- 1,771 files and 561,857,193 bytes including nested Git metadata.
- 1,575 working-tree/reference files after excluding `.git` internals.
- 1,060 text/source/configuration files decoded and scanned in full.
- 515 binary/media files classified; every file was SHA-256 hashed, and every
  raster image was opened successfully and had dimensions/mode inspected.
- Three PDFs are present: the 320-page Black Book, the 21-page “Compile Like
  It's 1992” printout, and a one-page KDP cover. The substantive PDF text is
  duplicated by the more searchable LaTeX and HTML sources.
- Embedded Git histories were identified by remote and current commit. Git pack,
  index, log, and hook files are repository metadata rather than project content.

## Primary authority: original Wolfenstein 3D

Path: `wolf3d sources\wolf3d`

Origin: `https://github.com/id-Software/wolf3d.git`, commit
`05167784ef009d0d0daefe8d012b027f39dc8541`.

Important locations:

- `WOLFSRC\WL_*.C`, `WL_DEF.H`: gameplay, actor state, renderer, menus, main
  loops, intermissions, save/load, and shared structures.
- `WOLFSRC\ID_CA.*`: graphics/audio/map caching and decompression.
- `WOLFSRC\ID_PM.*`: VSWAP page manager and EMS/XMS-era paging.
- `WOLFSRC\ID_MM.*`: segmented-memory manager; behavior to simplify, not emulate
  mechanically.
- `WOLFSRC\ID_VL.*`, `ID_VH.*`, `*_A.ASM`: VGA, palettes, planar drawing,
  latches, screen updates, and low-level assembly.
- `WOLFSRC\ID_IN.*`: keyboard interrupt state, mouse, joystick, and acknowledge
  semantics.
- `WOLFSRC\ID_SD.*`, `ID_SD_A.ASM`: 70/700 Hz timing, AdLib, PC speaker,
  Sound Blaster, digitized effects, and music.
- `WOLFSRC\WL_DR_A.ASM`, `WL_SCALE.C`, `CONTIGSC.C`, `OLDSCALE.C`: ray traversal
  and generated/alternative scalers.
- `WOLFSRC\VERSION.H` and variant headers: original build switches and generated
  resource identifiers.
- `WOLFSRC\README`: release notes and original limited-use license text.
- `WOLFSRC\WOLF3D.EXE`, maps, objects, and Borland project files: historical
  build/reference artifacts, not portable inputs.
- Root DeIce installer files and `README.rst`: provenance and release packaging.

Use this tree first whenever intent or behavior is in question.

Additional original visual references:

- `wolf3d sources\SIGNON\GAMEPAL_WOLF.OBJ` and `GAMEPAL_SPEAR.OBJ` contain
  the two executable-linked 256-color VGA game palettes. They differ only at
  palette indices 166 and 167: Wolf uses purple entries and Spear uses dark
  greens.
- `wolf3d sources\SIGNON\SIGNON_APOGEE.BIN`, `SIGNON_GT.BIN`,
  `SIGNON_ID.BIN`, `SIGNON_ACTIVISION.BIN`, and `SIGNON_SPEAR.BIN` are the
  five raw 320x200 publisher/game startup screens. They are embedded in the
  generated `src\\WG_SIGNON_ASSETS.inc`; `tools\\GENERATE_SIGNON_ASSETS.ps1`
  reproduces that source representation from these canonical inputs.
- `game files\SDM` is a complete two-map Spear demo installation. Its eight
  required archives, original executable/configuration, launch batch file,
  documentation, icon, release identification, and historical FAQ were
  inventoried separately from the full SOD mission data.

## Fidelity-oriented modern port

Path: `wolf3d sources\Chocolate-Wolfenstein-3D`

Origin: `https://github.com/fabiensanglard/Chocolate-Wolfenstein-3D.git`, commit
`eaade7d5c1887a144c6468d4777ef9f8841203ab`.

This is a Wolf4SDL derivative that removes optional weather/mod features and
aims at the original experience. It is most useful for:

- portable translations of `ID_*` and `WL_*` routines;
- 32/64-bit type and pointer conversions;
- C replacements for raycasting, scaling, latches, and page management;
- original-demo compatibility switches;
- data-version selection and known bug compatibility.

Its SDL/OpenGL CRT presentation, SDL types embedded in engine headers, C++
conversion, and platform initialization are references, not target architecture.

## Broad modern port

Path: `wolf3d sources\wolf4sdl`

Snapshot without nested Git metadata. This is the largest practical comparison
for portable gameplay and subsystem code. Particularly useful files are
`id_ca.cpp`, `id_pm.cpp`, `id_vl.cpp`, `id_in.cpp`, `id_sd.cpp`, `wl_draw.cpp`,
`wl_play.cpp`, `wl_main.cpp`, and `version.h`. Its bundled DOSBox/MAME OPL
implementations are excluded by project policy.

Optional shading, floor/ceiling textures, high-resolution assets, skies,
weather, directional sprites, changed pushwall behavior, higher resolutions,
multichannel effects, and other enhancements are explicitly out of scope.

## Doom architecture references

Paths:

- `doom sources\DOOM` — id Software Linux Doom, commit
  `a77dfb96cb91780ca334d0d4cfd86957558007e0`.
- `doom sources\doomgeneric` — doomgeneric, commit
  `dcb7a8dbc7a16ce3dda29382ac9aae9d77d21284`.

Use Linux Doom to understand the original `I_*` platform boundary and use
doomgeneric for the minimal-host pattern: caller-owned main loop, shared screen
buffer, initialize/present/time/sleep/input/title functions, per-platform source
files, and small build recipes. Doom gameplay/rendering code is not a Wolf3D
implementation reference.

## OPL emulation references

Paths:

- `opl\Nuked-OPL3` — official Nuke.YKT implementation, origin
  `https://github.com/nukeykt/Nuked-OPL3.git`, commit
  `765ec962e473aeb767e4cba74ffdc8f588ffbfe8`.
- `opl\Nuked-OPL3-fast` — bit-exact performance fork, origin
  `https://github.com/tgies/Nuked-OPL3-fast.git`, commit
  `f44bacb1143cd78d36bacf69fbec8c60a125b3c9` (`1.8-fast.3`).
- PrBoom+ `prboom2/src/MUSIC/dbopl.c` and `dbopl.h` — DOSBox DBOPL r3635
  converted to C, upstream commit
  `969515162c5aebea4ad7a125ee178dcad3d576ad`.

The official implementation consists of `opl3.c` and `opl3.h` and is the
required initial emulator and behavioral reference. Its public API provides chip
reset, register writes (direct or buffered), and native/resampled stereo sample
generation. It is licensed LGPL-2.1-or-later.

The fast fork is a source-level API replacement pinned to an earlier upstream
1.8 revision. It adds cached waveform/envelope/phase data, silent-slot and mix
fast paths, an optional generated waveform table, and other optimizations. Its
README reports bit-exact output against its pinned upstream revision. It is also
LGPL-2.1-or-later and may be used only if profiling shows the official emulator
cannot meet real-time deadlines and project-specific sample-exact tests confirm
equivalence for all Wolf3D music and AdLib effects.

PrBoom+'s pure-C DBOPL is the supported lower-resource alternative. Its output
is not expected to be sample-identical to Nuked and it does not replace Nuked
as the behavioral reference. Historical MAME FMOPL remains excluded.

## Game Engine Black Book

Paths:

- `gebb\gebbwolf3d.pdf` — 320-page rendered book.
- `gebb\gebbwolf3` — authoritative, searchable LaTeX and illustration sources;
  origin `https://github.com/fabiensanglard/gebbwolf3.git`, commit
  `ecbdf073a1444b170330190e9aec96d21390e4ae`.

Highest-value sources for this port:

- `src\software_architecture.tex`: original subsystem map, startup, memory/page
  managers, cache manager, video/input/sound boundaries.
- `src\software_2d.tex`: menu renderer.
- `src\software_3d.tex`: per-frame flow, fixed-point raycaster, walls, sprites,
  scaler generation, and AI.
- `src\audio.tex`: timer interrupt, 70/700 Hz heartbeats, IMF, AdLib, digitized
  effects, and PC speaker.
- `src\inputs.tex`: keyboard, mouse, and joystick hardware behavior.
- `src\tricks.tex`: lookup tables, fizzle fade, palette effects, and PRNG.
- `src\hardware.tex`: the DOS/386/VGA constraints that explain original code.
- `src\ports.tex`: historical ports and platform tradeoffs.
- `tools`: small explanatory programs for VGA planes, scaling, raycasting, audio,
  and performance; useful for understanding, not production dependencies.

The hundreds of PNG/JPG/SVG/EPS files are book figures, diagrams, covers, and
100/300-DPI variants referenced by the LaTeX. They are explanatory media rather
than game assets.

## Original-build reconstruction

Paths:

- `gebb\Compile like it's 1992.pdf` — 21-page website printout.
- `gebb\Compile like it's 1992` — downloaded HTML, scripts/styles, and screenshots.

This documents the Borland C++ 3.1/DOSBox environment, original source layout,
build fixes, and need for external VGA/game data. It is useful for establishing
an original DOS baseline, but is not the modern build plan.

## External local validation material

Supplied inside the trusted reference tree:

- `game files\WL1 - Shareware v1.4`: the complete Apogee shareware data set,
  original executable, configuration, and `file_id.diz`.
- `game files\WL6 - Apogee v1.4`: the original full Apogee data set,
  executable, and configuration, including its expanded graphics layout.
- `game files\WL6 - GT v1.4`: the complete GT/ID/Activision full data set,
  original executable, and configuration.

SHA-256 values for the required data archives are published in
[`supported-data.md`](supported-data.md); executables and runtime files remain
external validation material rather than engine inputs.

Also available as read-only installations outside the trusted reference tree:

- `C:\GOG Games\Wolfenstein 3D`: complete v1.4 `WL6` data, original
  `Wolf3d.exe`, DOSBox configuration, and DOSBox executable.
- `C:\GOG Games\Spear of Destiny`: complete `SOD` data for missions 1–3.

The data files may be read in place by local tests and the original executables
may be run as behavioral oracles. No full-game data may be copied into the
repository or published. Shareware redistribution terms will be checked before
deciding whether even that data should appear in fixtures; the safe default is
to keep all game data external.

## Lookup rules during implementation

1. Read the original routine and its callers first.
2. Read the corresponding Black Book section for hardware-era intent.
3. Compare Chocolate and Wolf4SDL translations for portability hazards.
4. Prefer the smallest C translation that preserves original output and timing.
5. Consult doomgeneric only when shaping a platform-facing seam or example host.
6. Record any deliberate behavioral divergence in the source and test notes.
