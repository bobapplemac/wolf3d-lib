# Development log

Graphical captures are generated from the user's external Wolfenstein 3D data
and remain under the Git-ignored `out/` directory. They are never distributed
with the source repository. Each entry records a deterministic indexed-frame
hash so the milestone remains verifiable even when the screenshot is absent.

## 2026-09-23: Initial E1M1 walls and doors

- Commit: `a3f6c09`
- Scope: fixed-point ray traversal, original wall textures, closed door plane,
  ceiling/floor colors; no actors, first-person weapon, or status bar.
- 320x200 indexed-frame FNV-1a: `52a9cf2dd9dcab66`
- Identical result with the supplied WL1 and WL6 data sets.

![Initial E1M1 wall and door view](../out/initial-play-view.png)

## 2026-09-23: Original static status bar

- Scope: `STATUSBARPIC`, initial BJ face, floor, score, lives, health, ammo,
  empty key slots, and pistol icon, ported into `WL_AGENT.c`.
- 320x200 indexed-frame FNV-1a: `b2f43cae26b556f5`
- Identical result with the supplied WL1 and WL6 data sets.

![Initial E1M1 view with status bar](../out/initial-hud-view.png)

Regenerate the latest capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --dump-frame out\initial-hud-view.ppm
```

The PPM is the authoritative output. The PNG shown above is a convenience copy
created locally for visual inspection.

## 2026-09-23: First-person ready pistol

- Scope: the original compiled pistol sprite rendered with a portable
  `SimpleScaleShape` translation at the original 161-unit weapon scale.
- 320x200 indexed-frame FNV-1a, including the HUD: `ab0c1a3f48fece62`
- Identical result with the supplied WL1 and WL6 data sets.

![Initial E1M1 view with pistol and status bar](../out/initial-weapon-view.png)

## 2026-09-23: Static objects and wall occlusion

- Scope: original plane-1 static-object scan, `TransformTile` projection,
  ray-traversal `spotvis`, far-to-near ordering, and per-column `ScaleShape`
  wall occlusion.
- The normal initial frame remains `ab0c1a3f48fece62`: its closed door correctly
  hides the corridor scenery.
- The diagnostic below uses the same E1M1 starting pose with all doors in their
  original fully-open state so ceiling lights and floor objects are visible.
- 320x200 indexed-frame FNV-1a, including weapon and HUD: `800512fcf839700f`
- Identical result with the supplied WL1 and WL6 data sets.
- Corrected during the guard milestone to use `ScaleShape`'s `height >> 3`
  world scale; the earlier capture incorrectly reused `SimpleScaleShape`'s
  first-person weapon scale.

![E1M1 open-door static-object rendering](../out/open-door-static-objects.png)

Regenerate the diagnostic capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --open-doors --dump-frame out\open-door-static-objects.ppm
```

## 2026-09-23: Initial standing and patrolling guards

- Scope: original easy/medium/hard guard map-code layering, `SpawnStand`,
  initial `SpawnPatrol` placement, `TransformActor`, neighboring-tile visibility,
  and eight-way `CalcRotate` sprite selection.
- E1M1 medium difficulty creates 17 guards; baby creates 10 and hard creates 32.
- 320x200 indexed-frame FNV-1a, including weapon and HUD: `a6db229142f7150b`
- Identical result with the supplied WL1 and WL6 data sets.
- The diagnostic pose is three tiles west of E1M1's final map-order guard and
  does not alter ordinary level initialization.

![E1M1 standing guard rendering](../out/standing-guard.png)

Regenerate the guard capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --guard-view --dump-frame out\standing-guard.ppm
```

## 2026-09-23: Ordinary enemy setup and dead guards

- Scope: original map-code setup for guards, officers, SS, dogs, mutants, and
  inert dead guards, including each difficulty band and initial stand/patrol
  state shape.
- E1M1 medium difficulty creates 17 live guards, three dogs, and one inert dead
  guard; baby creates 12 total actors and hard creates 38.
- 320x200 indexed-frame FNV-1a, including weapon and HUD: `0b077346cfd7b513`
- Identical result with the supplied WL1 and WL6 data sets.
- The corpse and locked door in this normal starting pose are both original
  E1M1 map objects; no diagnostic level-state changes are applied.

![E1M1 original dead guard and locked door](../out/initial-standard-actors.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --dump-frame out\initial-standard-actors.ppm
```

## 2026-09-23: Boss and ghost setup

- Scope: original fixed-shape setup for Hans and Gretel Grosse, Dr. Schabbs,
  Giftmacher, Fatface, Fake Hitler, Mecha Hitler, and the four Pac-Man ghosts.
