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

## 2026-09-24: Item pickups and inventory effects

- Restored the original `statinfo` item identities alongside each portable
  static sprite, including both gib entries and the duplicate dropped clip.
- Ported `GetBonus`, `GiveAmmo`, `GiveWeapon`, `GivePoints`, `GiveExtraMan`,
  and `HealSelf` behavior from `WL_AGENT.C`: rejection at full capacity,
  original health/ammo values, weapon upgrades, key bits, treasure scores,
  40,000-point extra lives, and the full-heal one-up are preserved.
- Enemy ammo, machine-gun, and boss-key drops now use the same collectible
  item types as map-authored bonuses.
- Consumed statics disappear from sprite rendering and no longer obstruct
  pushwalls; player-tile collection provides the movement-facing integration
  point for the eventual interactive loop.
- Connected live key and life inventory to the status bar.
- Focused tests cover every Wolf3D bonus type, capacity rejection, healing
  thresholds, score/life thresholds, repeat collection, map item decoding, and
  enemy drop identities across all supplied maps.
- The E1M1 diagnostic collects the first cross and frames the treasure room
  with the cross removed and the score advanced to 100.
- 320x200 indexed-frame FNV-1a: `3745f8af67b60f69`
- Identical result with the supplied WL1 and WL6 data sets.

![E1M1 treasure room after collecting a cross](../out/pickup-cross.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --pickup-view --frame-hash --dump-frame out\pickup-cross.ppm
```

## 2026-09-24: Player movement, use actions, and live doors

- Ported the original `ControlMovement`, `TryMove`, `ClipMove`, and `Thrust`
  paths with angle-fraction accumulation, forward/backward speed scales,
  strafing, the original movement cap, fixed-radius collision, and axis sliding.
- Player collision now accounts for walls, partially open doors, blocking
  statics, and shootable actors. Successful movement updates tile/area state,
  collects bonuses, and recognizes the original exit-tile marker.
- Ported cardinal `Cmd_Use` dispatch for pushwalls, elevators, secret exits,
  locked doors, and ordinary doors.
- Added the original closed/opening/open/closing door cycle, 64-tic travel,
  300-tic open wait, lock/key checks, player/actor obstruction reversal, and
  area connectivity from the first opening tic.
- Enemies waiting at a door now request that it open, matching the original
  chase and path logic rather than waiting on externally forced door state.
- Corrected `SetupGameLevel` parity by converting ambush markers to walkable
  floor after retaining their spawn flag and by assigning door tiles an
  adjacent floor area.
- Focused tests cover wall/corner sliding, static and actor collision, movement
  pickup and exit effects, turning, locked doors, partial/full door collision,
  automatic closing, obstruction reversal, area connectivity, pushwall use,
  and elevator completion.
- The E1M1 diagnostic uses a real door and renders its sliding plane after 32
  of the original 64 opening tics.
- 320x200 indexed-frame FNV-1a: `d5720a5ad520646d`
- Identical result with the supplied WL1 and WL6 data sets.

![E1M1 door halfway open after use](../out/door-use.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --door-use-view --frame-hash --dump-frame out\door-use.ppm
```

## 2026-09-24: Persistent 70 Hz interactive play loop

- Added `WL_PLAY.c` under its historical source owner. One deterministic tic
  preserves the original high-level order: advance doors, advance pushwalls,
  process the player, then process the remaining actors.
- Split host key sampling from the fixed simulation step. The Win32 host uses
  an integer accumulator that averages exactly 70 game tics per second without
  assuming a 14 ms tic or coupling gameplay to rendering.
- Restored the original keyboard defaults: arrows, Alt-strafe, Shift-run,
  Control-fire, Space-use, and weapon keys 1-4. Escape cleanly leaves the host.
- Added persistent map, page, wall, graphics, projection, and input ownership
  to the generic runtime, replacing the former one-frame interactive display.
- Kept the headless diagnostic path byte-for-byte stable and added a complete
  play-loop capture that holds forward for 35 tics.
- Focused tests cover base-speed motion, angle changes, weapon selection,
  held-fire timing, use-edge behavior, and door-before-player ordering.
- A live Win32 smoke test opened the shareware session and remained responsive.
- 320x200 indexed-frame FNV-1a: `41b5819e00a95446`
- Identical result with the supplied WL1 and WL6 data sets.

![E1M1 after 35 forward play-loop tics](../out/play-loop-forward.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --forward-tics 35 --frame-hash --dump-frame out\play-loop-forward.ppm
```

## 2026-09-24: IMF music and official Nuked-OPL3

- Ported the active IMF sequencer from `ID_SD.C`, including length-prefixed
  event validation, zero-delay event batching, register/delay ordering, and
  the original loop restart behavior.
- Added a rational sample clock that produces exactly 700 IMF services per
  second at both 44.1 and 48 kHz. Music timing is driven by generated sample
  count and remains independent of the 70 Hz gameplay accumulator.
- Vendored the official `nukeykt/Nuked-OPL3` revision
  `765ec962e473aeb767e4cba74ffdc8f588ffbfe8`, with its LGPL-2.1-or-later
  license and an independently replaceable CMake target. The reference files
  match the supplied checkout after line-ending normalization.
- Restored the original 60-map song table from `WL_PLAY.C`. E1M1 therefore
  starts `GETTHEM_MUS`, rather than a generic test track.
- Added platform-neutral signed stereo PCM submission. Win32 uses four 1,024-
  frame `waveOut` buffers; the headless host can export a deterministic WAV.
- Validated all 11 populated shareware and all 27 registered-edition music
  chunks. The sparse shareware music slots remain valid empty archive entries.
- One second of E1M1 music at 48 kHz has PCM FNV-1a `201858e57f147650`,
  identical for supplied WL1/WL6 data and MSVC x86/x64 builds.
- The ten-second WAV fixture is 48 kHz, stereo, signed 16-bit PCM with SHA-256
  `40ecca1c103a8e38dab7e7eb4725f268348c18e8120a6c5e996984544df15e1e`.

![Ten-second E1M1 Nuked-OPL3 reference render](../out/e1m1-nuked-opl3.wav)

Regenerate it with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --map 0 --dump-music out\e1m1-nuked-opl3.wav
```

## 2026-09-24: AdLib effects in the live mixer

- Ported `SDL_AlSetFXInst`, `SDL_ALPlaySound`, and `SDL_ALSoundService` behavior
  into the portable `ID_SD.c` owner: channel-0 operator programming, block/key
  state, zero-pitch key-off, stream completion, and sound priority are retained.
- Effects advance at the original 140 Hz—every fifth 700 Hz audio service—and
  share the same Nuked chip and PCM stream as IMF music.
- Added a bounded per-level sound event queue. Player weapon frames, opening
  and closing doors, locked doors, and pushwalls now emit their original sound
  numbers; the runtime resolves their AUDIO chunks into the priority mixer.
- Reduced Win32 transport blocks to 512 frames, bounding queued device latency
  to roughly 43 ms while retaining four reusable buffers.
- Validated all 87 AdLib effect chunks in both supplied editions, lower-priority
  rejection, service completion, and deterministic music/effect synthesis.
- The first 0.1 seconds of E1M1 music plus the pistol has PCM FNV-1a
  `bebd8fbdef66d214` in both editions.
- The ten-second mixed WAV has SHA-256
  `ebe613d9a705ab31add505e3bf581646ed9ed71ea4ac5951179d9983efb701fd`.

![E1M1 music with the original AdLib pistol effect](../out/e1m1-pistol-opl.wav)

Regenerate it with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --map 0 --sound 24 --dump-music out\e1m1-pistol-opl.wav
```
