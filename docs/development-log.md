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

## 2026-09-24: Dog chase and bite

- Scope: original dog-only `T_DogChase`, door-avoiding dodge selection,
  movement-step leap test, five-state jump sequence, and `T_Bite` hit and
  damage rolls.
- The movement test covers the dog's doubled first-sighting speed, seeded
  diagonal dodge, destination reservation, leap threshold, exact sprite order,
  bite action timing, and deterministic six-point damage.
- The diagnostic selects an original E1M1 dog, finds a clear two-tile viewing
  pose, and executes the second jump state's action. The captured third jump
  frame shows the dog airborne with health reduced from 100% to 94%.
- 320x200 indexed-frame FNV-1a: `2391755b562c8744`
- Identical result with the supplied WL1 and WL6 data sets.

![E1M1 dog landing a bite](../out/dog-bite.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --bite-view --frame-hash --dump-frame out\dog-bite.ppm
```

## 2026-09-24: Hitscan boss bursts

- Scope: original Hans, Gretel, Mecha-Hitler, and Hitler shooting state tables,
  attack selection, sprite sequences, timing, and repeated `T_Shoot` actions.
- Hans and Gretel retain their eight-state, six-shot bursts. Mecha-Hitler and
  Hitler retain their six-state, five-shot bursts. Tests cover all four tables,
  including the 30-tic windup, alternating firing shapes, action cadence, and
  Hans's boss-only distance advantage.
- The E1M9 diagnostic frames Hans at three tiles, executes the first shot, and
  captures his third firing sprite with health reduced from 100% to 87%.
- 320x200 indexed-frame FNV-1a: `cd8a4e5001298ee6`
- Identical result with the supplied WL1 and WL6 data sets.

![Hans Grosse firing his original burst](../out/hans-firing.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --map 8 --boss-fire-view --frame-hash --dump-frame out\hans-firing.ppm
```

## 2026-09-24: Schabbs syringe projectiles

- Scope: original `T_Schabb` attack probability, two-state throw sequence,
  360-angle aim, four-frame syringe animation, `T_Projectile` motion, wall and
  player collision, damage, removal, and bounded actor-slot reuse.
- Focused tests verify the 30-tic windup and throw action, exact projectile
  speed and first animation transition, straight-ahead fixed-point movement,
  seeded 21-point player damage, and solid-wall removal.
- The full-data diagnostic uses E2M9, holds Schabbs in his second throw frame,
  and advances the syringe nine original 70 Hz tics toward the player.
- 320x200 indexed-frame FNV-1a: `581ce5d3716e7ca0`
- This checkpoint requires WL6 because the shareware WL1 data contains only
  Episode 1.

![Dr. Schabbs throwing a syringe](../out/schabbs-needle.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --map 18 --needle-view --frame-hash --dump-frame out\schabbs-needle.ppm
```

## 2026-09-24: Giftmacher and Fatface rockets

- Scope: original `T_Gift`/`T_Fat` attack and close-range retreat behavior,
  Giftmacher's two-state throw, Fatface's rocket-plus-four-shot sequence,
  directional rocket rendering, smoke trail, wall explosion, and player damage.
- Focused tests verify both firing tables, `SelectRunDir`, rocket aim and speed,
  angle-based sprite rotation data, smoke creation and timing, seeded 31-point
  damage, wall impact, and the first two explosion frames.
- The full-data diagnostic uses E4M9, holds Giftmacher in his second firing
  frame, and advances a rocket eleven original 70 Hz tics with its smoke trail.
- 320x200 indexed-frame FNV-1a: `c85b4f4095ac18f2`
- This checkpoint requires WL6 because the shareware WL1 data contains only
  Episode 1.

![Giftmacher firing a rocket](../out/gift-rocket.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --map 38 --rocket-view --frame-hash --dump-frame out\gift-rocket.ppm
```

## 2026-09-24: Fake Hitler flame burst

- Scope: original `T_Fake` attack probability and dodge-only movement,
  nine-state firing table, eight `T_FakeFire` actions, two-frame flame
  animation, projectile motion, collision, and damage.
- Focused tests verify the constant firing sprite and eight-tic cadence, all
  eight emissions, 360-angle aim, `0x1200` speed, animation timing, seeded
  one-point damage, and the distinct `tics << 1` attack threshold.
- The full-data diagnostic uses E3M9 and advances the first four flames of the
  burst through their alternating animation and cumulative damage.
- 320x200 indexed-frame FNV-1a: `ba572ca325d89701`
- This checkpoint requires WL6 because the shareware WL1 data contains only
  Episode 1.

![Fake Hitler's flame burst](../out/fake-flames.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --map 28 --flame-view --frame-hash --dump-frame out\fake-flames.ppm
```

## 2026-09-24: Moving pushwalls

- Added the historically owned `WL_ACT1.c` translation for `PushWall` and
  `MovePWalls`, including one-active-wall gating, secret counting, forward-cell
  reservation, blocking actors/statics, tile/area handoff, and original state
  and position arithmetic.
- Restored the original blocking classification for Wolf3D static objects so
  scenery participates correctly in pushwall obstruction checks.
- Ported the vertical and horizontal moving-wall hit calculations from
  `WL_DRAW.C`; ray distance and texture sampling now follow the fractional wall
  plane instead of treating the occupied map cell as fixed geometry.
- Focused tests cover activation and rejection, halfway ray geometry, first-cell
  crossing, actor obstruction, full travel, and blocking versus non-blocking
  statics.
- The E1M1 diagnostic captures the first secret portrait wall at the original
  half-tile position.
- 320x200 indexed-frame FNV-1a: `48bdacd231ece2d9`
- Identical result with the supplied WL1 and WL6 data sets.

![E1M1 portrait pushwall halfway into motion](../out/moving-pushwall.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --pushwall-view --frame-hash --dump-frame out\moving-pushwall.ppm
```

## 2026-09-24: Ordinary enemy damage and death

- Added the original difficulty-indexed hit points to the portable actor
  structure for guards, officers, SS, dogs, mutants, ghosts, and Wolf3D bosses.
- Ported `DamageActor`'s noise flag, double damage against an unaware enemy,
  first-sighting transition, and parity-selected pain frames.
- Ported ordinary-enemy scoring, kill counting, non-marking corpses, ammo and
  machine-gun drops, and the class-specific death animation timing and sprites.
- Restored the original special two-way pain-frame rotation calculation while
  retaining the ordinary eight-way actor rotation path.
- Focused tests cover hit points, surprise damage, combat activation, pain
  recovery, corpse sequencing, scoring, drops, and the dog's looping dead state.
- The E1M1 diagnostic advances a guard 30 tics into his collapse and shows the
  dropped ammo clip behind him.
- 320x200 indexed-frame FNV-1a: `1a6b2675d0e3e31b` (updated when the
  runtime score was connected to the HUD)
- Identical result with the supplied WL1 and WL6 data sets.

![E1M1 guard in the third death frame](../out/guard-death.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --death-view --frame-hash --dump-frame out\guard-death.ppm
```

## 2026-09-24: Wolf3D boss damage and death

- Extended `DamageActor` and `KillActor` across Hans, Gretel, Schabbs, Fake
  Hitler, Mecha Hitler, Hitler, Giftmacher, and Fatface with original scores,
  gold-key drops, corpse marking, and kill-position capture.
- Ported each boss's distinct sprite sequence and timing. Until digital audio
  is present, speech-hold frames use the original `sds_Off` durations.
- Terminal Schabbs, Giftmacher, Fatface, and Hitler corpses now drive the two
  successive death-camera actions into victory and level-complete state.
- Mecha Hitler's third death action spawns a separate Hitler actor with the
  original difficulty-indexed 500/700/800/900 hit points, chase speed, inherited
  position and direction, and independent final death sequence.
- Focused tests exercise every boss chain, scores, keys, terminal timing,
  victory transitions, both Mecha/Hitler health bars, and morph behavior.
- The E1M9 diagnostic captures Hans after 30 tics in his third collapse frame.
- 320x200 indexed-frame FNV-1a: `2a5b6e8514c2298b` (updated when the
  runtime score was connected to the HUD)
- Identical result with the supplied WL1 and WL6 data sets.

![Hans Grosse in the third death frame](../out/hans-death.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --map 8 --boss-death-view --frame-hash --dump-frame out\hans-death.ppm
```

## 2026-09-24: Player weapon attacks

- Ported the four original `attackinfo` tables and their six-tic knife, pistol,
  machine-gun, and chaingun frame/action cadence into `WL_AGENT.c`.
- `DrawScaleds` now retains each actor's original-style projected `viewx` and
  `transx` targeting values alongside `FL_VISABLE`.
- Ported `KnifeAttack` and `GunAttack`: center-screen and nearest-target tests,
  knife reach, gun line tracing, tile-distance accuracy, random consumption,
  damage divisors, noise, ammo use, held automatic fire, and empty-gun fallback.
- Connected live score, ammo, weapon selection, and weapon frame state to the
  status bar and first-person weapon renderer.
- Focused tests cover close shots, blocked shots, long-range misses, knife
  range/damage, pistol exhaustion, and machine-gun/chaingun repeat loops.
- The E1M1 diagnostic fires a seeded pistol shot through the complete target,
  damage, death, ammo, score, HUD, and recoil-frame path.
- 320x200 indexed-frame FNV-1a: `6fb9a7201588ef0c`
- Identical result with the supplied WL1 and WL6 data sets.

![BJ firing the pistol at an E1M1 guard](../out/player-pistol.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --player-fire-view --frame-hash --dump-frame out\player-pistol.ppm
```
