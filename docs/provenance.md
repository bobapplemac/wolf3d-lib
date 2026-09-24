# Source provenance

The implementation follows the source priority and lookup rules in
[`reference-index.md`](reference-index.md).

The authoritative gameplay source is id Software's Wolfenstein 3D release at
commit `05167784ef009d0d0daefe8d012b027f39dc8541`. Portable translations may
consult Wolf4SDL and Chocolate Wolfenstein 3D; copied or adapted routines must
retain applicable notices and remain compatible with GPL-2.0-only.

Nuked-OPL3 will be maintained as a separately identified LGPL-2.1-or-later
third-party component when audio implementation begins. Its source and license
will accompany distributed builds. The MAME and DOSBox OPL implementations are
not used.

`src/WOLFPAL.inc` is the original Wolfenstein 3D 256-color VGA palette in the
portable initializer format used by Chocolate Wolfenstein 3D. Its 6-bit channel
values are converted to 8-bit values at compile time.

The resource readers follow the original `ID_CA.C`, `ID_PM.C`, and generated
graphics/audio headers. Wolf4SDL's `id_ca.cpp` and `id_pm.cpp` were used to
cross-check intended behavior after removing segmented-memory and stdio
assumptions. The implementations here are new, bounds-checked C99 code rather
than copied port code.

The initial runtime-level conversion follows `SetupGameLevel` in original
`WL_GAME.C`, including `AREATILE`, player object codes 19--22, tile-center
coordinates, starting-angle mapping, and the `SpawnDoor` scan contract from
`WL_GAME.C`/`WL_ACT1.C`. The deterministic random table and
increment-before-lookup behavior follow original `US_InitRndT`/`US_RndT` in
`ID_US_1.C`; Wolf4SDL's `id_us_1.cpp` was used to cross-check the portable byte
index behavior.

The fixed-point and view-table implementation follows original `BuildTables`,
`CalcProjection`, and `FixedByFrac` in `WL_MAIN.C` and `WL_DRAW.C`. Wolf4SDL's
corresponding routines were used to identify the safe cardinal-angle assignments
and the conversion from the DOS assembly routine's sign-magnitude fraction to
ordinary signed fixed-point values.

The portable wall-post scaler follows the original `ScalePost` sampling contract
in `WL_DRAW.C`. Wolf4SDL's C replacement was consulted to translate the compiled
scaler and VGA plane-mask behavior into direct indexed-framebuffer writes.

Ray traversal and wall-hit calculations follow original `AsmRefresh`,
`HitVertWall`, `HitHorizWall`, `HitVertDoor`, `HitHorizDoor`, and `CalcHeight`
in `WL_DR_A.ASM`/`WL_DRAW.C`.
Wolf4SDL's structured `AsmRefresh` translation was used to make the assembly
control flow explicit while retaining the original fixed-point stepping order.
The play-view clear colors and 160-line layout follow original
`vgaCeiling`/`VGAClearScreen` in `WL_DRAW.C`.

Wolfenstein 3D data and executables are external test inputs. No game assets are
part of this repository or covered by its license.