- All 10 WL1 and 60 WL6 maps are checked to ensure every spawned actor's initial
  sprite page is present in that edition's sparse VSWAP table.
- Added zero-based `--map`, `--boss-view`, and `--frame-hash` headless
  diagnostics. The boss pose is selected only along a clear three-tile path.
- 320x200 indexed-frame FNV-1a: `a38a57b87c979508`
- Identical result with the supplied WL1 and WL6 data sets.

![Hans Grosse on the original Episode 1 boss map](../out/hans-grosse.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --map 8 --boss-view --frame-hash --dump-frame out\hans-grosse.ppm
```

## 2026-09-23: Timed patrol movement

- Scope: original randomized initial state phase, six-state walk animation,
  `T_Path` distance consumption, `SelectPathDir` map arrows, tile-center
  snapping, class speeds, collision checks, and closed-door waiting.
- The corrected diagnostic advances all actors by 127 original 70 Hz tics,
  landing on a state with a `T_Path` thinker, opens doors to keep the selected
  E1M1 patrol visible, and chooses a clear viewing pose.
- Corrected the first implementation so the short `path1s` and `path3s` states
  no longer move; the original state table assigns them no thinker.
- 320x200 indexed-frame FNV-1a: `0460cc1c73d44f60`
- Identical result with the supplied WL1 and WL6 data sets.

![E1M1 guard following his original patrol path](../out/moving-patrol.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --open-doors --patrol-view --actor-tics 127 --frame-hash --dump-frame out\moving-patrol.ppm
```

## 2026-09-23: Area connectivity and first sighting

- Scope: original 37-area bookkeeping, ambush-tile replacement, open-door area
  connectivity, 1/256-tile `CheckLine`, close/facing `CheckSight` rules,
  ambush/noise handling, class reaction delays, and `FirstSighting` chase-state
  transition and speed changes.
- Focused synthetic maps verify walls, closed and fully open doors, facing,
  ambush setup, deterministic random delay, and the guard's first chase frame.
- The diagnostic uses the established E1M1 guard pose and advances awareness in
  two calls: one detection tic followed by enough tics for the original maximum
  reaction delay. At this checkpoint, chase direction selection and attacks
  intentionally remained inactive.
- 320x200 indexed-frame FNV-1a: `63df8fa451a6ba6b`
- Identical result with the supplied WL1 and WL6 data sets.

![E1M1 guard after first sighting](../out/alerted-guard.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --alert-view --frame-hash --dump-frame out\alerted-guard.ppm
```

## 2026-09-23: Guard chase movement

- Scope: original six-state chase animation, `SelectChaseDir`,
  `SelectDodgeDir`, destination reservation, collision fallback ordering,
  closed-door waiting, tile-center correction, and `MINACTORDIST` player
  separation.
- The ranged attack probability consumes the original random value. At this
  checkpoint, a selected attack was held as an explicit pending state for the
  following combat slice.
- Focused tests cover deterministic first-attack diagonal dodging, chase
  animation pauses, rotated chase shapes, attack selection, and waiting at and
  crossing a door.
- The diagnostic advances the alerted E1M1 guard by nine original 70 Hz tics,
  short enough for the seeded attack roll to fail and expose chase movement.
- 320x200 indexed-frame FNV-1a: `8485e203e6d30079`
- Identical result with the supplied WL1 and WL6 data sets.

![E1M1 guard beginning his chase](../out/chasing-guard.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --chase-view --actor-tics 9 --frame-hash --dump-frame out\chasing-guard.ppm
```

## 2026-09-24: Ordinary enemy ranged attacks

- Scope: original guard, officer, mutant, and SS shooting state tables;
  `T_Shoot` hit chance and distance-scaled damage; player health, baby-mode
  damage reduction, death marking, and damage-flash accumulation.
- Restored the actor flag bits to the original `FL_SHOOTABLE`, `FL_VISABLE`,
  `FL_ATTACKMODE`, `FL_FIRSTATTACK`, and `FL_AMBUSH` values. `DrawScaleds` now
  maintains visibility on each actor for the shooting accuracy branch.
- Tests cover every ordinary ranged class's distinct timings, sprite order, and
  shot actions, deterministic hit/damage rolls, baby-mode quarter damage, and
  lethal damage.
- The diagnostic selects E1M1's established guard pose, executes one original
  shot action, and renders the recoil frame with health reduced from 100% to
  91% by the seeded roll.
- 320x200 indexed-frame FNV-1a: `dbfa84e64bb68acf`
- Identical result with the supplied WL1 and WL6 data sets.

![E1M1 guard firing and damaging the player](../out/guard-firing.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --fire-view --frame-hash --dump-frame out\guard-firing.ppm
```
