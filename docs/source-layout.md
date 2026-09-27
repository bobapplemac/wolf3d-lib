# Source layout and lineage

wolf3dgeneric retains an original Wolfenstein 3D filename whenever a portable
translation has a clear owner in the 1992 source tree. This makes the repository
usable as a cross-reference: a reader can place the original and portable files
side by side and inspect the required modernization directly.

Source basenames use the original uppercase convention. Extensions use modern
lowercase `.c`, `.h`, and `.inc` forms for portable toolchain detection.

| Portable file | Original owner | Current responsibility |
| --- | --- | --- |
| `ID_CA.c` | `ID_CA.C` | Huffman, Carmack, and RLEW expansion |
| `ID_IN.c` | `ID_IN.C` | Two-port joystick state, original calibrated dead zone, and signed delta scaling |
| `ID_PM.c` | `ID_PM.C` | Bounded VSWAP page access |
| `ID_SD.c` | `ID_SD.C`, `ID_SD_A.ASM` | IMF/AdLib and 140 Hz PC-speaker service, VSWAP digitized effects, resampling, and PCM mixing |
| `ID_VL.c` | `ID_VL.C` | Indexed framebuffer, blits, palettes, and fizzle fade |
| `ID_VH.c` | `ID_VH.C` | Proportional font decoding and drawing |
| `ID_US_1.c` | `ID_US_1.C`, `ID_IN.C` | Original deterministic random table and `US_LineInput` scan-to-ASCII translation |
| `WL_AGENT.c` | `WL_AGENT.C`, `WL_GAME.C` | Player movement, use, combat, pickups, status, and HUD drawing |
| `WL_ACT1.c` | `WL_ACT1.C` | Door and pushwall state, obstruction, and movement |
| `WL_ACT2.c` | `WL_ACT2.C` | Actor construction and initial state metadata |
| `WL_STATE.c` | `WL_STATE.C`, `WL_ACT2.C` | Actor timing, path/chase/combat states, sight, and awareness |
| `WL_GAME.c` | `WL_GAME.C` | Runtime level, area, player, door, and static-object construction |
| `WL_PLAY.c` | `WL_PLAY.C`, `WL_AGENT.C` | Fixed 70 Hz play-loop ordering, input state, and control dispatch |
| `WL_MAIN.c` | `WL_MAIN.C` | SIGNON completion prompt, trigonometric tables, and projection setup |
| `WL_MENU.c` | `WL_MENU.C` | Control-panel menu drawing and selection movement |
| `WL_DRAW.c` | `WL_DRAW.C`, `WL_DR_A.ASM` | Fixed-point ray traversal, wall/door hits, and projected scenery ordering |
| `WL_INTER.c` | `WL_INTER.C`, `ID_US_1.C` | Floor statistics, bonuses, victory, and high-score presentation |
| `WL_TEXT.c` | `WL_TEXT.C` | Markup-driven help/ending article layout and word wrapping |
| `WL_SCALE.c` | `WL_SCALE.C`, `WL_DRAW.C` | Portable wall-post and occluded sprite scaling |

Files beginning with `WG_` have no single equivalent original translation unit.
They are deliberately small portability or safety layers:

| New file group | Reason it is new |
| --- | --- |
| `WG_DATA`, `WG_FILE`, `WG_ENDIAN` | Checked host filesystem and byte-order boundary |
| `WG_SAVE` | Versioned, pointer-free portable save-game encoding |
| `WG_CONFIG` | Versioned settings and high-score persistence independent of compiler structure layout |
| `WG_GRAPHICS`, `WG_MAPS`, `WG_AUDIO` | Owned, bounded resource objects replacing cache globals and far pointers |
| `WG_ASSETS` | Safe conversion from VSWAP-native wall/sprite layouts |
| `WG_FIXED` | Fixed-width helper independent of compiler integer models |
| `WG_PALETTE` | Read-only portable ownership of the VGA palette |
| `WG_RENDERER` | Composition layer over the translated drawing routines |
| `WG_PLATFORM` | The doomgeneric-style host contract |
| `WG_HEADLESS`, `WG_WIN32` | Reference implementations of that host contract |

`third_party/Nuked-OPL3` is intentionally outside both groups: it is the
unaltered LGPL OPL emulator selected by the project, built as its own library.

`WOLF3DGENERIC.c` and `WOLF3DGENERIC.h` are the new public engine boundary.
Routine and data names use the `WG_` namespace where exposing an original global
would create hidden ownership or host-size assumptions. Each translated legacy
file records the original routines it currently contains; as additional systems
are ported, code should move into its historical owner rather than accumulating
in a generic catch-all file.
