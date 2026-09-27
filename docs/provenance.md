# Source provenance

The implementation follows the source priority and lookup rules in
[`reference-index.md`](reference-index.md).

The authoritative gameplay source is id Software's Wolfenstein 3D release at
commit `05167784ef009d0d0daefe8d012b027f39dc8541`. Portable translations may
consult Wolf4SDL and Chocolate Wolfenstein 3D; copied or adapted routines must
retain applicable notices and remain compatible with GPL-2.0-only.

Nuked-OPL3 is maintained as a separately identified LGPL-2.1-or-later
third-party component. Its exact upstream source, license, and local provenance
notice accompany the tree. The MAME and DOSBox OPL implementations are not
used.

`src/WOLFPAL.inc` is the original Wolfenstein 3D 256-color VGA palette in the
portable initializer format used by Chocolate Wolfenstein 3D. Its 6-bit channel
values are converted to 8-bit values at compile time.

The SIGNON bottom-strip prompt follows original `FinishSignon` in `WL_MAIN.C`,
including its 300-pixel clear, centered font-zero text, yellow acknowledgement
prompt, green working prompt, and separate timed Spear behavior.

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
Static HUD placement and number formatting follow `DrawStatusBar`,
`StatusDrawPic`, `LatchNumber`, and the individual draw routines in original
`WL_GAME.C`/`WL_AGENT.C`.
First-person weapon selection and scaling follow `DrawPlayerWeapon` and
`SimpleScaleShape` in original `WL_DRAW.C`; the portable scaler consumes the
already validated compiled-sprite representation from `WG_ASSETS.c`.
Static-object scanning and shape assignment follow `ScanInfoPlane`,
`InitStaticList`, `SpawnStatic`, and the `statinfo` table in original
`WL_GAME.C`/`WL_ACT1.C`. Projection, far-to-near ordering, and wall-column
occlusion follow `TransformTile`, `DrawScaleds`, `spotvis`, and `ScaleShape` in
original `WL_DRAW.C`/`WL_SCALE.C`.
Static item identities also follow the original `statinfo` table. Pickup
effects, capacity rejection, health and ammo clamps, weapon promotion, key
bits, treasure scoring, and 40,000-point extra-life thresholds are direct
translations of `GetBonus`, `HealSelf`, `GiveAmmo`, `GiveWeapon`, `GiveKey`,
`GivePoints`, and `GiveExtraMan` in original `WL_AGENT.C`. The portable
`removed` flag represents the original `shapenum = -1` consumption marker.
Player turning, strafing, forward/backward thrust, speed limiting, axis-sliding
collision, exit detection, and cardinal use targeting follow `ControlMovement`,
`TryMove`, `ClipMove`, `Thrust`, and `Cmd_Use` in original `WL_AGENT.C`.
Door locks and the closed/opening/open/closing state machine follow `OpenDoor`,
`CloseDoor`, `OperateDoor`, `DoorOpen`, `DoorOpening`, `DoorClosing`, and
`MoveDoors` in original `WL_ACT1.C`. Ambush tiles and door-floor area assignment
follow the post-scan cleanup and `SpawnDoor` logic in original `WL_GAME.C`.
Standing and patrolling guard map codes and difficulty fallthrough follow
`ScanInfoPlane` in original `WL_GAME.C`. Their tile-center construction,
direction mapping, and patrol destination adjustment follow `SpawnNewObj`,
`SpawnStand`, and `SpawnPatrol` in original `WL_STATE.C`/`WL_ACT2.C`. Actor
projection and eight-way standing-frame selection follow `TransformActor` and
`CalcRotate` in original `WL_DRAW.C`.

Wolfenstein 3D data archives and executables remain external test inputs. The
sole embedded game-art exception is `src/WG_SIGNON_ASSETS.inc`, generated from
the five original 64,000-byte executable-linked SIGNON templates by
`tools/GENERATE_SIGNON_ASSETS.ps1`. Those images remain original game artwork
and are not covered by the wolf3dgeneric source license.
