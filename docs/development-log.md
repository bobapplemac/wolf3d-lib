# Development log

## 2026-09-29: Prompt-safe F11 fullscreen toggle

- Added F11 alongside Alt+Enter as a fullscreen toggle in the native Win32 and
  cross-platform SDL3 hosts.
- Consumed F11 key-down, repeat, and key-up events entirely at the host layer.
  It can therefore change presentation during SIGNON, Get Psyched,
  intermissions, cheat warnings, and other acknowledgement waits without being
  observed as the key that dismisses them.
- Retained Alt+Enter for platform convention and left the generic engine input
  contract unchanged.

## 2026-09-29: Original M-L-I gameplay cheat

- Restored the original simultaneous `M` + `L` + `I` gameplay chord across
  every wrapper through the generic scan-code event path.
- Matched `CheckKeys` exactly: health becomes 100, ammo 99, both keys are
  granted, the chaingun becomes the active/best/chosen weapon when needed,
  score is reset to zero, and 42,000 tics (ten minutes) are added to the level
  timer. The original had no separate high-score-disqualification flag.
- Restored the original five-line warning as a modal in-game message. A fresh
  keyboard, mouse, or enabled-controller input dismisses it while OPL music
  continues independently, matching the DOS acknowledgement wait.
- Added direct regression coverage for every gameplay-state consequence.

## 2026-09-29: Sound Blaster FM and digitized-output balance

- Restored a mixer policy that was implicit in the original hardware path:
  `SDL_StartSB` set the Sound Blaster Pro FM mixer to maximum with the explicit
  intent of matching its digitized output level.
- Kept the official Nuked-OPL3 implementation untouched and applied a named 4x
  FM-bus gain at the portable mixer boundary, before digitized and PC-speaker
  buses are added. Music and AdLib effects continue to share the same physical
  OPL model and therefore receive identical gain.
- Calibrated the gain against the available WL1 music and digitized pistol
  data. The loudest ten-second music sample remained below clipping after
  gain, and the combined music/AdLib-effect corpus peaked at 23,208 of 32,767.
- Updated the deterministic music, mixed AdLib, and mixed PC-speaker PCM
  fixtures. The digitized-only fixture remains byte-identical.

## 2026-09-28: Persistent floor HUD and original portrait updates

- Fixed the active-game status construction to retain `level.map_number`.
  Get Psyched already showed the newly loaded floor correctly, but the first
  rendered gameplay frame had left the status field at its Floor 1 default.
- Corrected the same omission in level-complete and victory status-bar
  reconstruction, preserving the displayed floor on all later transitions.
- Applied the same correction to the headless play-view diagnostic, turning
  the existing map-1 framebuffer checks into regressions for the live HUD.
- Removed the full-screen intermission rebuild added during the static-GOG-BJ
  investigation. Timed breathing once again changes only the portrait, like
  the original `BJ_Breathe`, while ordinary score changes continue to redraw
  the complete results content as required.

## 2026-09-28: Activision/GOG WL6 resource profile

- Compared the added GOG corpus byte-for-byte with GT WL6. Maps and audio are
  identical; the only decoded picture change is `L_GUYPIC`, which is an exact
  duplicate of `L_GUY2PIC`, and the only VSWAP page change is sprite 42
  (`SPR_STAT_40`, the original `Call Apogee` scenery object).
- Used the replacement VSWAP page's exact fingerprint to identify this data
  distribution without relying on a directory, executable, or storefront
  name. Automatic SIGNON selection now chooses the embedded Activision screen.
- Kept the GOG intermission portrait static because both animation frames in
  that release genuinely contain the same pixels; no art is borrowed from a
  different installation. Added the GOG corpus to data and SIGNON regression
  coverage on every configured compiler and architecture.

## 2026-09-27: Intermission portrait investigation and fullscreen cursor fix

- Revalidated GT WL6's original `L_GUYPIC`/`L_GUY2PIC` chunk mapping (43 and
  84) and retained the original first 11-tic, then 36-tic breathing cadence.
- Initially replaced portrait-only repaints during intermission sounds and the
  acknowledgment wait with complete results-screen rebuilds while diagnosing
  a reportedly static portrait. The later GOG corpus comparison proved its
  two source pictures are identical, so the engine has returned to the
  original `BJ_Breathe`-style direct portrait updates.
- The native Win32 and SDL3 wrappers now hide the system cursor in fullscreen
  and restore it when returning to windowed mode. Win32 also handles
  `WM_SETCURSOR`, preventing the class cursor from reappearing on movement.

## 2026-09-27: Live Change View projection update

- Restored the original `CP_ChangeView` to `NewViewSize` behavior: accepting a
  new size now recalculates the active session's projection table immediately.
- Kept the preview non-destructive and retained Escape's original cancellation
  behavior. Keyboard, mouse, and gamepad acceptance share the corrected path.

## 2026-09-27: Toggleable GUI fullscreen

- Added host-owned borderless desktop fullscreen without changing the generic
  engine's native framebuffer or aspect-correct presentation.
- Both the native Win32 and cross-platform SDL3 hosts accept `--fullscreen`
  and toggle between fullscreen and their prior window with Alt+Enter.
- The shortcut is consumed by the host so its Enter keystroke cannot activate
  a game menu item; returning to windowed mode restores the prior placement.

## 2026-09-27: Data-driven exit-screen prompt placement

- Replaced the assumed 25-row prompt position with a scan for the final
  nonblank CP437 cell row. This distinguishes the seven-row GT/ID/Activision
  `ORDERSCREEN` from the 24-row Apogee screen and applies equally to future
  short exit resources.
- Accounted for Windows GUI-subsystem launch behavior: `cmd.exe` renders its
  next prompt without waiting for the game, so writing the DOS screen had
  erased it. Both Win32 and SDL3 hosts now capture those actual console cells,
  draw the 80x25 resource, restore the real prompt immediately below its
  content, and leave the input cursor after it. Customized prompts and current
  paths are preserved rather than reconstructed.
- Made the content-row calculation shared with ANSI/plain terminal output and
  added synthetic edge cases plus real-data assertions: Apogee order/error
  screens occupy 24 rows, while later WL6 occupies 7/24 respectively.

## 2026-09-27: Original DOS console exit and launch controls

- Restored `Quit`'s original data-driven `ORDERSCREEN` and `ERRORSCREEN`
  resources for WL1, both WL6 layouts, SOD, SDM, and the mission disks. The
  library passes the decoded 80x25 CP437 character/VGA-attribute cells through
  its platform boundary before shutdown.
- Added native Win32 console-cell output and an ANSI/Unicode terminal path for
  Linux and headless hosts. Both preserve the original foreground/background
  colors; redirected output omits escape codes while retaining readable text.
- Restored the DOS argument convention (case-insensitive after leading
  punctuation) for `GOOBERS`, `DEBUGMODE`, `TEDLEVEL`, `NOWAIT`, and TED's
  four difficulty names. Implemented the original debug unlock chord and its
  principal level, god, hurt, item, and fast-quit keys.
- Added a distinct ordinary-message callback so debug text does not become a
  graphical error dialog. Fatal paths use the original colored error panel
  when game resources are available.
- Verified all text resources against the real data corpus. The full suite
  passes 147/147 under MSVC and 148/148 under both GCC and Clang.

## 2026-09-27: Apogee and later WL6 resource profiles

- Split full Wolfenstein 3D v1.4 into automatically detected Apogee and
  GT/ID/Activision graphics profiles while retaining `WL6` as the single user
  selector. The 162-entry Apogee `VGAHEAD.WL6` and 150-entry later header are
  unambiguous and share the same maps, sprites, audio, and gameplay.
- Routed title, menu, status, intermission, pause, Get Psyched, demo, help, and
  episode-ending chunks through the detected layout. Apogee uses the original
  144-picture corpus; later releases use the compacted 132-picture corpus.
  The decoded `T_HELPART` streams are byte-identical (41 pages in both sets),
  but their surrounding picture tables and article-frame chunk numbers differ.
- Added the supplied Apogee WL6 archive as an independent real-data test. It
  decodes every graphics chunk, renders all 41 help pages and all twelve
  ending pages, validates every map/sprite/audio resource, and checks all four
  complete demos separately from the existing later-WL6 corpus.

## 2026-09-27: WL1 Get Psyched resource correction

- Corrected the v1.4 shareware `GETPSYCHEDPIC` mapping from chunk 138 to
  chunk 146, matching both the installed archive's picture table and the
  `GFXV_APO.H` layout for the released Apogee data. Chunks 138 and 141 are
  both 24x32 status faces in this archive; chunk 146 is the final 224x48
  Get Psyched artwork.
- Added deterministic framebuffer checks for both WL1 and WL6 loading screens
  so a valid but semantically wrong graphics chunk can no longer pass the
  real-data tests merely because it decodes successfully.

## 2026-09-27: WL1 fixed-point and demo-parity pass

- Replaced the remaining generalized 16.16 multiplies in original gameplay,
  raycasting, projection, and positional-audio paths with a readable C form of
  `WL_DRAW.C`'s signed-magnitude `FixedByFrac`. Perspective multiplication now
  also reproduces Borland's 32-bit `long` wrap and the original
  `centerx = viewwidth / 2 - 1` convention.
- Restored the original combined `actorat` model. Low values are wall/door
  tokens and higher values identify actors; patrol destination marking, stale
  dead-actor marks, non-marking objects, player collision, actor pathing,
  pushwalls, and doors now observe that one shared occupancy grid.
- Corrected `DoorClosing` to use its narrower original obstruction test. The
  earlier port repeated `CloseDoor`'s adjacent-overlap check every tic, which
  reopened a door brushed by a guard, kept areas connected, and eventually
  desynchronized WL1 demo 1's actor and RNG state.
- Restored render-side simulation effects during headless demo playback:
  visibility activation, reachable bonus collection, and status-face RNG now
  occur once per recorded four-tic command in their original order.
- Bumped portable saves to version 3 to retain `actorat` exactly. Versions 1
  and 2 remain readable and reconstruct walls, doors, and live actor marks
  where their older format lacks stale occupancy information.
- Added focused regressions for the complete RNG table, signed-magnitude fixed
  math, stale visibility, blocking-static path selection, combined occupancy,
  and the closing-door threshold case. The complete WL1 matrix passes 51/51,
  including all four full embedded demo streams.
- Re-baselined only the frames affected by restored DOS projection math, then
  verified the full WL6, SOD, SDM, SD1, SD2, and SD3 matrix. The complete
  configured suite passes 141/141, including every embedded WL1, WL6, SOD,
  and SDM demo from beginning to end.
- Chocolate Wolfenstein 3D was used as a traceable secondary oracle. Demo 1's
  scalar trace matches it for all 1,284 commands. The remaining Chocolate
  differences in demos 0, 2, and 3 occur at sprite projection/pickup or
  hitscan-edge thresholds where Chocolate uses modern `FixedMul`; the released
  DOS source specifies the `FixedByFrac` behavior retained here.

## 2026-09-27: three-way source-parity call-graph audit

Added a tolerant static analyzer for the original Borland-era sources,
Chocolate Wolfenstein 3D, and wolf3dgeneric. It evaluates the original as four
separate build profiles, includes `statetype` callback and transition edges,
tracks production reachability from the generic public API, emits Markdown,
JSON, and Graphviz DOT output, and keeps reviewed mappings/classifications in a
versioned override file. Chocolate is corroborating evidence only; the DOS
source remains authoritative.

The first state-machine review found and restored three omitted behavior
chains:

- Wolf3D's exit tile now spawns BJ and runs the original six-tile run,
  four-frame jump, `YEAHSND`, and 300-tic completion sequence. Previously the
  exit only set `victory_flag`, leaving no mechanism to finish the level.
- Pac-Man ghosts now execute their original alternating two-frame chase and
  movement logic. Ghost and Spear Spectre contact once again applies the
  original `tics * 2` damage before the actor backs away from the player.
- Mecha-Hitler's alternating chase footsteps and Hitler's mid-death slurpie
  action are again dispatched at their original state transitions.

Each restored chain has direct unit coverage. Reviewed structural translations
(for example, behavior moved from renderer side effects into the simulation
tick) are recorded with reasons in `tools/callgraph-overrides.json` so the audit
does not repeatedly flag them as omissions.

## 2026-09-27: level-completed BJ breathing restored

The original `BJ_Breathe` alternates `L_GUYPIC` and `L_GUY2PIC` while the
level-completed screen waits for acknowledgement. The generic session had
rendered only the first image and frozen all intermission timing. The wrapper
now advances this presentation-only animation at the original 70 Hz timing:
the first change follows the original 10-tic threshold, with later changes
following the 35-tic threshold. Gameplay simulation remains stopped.

Graphical captures are generated from the user's external Wolfenstein 3D data
and remain under the Git-ignored `build/artifacts/` directory. They are never distributed
with the source repository. Each entry records a deterministic indexed-frame
hash so the milestone remains verifiable even when the screenshot is absent.

## 2026-09-26: Fractional pushwall intersections

- Restored the `WL_DR_A.ASM` pushwall intersection step omitted by the initial
  portable raycaster: each ray now advances along its tangent by the wall's
  fractional movement and continues tracing if that point leaves the tile.
- This restores the moving block's side geometry and removes the flat vertical
  squeeze seen while opening E1M1's first secret. The corrected halfway frame
  hashes to `deb2c8e3a4c04570`; its synthetic ray geometry hashes to
  `f2347a1bd90b06f4` with 95 rays striking the translated front plane.

## 2026-09-26: Opt-in mouse control

- Made mouse hardware absent unless `--mouse` is present, matching the original
  input manager's distinction between undetected hardware and disabled use.
- Without the switch, SIGNON leaves Mouse unmarked, mouse events are ignored,
  and mouse-specific Controls and Customize Controls entries are inactive.
- Corrected the WL1 menu-art chunk mapping after its extra Spear advertisement
  shifts the remaining menu graphics by one chunk. Control and Sound menus now
  use the intended radio boxes and title art instead of weapon/disk graphics.
- The corrected WL1 Sound menu hashes to `bd51c51cceb48736`; the no-mouse
  Control menu hashes to `59ee44f4e7ffa03d`. WL6 SIGNON hashes to
  `7f78b83ef22e6fbc` without mouse hardware and `f83e3e048c4550f4` with
  `--mouse`.

## 2026-09-26: Win32 4:3 VGA presentation

- Kept the generic library's canonical output at its original 320x200 indexed
  resolution.
- Changed only the Win32 host to present that framebuffer in a centered 4:3
  viewport, matching the non-square pixel geometry of a contemporary VGA CRT.
- Made the default client area 960x720 and preserve 4:3 with black
  letterbox/pillarbox bars when the window is resized.

## 2026-09-26: Original startup presentation restored

- Restored the original startup order for interactive hosts: hardware SIGNON,
  seven-second PC-13 rating screen, then the title and attract loop.
- Restored `FinishSignon`'s bottom strip behavior for Wolf3D: erase the dark
  embedded placeholder, center yellow `Press a key`, and replace it with green
  `Working...` after acknowledgement while preserving the version text at the
  lower right. Spear retains its original automatic three-second SIGNON hold.
- Embedded all five original SIGNON templates in the generic engine. WL1,
  WL6, and Spear automatically select Apogee, GT, and Spear respectively;
  `--signon apogee|gt|id|activision|spear` remains a runtime override.
- Ported the original `IntroScreen` overlay coordinates and Wolf/Spear color
  ramps. MAIN, EMS, and XMS display their maximum values; mouse and Sound
  Blaster are marked, joystick follows actual detection, and the excluded
  Disney Sound Source remains unmarked.
- Startup input now advances SIGNON or PC-13 instead of jumping directly to
  the control panel.
- The PC-13 indexed framebuffer is identical across all four base archives
  (`f49d01316c7fdd4c`); palette hashes distinguish Wolf3D
  (`1e14f48394b7e6fd`) from Spear (`eba126e8df00d1cb`).

![Restored original PC-13 startup screen](../build/artifacts/pc13-wl6.png)

![Embedded GT SIGNON with restored detection overlays](../build/artifacts/signon-gt-filled.png)

![Embedded Spear SIGNON with restored detection overlays](../build/artifacts/signon-spear-filled.png)

- Added `--adlib` as a hardware-profile override: OPL music and AdLib effects
  remain active, digitized Sound Blaster effects are disabled, and SIGNON uses
  the original mutually exclusive AdLib marker instead of Sound Blaster.

![AdLib-only SIGNON hardware marker](../build/artifacts/signon-gt-adlib.png)

## 2026-09-26: Original status faces and Spear F1 behavior

- Restored the original animated status-face timing instead of leaving BJ on
  the first frame permanently. Face timing consumes the table-driven game RNG
  in the original player-update position, including the demo-specific gatling
  hold interval.
- Corrected the zero-health portrait to use `FACE8APIC`; the earlier portable
  renderer incorrectly selected the final wounded row.
- Added archive-independent face states for the gatling pickup and Spear's
  `BJOUCHPIC` and two 30-second idle portraits. Both full Spear and SDM map
  those semantic states onto their different graphics chunk layouts.
- Restored Spear's historical F1 behavior. Its `BossKey` body was guarded by
  `NOTYET` in the released source, so F1 returns to play rather than opening
  Wolf3D's `Read This` article.
- Added data-backed coverage that decodes every special status portrait from
  both the SOD and SDM graphics archives.
- Corrected the rare `DEATHSCREAM6SND` easter-egg condition: Wolf3D uses each
  episode's internal map 9, while Spear uses internal maps 18 and 19. The
  earlier shared modulo check incorrectly applied Wolf's rule to Spear.
- Kept all eight ordinary guard death screams enabled for SDM, as in the
  original `SPEARDEMO` build; only the Wolf3D shareware (`UPLOAD`) profile
  limits that selection to its first two sounds and omits the rare scream.
- Added full-command playback regressions for all four SOD attract demos and
  the single SDM demo, complementing the existing early-frame visual checks.

## 2026-09-23: Initial E1M1 walls and doors

- Commit: `a3f6c09`
- Scope: fixed-point ray traversal, original wall textures, closed door plane,
  ceiling/floor colors; no actors, first-person weapon, or status bar.
- 320x200 indexed-frame FNV-1a: `52a9cf2dd9dcab66`
- Identical result with the supplied WL1 and WL6 data sets.

![Initial E1M1 wall and door view](../build/artifacts/initial-play-view.png)

## 2026-09-23: Original static status bar

- Scope: `STATUSBARPIC`, initial BJ face, floor, score, lives, health, ammo,
  empty key slots, and pistol icon, ported into `WL_AGENT.c`.
- 320x200 indexed-frame FNV-1a: `b2f43cae26b556f5`
- Identical result with the supplied WL1 and WL6 data sets.

![Initial E1M1 view with status bar](../build/artifacts/initial-hud-view.png)

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

![Initial E1M1 view with pistol and status bar](../build/artifacts/initial-weapon-view.png)

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

![E1M1 open-door static-object rendering](../build/artifacts/open-door-static-objects.png)

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

![E1M1 standing guard rendering](../build/artifacts/standing-guard.png)

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

![E1M1 original dead guard and locked door](../build/artifacts/initial-standard-actors.png)

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

![Hans Grosse on the original Episode 1 boss map](../build/artifacts/hans-grosse.png)

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

![E1M1 guard following his original patrol path](../build/artifacts/moving-patrol.png)

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

![E1M1 guard after first sighting](../build/artifacts/alerted-guard.png)

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

![E1M1 guard beginning his chase](../build/artifacts/chasing-guard.png)

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

![E1M1 guard firing and damaging the player](../build/artifacts/guard-firing.png)

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

![E1M1 dog landing a bite](../build/artifacts/dog-bite.png)

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

![Hans Grosse firing his original burst](../build/artifacts/hans-firing.png)

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

![Dr. Schabbs throwing a syringe](../build/artifacts/schabbs-needle.png)

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

![Giftmacher firing a rocket](../build/artifacts/gift-rocket.png)

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

![Fake Hitler's flame burst](../build/artifacts/fake-flames.png)

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

![E1M1 portrait pushwall halfway into motion](../build/artifacts/moving-pushwall.png)

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

![E1M1 guard in the third death frame](../build/artifacts/guard-death.png)

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

![Hans Grosse in the third death frame](../build/artifacts/hans-death.png)

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

![BJ firing the pistol at an E1M1 guard](../build/artifacts/player-pistol.png)

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

![E1M1 treasure room after collecting a cross](../build/artifacts/pickup-cross.png)

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

![E1M1 door halfway open after use](../build/artifacts/door-use.png)

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

![E1M1 after 35 forward play-loop tics](../build/artifacts/play-loop-forward.png)

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

![Ten-second E1M1 Nuked-OPL3 reference render](../build/artifacts/e1m1-nuked-opl3.wav)

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

![E1M1 music with the original AdLib pistol effect](../build/artifacts/e1m1-pistol-opl.wav)

Regenerate it with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --map 0 --sound 24 --dump-music out\e1m1-pistol-opl.wav
```

## 2026-09-24: VSWAP digitized sound effects

- Ported the original terminal VSWAP sound-info table and page-spanning sample
  loading into `ID_SD.c`, with checked offsets and lengths instead of the DOS
  page manager's far pointers.
- Restored the original `wolfdigimap` mapping and AdLib-derived priority for all
  46 mapped effects. Live playback selects the Sound Blaster sample when it is
  present and falls back to the matching AdLib effect when it is not.
- Mixed unsigned 8-bit samples into the existing signed stereo stream using the
  original Sound Blaster Pro 0-15 attenuation scale and priority replacement.
- Resampling uses a deterministic integer phase accumulator and zero-order hold
  at 7,042 Hz, matching the effective rate of the original integer Sound Blaster
  DSP time constant (`256 - 1000000 / 7000`).
- The registered VSWAP supplies all 46 entries. The supplied shareware VSWAP
  retains the 46-entry table but has 20 physically loadable samples; this sparse
  layout is explicitly tested and its missing entries follow the AdLib fallback.
- One second of the centered pistol sample at 48 kHz has PCM FNV-1a
  `44dc84b3f78798a3`, identical for supplied WL1/WL6 data and MSVC x86/x64
  builds.
- The ten-second E1M1-plus-pistol WAV has SHA-256
  `61376fcbeb1199d909ef084cbaa6ba4a3db4a4e5d0609758f21fc7f4083f86b5`.

![E1M1 music with the original digitized pistol sample](../build/artifacts/e1m1-pistol-digital.wav)

Regenerate the ten-second reference render with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --map 0 --sound 24 --digitized --dump-music out\e1m1-pistol-digital.wav
```

## 2026-09-24: Spatial and gameplay sound dispatch

- Replaced the scalar sound-number queue with bounded events that explicitly
  distinguish centered sounds from fixed-point world locations.
- Ported `SetSoundLoc` from `WL_GAME.C`, including the focal-point offset,
  fixed-point listener transform, clamping, and exact 15-by-30 attenuation
  table. The right channel uses the original mirrored axis.
- Active digitized sounds are repositioned every 70 Hz game tic as the player
  moves and turns, preserving `UpdateSoundLoc` behavior without coupling audio
  sample generation to the gameplay clock.
- Door motion, ordinary enemy sighting, hitscan fire, dog attacks, Schabbs'
  syringe, Giftmacher/Fatface rockets, Fake Hitler flames, and rocket impacts
  now emit their original positioned sound numbers.
- Boss sight lines remain centered as in the DOS source. Item pickups and both
  shareware/full guard-death sound selection now emit their original effects;
  all other supported actor classes have their original death calls as well.
- The headless WAV diagnostic accepts `--left-position` and
  `--right-position` to isolate and reproduce mixer panning.
- The right-biased ten-second pistol fixture has SHA-256
  `54aeae4d8a7e70efee58d2915e8236c67979301fd005a14d0bbea34bd0213a59`.

![Right-biased digitized pistol spatial-audio fixture](../build/artifacts/e1m1-pistol-panned.wav)

Regenerate it with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --map 0 --sound 24 --digitized --left-position 12 --right-position 0 --dump-music out\e1m1-pistol-panned.wav
```

## 2026-09-24: Floor progression and death restart state

- Added the campaign state boundary that the original `GameLoop` maintained
  outside each `SetupGameLevel`: score, extra-life threshold, health, ammo,
  lives, and selected/owned weapons now survive normal floor loads.
- Normal exits advance one floor, secret exits select floor 10, and completing
  floor 10 uses the original six-entry `ElevatorBackTo` table.
- Keys clear between floors. Death restores the score at floor entry, full
  health, the pistol and eight rounds, decrements one life, and retains the
  already advanced extra-life threshold exactly as the DOS game state did.
- Dead-player simulation is frozen for a deterministic 70-tic pause before the
  restart; enemies no longer continue acting while the death sound completes.
  The death camera/red fizzle and intermission UI remain explicit follow-up
  work rather than being approximated here.
- Added identical WL1/WL6 and x86/x64 regression coverage for Episode 1,
  Floor 2. Its initial 320x200 indexed-frame FNV-1a is `dd20149a5592b546`.

![Episode 1 Floor 2 after campaign transition](../build/artifacts/e1f2-start.png)

Regenerate the destination frame with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --map 1 --play-view --frame-hash --dump-frame out\e1f2-start.ppm
```

## 2026-09-24: Relative mouse input

- Added Win32 raw-mouse packets to the existing platform event boundary, so
  gameplay receives device deltas without cursor-edge clipping or desktop
  coordinate assumptions.
- Ported the original default `PollMouseMove` sensitivity and per-tic control
  clamp into `WL_PLAY.c`.
- Restored the DOS default mouse buttons: left attack, right strafe, middle use.
  Keyboard and mouse states combine through the same attack/use edge handling.
- Added focused deterministic tests for simultaneous mouse turn and forward
  movement; the complete 37-test WL1/WL6 suite remains stable on x86 and x64.

## 2026-09-24: Original Pause-key behavior

- Mapped the PC Pause key explicitly at the generic input boundary, preserving
  its original special status rather than treating it as a normal held key.
- Restored `CheckKeys` pause semantics: simulation and IMF sequence time freeze,
  the music channels key off without rewinding, and any subsequent key or mouse
  button acknowledges the pause and redraws the live game view.
- Draws the original 64x32 `PAUSEDPIC` at the original `(128,64)` coordinates.
  The supplied late Apogee WL1 graph places this at chunk 145 rather than either
  generated WL1 header retained in the source release; WL6 uses chunk 133.
- Both editions decode to the identical pause overlay framebuffer hash
  `ee855388f16e0af7` on an otherwise color-zero 320x200 frame.
- The complete E1M1 paused view is identical for WL1 and WL6, with indexed
  framebuffer FNV-1a `fb088c39d6f75570`.

![Original pause plaque over the live E1M1 view](../build/artifacts/paused.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --pause-view --frame-hash --dump-frame out\paused.ppm
```

## 2026-09-24: Level-complete statistics and intermission

- Added `WL_INTER.c` as the portable owner of the original `LevelCompleted`
  calculation and presentation, retaining the six-episode par table, 500-point
  per-second par bonus, three 10,000-point perfect bonuses, and 15,000-point
  secret-floor award.
- Map setup now records the original difficulty-filtered kill total, treasure
  total, and pushwall-secret total. The 70 Hz play loop owns floor `TimeCount`.
- Normal and secret exits stop simulation, switch to `ENDLEVEL_MUS`, display
  the live results and updated score, and wait for a key or mouse-button
  acknowledgement before loading the destination map.
- The supplied E1M1 contains 20 medium-difficulty kills, 5 secrets, and 23
  treasures. A deterministic 1:15 perfect result awards 37,500 points.
- The late Apogee WL1 archive places `L_GUYPIC` at chunk 55; the canonical WL6
  archive uses chunk 43. Both decode to the same completed screen, with indexed
  framebuffer FNV-1a `85b3dfdb33f1fb4d`.

![Perfect E1M1 level-complete result](../build/artifacts/e1m1-intermission.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --intermission-view --frame-hash --dump-frame out\e1m1-intermission.ppm
```

## 2026-09-24: Damage and bonus palette feedback

- Ported `InitRedShifts` and `UpdatePaletteShifts` behavior into `WL_PLAY.c`:
  six red damage stages, three yellow-white bonus stages, damage precedence,
  and original per-tic counter decay.
- Shift tables are calculated from the original 6-bit VGA palette before the
  generic boundary converts them to host RGB. Indexed framebuffer pixels do
  not change.
- Added `--palette-hash` to the headless host so palette-only behavior has an
  independent deterministic oracle. The 40-point damage fixture hashes to
  `d6969024db3bdb40`; the initial bonus fixture hashes to `8b16358ec3225130`.

![Original 40-point damage palette shift](../build/artifacts/damage-flash.png)

![Original pickup bonus palette shift](../build/artifacts/bonus-flash.png)

## 2026-09-24: Original player-death presentation

- Fatal dog bites, hitscan shots, and projectiles now retain the attacker's
  exact world position, matching the original `killerobj` boundary.
- Replaced the temporary fixed restart delay with the original `Died` flow:
  hide the weapon, rotate toward the attacker along the shortest arc at two
  degrees per 70 Hz tic, clear palette shifts, and fizzle the 320x160 play view
  to VGA color 4 over 70 frame batches.
- The post-fizzle hold accepts a key or mouse button after the fade, while the
  restart continues waiting for the player-death sound just as
  `SD_WaitSoundDone` did. The existing campaign restart then applies the life,
  score, inventory, health, and ammo rules.
- Added cardinal-angle, wraparound, equal-arc, and fatal-attacker tests plus a
  WL1/WL6 visual oracle. The half-fizzle indexed framebuffer is identical in
  both editions, with FNV-1a `110c844dbc2a3366`.

![Player death halfway through the original red fizzle](../build/artifacts/player-death.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --player-death-view --frame-hash --dump-frame out\player-death.ppm
```

## 2026-09-24: Episode victory totals

- Campaign map rebuilds now preserve the first eight ordinary-floor
  time/kill/secret/treasure records used by the original `LevelRatios` array.
- Ported the Wolf3D branch of `Victory()` into historical `WL_INTER.c`: total
  time, averaged ratios, the 99-minute cap, BJ victory portrait, and the
  medium-or-harder three-letter time verification code.
- A terminal boss victory now stops the live play renderer, draws the results,
  and switches music to `URAHERO_MUS`. End text and the later high-score/menu
  return remain in the front-end milestone.
- A deterministic eight-floor fixture uses 1:15 and perfect ratios on every
  floor. Both supplied WL1 and WL6 archives produce indexed framebuffer FNV-1a
  `8e828f1563f3c064` despite their different graphics chunk numbering.

![Original episode victory totals screen](../build/artifacts/victory.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --victory-view --frame-hash --dump-frame out\victory.ppm
```

## 2026-09-24: Original high-score table

- Ported the seven `ID_US_1.C` default entries and `CheckHighScore` insertion
  rule, including completed-floor tie-breaking and the empty name reserved for
  a qualifying player score.
- Ported the Wolf3D `DrawHighScores` branch: color-0x29 menu background,
  original stripe, picture headings, font-0 names, episode/floor notation, and
  characters 129-138 for fixed-width level and score digits.
- Exhausting the final life now enters this display and starts `ROSTER_MUS`.
  Editable name entry and persistent configuration storage remain later
  front-end work.
- The supplied WL1 and WL6 archives again resolve to identical pixels despite
  shifted chunk indices: default-table FNV-1a `7b063c0fb260132e`.

![Original Wolf3D high-score table](../build/artifacts/high-scores.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --high-score-view --frame-hash --dump-frame out\high-scores.ppm
```

## 2026-09-25: Editable high-score names

- Restored the original `ID_IN.C` unshifted and shifted ASCII lookup tables at
  the generic scan-code boundary, including Caps Lock's letter-only inversion.
- Qualifying scores now enter the `US_LineInput` editing state: left/right,
  Home/End, Backspace/Delete, in-place insertion, Shift, Caps Lock, Enter, and
  Escape all retain their original behavior.
- Name acceptance retains both original bounds: 57 stored characters and the
  pre-insertion 100-pixel font-width check. The I-bar uses font character 128
  at the measured cursor position.
- Added scan-to-ASCII unit coverage and an edition-independent `BJ` editing
  frame. WL1 and WL6 both hash to `3b75e429fe454ad8`.

![BJ high-score name with the original I-bar cursor](../build/artifacts/high-score-entry.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --high-score-entry-view --frame-hash --dump-frame out\high-score-entry.ppm
```

## 2026-09-25: Episode EndText articles

- Restored historical `WL_TEXT.c` as a bounded translation of the original
  article formatter. It recognizes all documented commands, proportional wrapping,
  picture-driven row margins, tabs, the four-piece window frame, and the
  `pg n of n` footer.
- The renderer reads the original embedded `T_ENDART1` through `T_ENDART6`
  chunks directly from `VGAGRAPH`. The supplied shareware archive exposes its
  single ending at shifted chunk 155; the full archive exposes all six at
  chunks 143-148.
- Window-piece lookup follows each edition's generated header rather than a
  uniform chunk shift: `H_TOPWINDOWPIC` is 17 in WL1 and 6 in WL6. This keeps
  the four border pictures aligned and prevents an unrelated chunk from being
  drawn across the footer.
- Acknowledging the victory totals now opens the correct episode article.
  Left/up and right/down/Enter navigate its pages; Escape leaves the article
  for score ranking and editable name entry.
- Data-backed tests render every page of every available ending. Their combined
  indexed hashes are `42a18636c4286987` for WL1's two pages and
  `7aeca6c61a08514b` for WL6's twelve pages. First-page headless
  fixtures both hash to `9751906604fce502`.

![Original Episode 1 ending article](../build/artifacts/end-text.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --end-text-view --frame-hash --dump-frame out\end-text.ppm
```

## 2026-09-25: Control-panel main menu

- Restored historical `WL_MENU.c` ownership for `DrawMainMenu`, including the
  original colors, stripe, beveled window, font-1 labels, inactive Save Game,
  highlighted Read This row, mouse legend, and gun cursor.
- Menu movement wraps in both directions and skips inactive entries. Title
  input now opens the menu; Escape returns to the title; New Game starts the
  current map; View Scores opens the original table; and completed game-over
  score entry returns to the menu instead of terminating the host.
- Chunk lookup follows the exact generated headers. Apogee 1.4 inserts
  `H_SPEARADPIC`, placing `C_OPTIONSPIC`, `C_CURSOR1PIC`, and
  `C_MOUSELBACKPIC` at 22, 23, and 30. WL6 uses 10, 11, and 18. Both editions
  consequently produce the identical indexed framebuffer hash
  `cdbff8b31548b64e`.

![Original Wolf3D control-panel main menu](../build/artifacts/main-menu.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --main-menu-view --frame-hash --dump-frame out\main-menu.ppm
```

## 2026-09-25: New Game episode and difficulty panels

- Ported `DrawNewEpisode`, `DrawNewGame`, and `DrawNewGameDiff` into the
  historical `WL_MENU.c` translation: original beveled windows, headings,
  two-line episode names, episode pictures, four difficulty labels, gun cursor,
  and selection-specific BJ portraits.
- The full data set enables all six episodes. Shareware retains the original
  locked color for Episodes 2-6 and refuses to advance those selections.
  Escape backs up one panel at a time.
- New games now start Floor 1 of the selected episode and pass the selected
  baby/easy/medium/hard value into level construction. Campaign reloads retain
  that difficulty rather than falling back to the legacy medium wrapper.
- Episode-screen indexed hashes are `23ad3114db625b9b` for shareware and
  `45053c7411aec10f` for the fully enabled WL6 screen. The medium difficulty
  screen is edition-independent at `6ef5da5dc52ae5b0`.

![Original shareware episode selector](../build/artifacts/episode-menu.png)

![Original medium difficulty selector](../build/artifacts/difficulty-menu.png)

Regenerate the captures with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --episode-menu-view --frame-hash --dump-frame out\episode-menu.ppm
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --difficulty-menu-view --frame-hash --dump-frame out\difficulty-menu.ppm
```

## 2026-09-25: Read This help article

- Connected the main menu's `Read This!` row to the portable `WL_TEXT.c`
  article renderer. Left/up and right/down/Enter/Space navigate, Escape and
  right mouse return to the control panel, and left mouse advances.
- The renderer now selects the original embedded `T_HELPART` chunk by exact
  edition: chunk 150 for Apogee shareware and chunk 138 for the full release.
- Data-backed tests decode and render all 41 pages in both archives. Their
  aggregate indexed hashes are `0555652391108503` for WL1 and
  `0c09b4fdb0b59e80` for WL6; the common first page hashes to
  `afa188f7e955ec42` in both editions.

![Original Wolf3D Read This help article](../build/artifacts/help.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --help-view --frame-hash --dump-frame out\help.ppm
```

## 2026-09-25: Embedded demo command playback

- Ported the original `PlayDemo` header and `PollControls` record format into
  `WL_PLAY.c`: map byte, 16-bit total length, skipped fourth header byte, button
  bitmap, and signed X/Y controls.
- Kept demo commands as their original four-tic frames. The shared player loop
  now accepts an explicit tic span internally, while normal gameplay remains a
  one-tic 70 Hz call with unchanged framebuffer regressions.
- Corpus tests decode all four streams in each supplied archive. WL1 contains
  3,740 commands across maps 0, 2, 4, and 6; WL6 contains 5,386 commands across
  maps 37, 43, 56, and 31. Combined command hashes are
  `4c9ba3762f6bb28c` and `8916c710d4bc58a2`, respectively.
- Added a 70-command headless replay checkpoint. WL1 hashes to
  `10458addd72fb2c0`; WL6 hashes to `8f5dc9097c041505`. Complete first-demo
  streams also run without parser, simulation, or renderer failure. Automatic
  attract-loop sequencing remains the next front-end integration step.

![Shareware embedded-demo checkpoint](../build/artifacts/demo-wl1.png)

![Full-release embedded-demo checkpoint](../build/artifacts/demo-wl6.png)

Regenerate the captures with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --demo-view --demo-commands 70 --frame-hash --dump-frame out\demo.ppm
```

## 2026-09-25: Live attract loop

- Restored `DemoLoop`'s original idle sequence: title for 15 seconds, credits
  for 10 seconds, high scores for 10 seconds, then one embedded demo. Completed
  demos return to the title and rotate through all four streams.
- Interactive demo sessions use hard difficulty and their authored map number,
  pump the original map music and sound events, and schedule one recorded frame
  per four 70 Hz tics. Palette counters now consume the same four-tic span.
- Any key or mouse-button press during the title, credits, scores, or demo opens
  the control panel. `Back to Demo` re-enters the attract sequence at the title.
- Added a separately owned front-end sequencer. The attract screens continuously
  play `NAZI_NOR_MUS`; the menu, help article, and score table select their
  original `WONDERIN_MUS`, `CORNER_MUS`, and `ROSTER_MUS` tracks. Map startup
  closes this owner before opening gameplay audio. One second of the shared
  WL1/WL6 attract track hashes to `cd1e371be8679285`.
- Added an edition-independent credits fixture with indexed framebuffer hash
  `877dd0b7c8d5bf6d`.

![Original Wolfenstein 3D credits screen](../build/artifacts/credits.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --credits-view --frame-hash --dump-frame out\credits.ppm
```

## 2026-09-25: Sound control panel

- Ported the original `SndMenu` presentation from `WL_MENU.C`: three outlined
  groups, authored title plaques, radio-button pictures, disabled-device color,
  mouse legend, and the wrapping gun cursor.
- Wired the implemented modes into live playback. AdLib effects use the
  Nuked-OPL3 voice, digitized effects use the original Sound Blaster sample
  bank, and music uses the IMF sequencer; every group also retains `None`.
  PC Speaker and Disney Sound Source remain disabled until implemented.
- Restored `ShootSnd` feedback when enabling a device or changing music mode.
  Music can be disabled without losing the front-end PCM owner needed for that
  preview, and gameplay still creates an effect-capable mixer with music off.
- Added edition-specific renderer fixtures: WL1 hashes to `ccbead18acb8b045`
  and WL6 hashes to `f311bc2002d7a216`.

![Original Wolfenstein 3D sound menu](../build/artifacts/sound-menu.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --sound-menu-view --frame-hash --dump-frame out\sound-menu.ppm
```

## 2026-09-25: Mouse control panel

- Restored `DrawCtlScreen` from `WL_MENU.C`, including the authored Control
  plaque, stripes, outlined device window, radio pictures, disabled joystick
  rows, mouse legend, and wrapping gun cursor.
- Mouse Enabled now gates relative motion and all three mouse buttons without
  changing keyboard input. Joystick and GamePad rows remain disabled because
  the generic platform contract currently has no joystick events.
- Restored the nested ten-position Mouse Sensitivity dialog. Arrow keys adjust
  the value, Enter or left mouse accepts it, and Escape or right mouse restores
  the prior value. The selected value now reaches `WL_PLAY.c`'s original mouse
  divisors rather than being a compile-time constant.
- Added control-menu hashes `6b0f54c32a663fa5` (WL1) and
  `466d386e2915c931` (WL6), plus the shared sensitivity-dialog hash
  `f7dc4f44ff382c6b`.

![Original Wolfenstein 3D control menu](../build/artifacts/control-menu.png)

![Original mouse sensitivity dialog](../build/artifacts/mouse-sensitivity.png)

Regenerate the captures with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --control-menu-view --frame-hash --dump-frame out\control-menu.ppm
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --mouse-sensitivity-view --frame-hash --dump-frame out\mouse-sensitivity.ppm
```

## 2026-09-25: Custom control bindings

- Restored the original Customize plaque and four-group table: Mouse,
  Joystick/Gravis GamePad, Keyboard actions, and keyboard movement. Authored
  labels, binding windows, disabled joystick values, cursor placement, and the
  mouse legend all come from the original layout.
- Added the original set-1 scan-code names and defaults. Keyboard action and
  movement fields can be selected horizontally and rebound to the next key;
  mouse Open, Fire, and Strafe fields can likewise be rebound to buttons 0–2.
  Reusing a mouse button clears its previous action as the DOS code did.
- Gameplay now resolves Run, Open, Fire, Strafe, four movement directions, and
  mouse buttons through those bindings rather than fixed host keys. Weapon
  number keys retain their original fixed behavior.
- Added renderer hashes `f94ac3381beb5e83` (WL1) and `2ac0b2270dc20b66`
  (WL6), plus scan-name and menu-navigation regression coverage.

![Original Wolfenstein 3D custom-control table](../build/artifacts/customize-controls.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --customize-controls-view --frame-hash --dump-frame out\customize-controls.ppm
```

## 2026-09-25: Change View and original play border

- Restored `CP_ChangeView`, `DrawChangeView`, and `DrawPlayBorder` behavior:
  arrows select sizes 4–19, Enter accepts, Escape restores the old value, and
  the panel shows the live beveled viewport preview above the three original
  instruction lines.
- Interactive games now begin at the original default size 15. Wall posts,
  static objects, actors, the player weapon, and the firing target window all
  use the chosen width instead of assuming a 320x160 view.
- Kept size 20 available to the headless fixture path so earlier full-width
  renderer checkpoints remain byte-for-byte stable. `--view-size N` selects a
  4–20 viewport for new captures.
- Added shared WL1/WL6 hashes `fe964b945ec6d203` for the Change View panel and
  `063d5289c4c1414e` for the authored default-size gameplay presentation.

![Original Change View panel at size 15](../build/artifacts/change-view.png)

![E1M1 using the original default size-15 viewport](../build/artifacts/default-view.png)

Regenerate the captures with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --change-view --frame-hash --dump-frame out\change-view.ppm
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --view-size 15 --frame-hash --dump-frame out\default-view.ppm
```

## 2026-09-25: In-game control panel and portable saves

- Restored the in-game `US_ControlPanel` path. Escape freezes the 70 Hz
  simulation, clears held input, pauses the existing IMF stream without losing
  its sequencer position, and opens the original panel with `Save Game` active
  and `Back to Game` selected. Returning redraws the same live map session.
- Restored `DrawLoadSaveScreen` and `PrintLSEntry`: the authored Load/Save
  plaques, ten font-0 slot boxes, empty labels, mouse legend, and gun cursor use
  the original coordinates for both supplied editions. Slot selection wraps,
  and save names accept original set-1 keyboard input.
- Added `WG_SAVE.c` as the intentionally new portability boundary. Unlike the
  DOS raw-structure/pointer dump, the format explicitly encodes fixed-width
  little-endian state, validates collection bounds and edition, and protects
  the payload with an FNV-1a checksum. Tests round-trip map arrays, player and
  campaign data, floor ratios, doors, statics, actors, pushwall state, and RNG;
  truncated, corrupt, and wrong-edition inputs are rejected. The nontrivial
  encoded fixture hashes to `edaa73019f926cd8` identically on x86 and x64.
- Added deterministic in-game-menu hash `9d4fae41d9d29488`; Load hashes are
  `a0188da801550518` (WL1) and `d81cecaa1eef0887` (WL6), while Save hashes are
  `d81cecaa1eef0887` (WL1) and `55f762913ddab4c7` (WL6). All 85 tests pass in
  MSVC x86 and x64 builds.

![Original in-game control panel](../build/artifacts/in-game-menu.png)

![Original Load Game panel](../build/artifacts/load-game.png)

![Original Save Game panel](../build/artifacts/save-game.png)

Regenerate the captures with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --in-game-menu-view --frame-hash --dump-frame out\in-game-menu.ppm
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --load-game-view --frame-hash --dump-frame out\load-game.ppm
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --save-game-view --frame-hash --dump-frame out\save-game.ppm
```

## 2026-09-25: Portable configuration and high scores

- Added `WG_CONFIG.c`, the portable counterpart to `WL_MAIN.C`'s raw DOS
  `CONFIG.WL1` / `CONFIG.WL6` structure dump. It explicitly encodes high
  scores, sound selections, mouse state and sensitivity, custom controls, and
  view size without serializing compiler padding or host-sized fields.
- The format is versioned, little-endian, edition-tagged, range-checked, and
  protected by an FNV-1a checksum. Missing or invalid configuration files fall
  back to the original defaults; interactive shutdown writes the current
  settings.
- A nontrivial serialization fixture hashes to `2ef00b526b4c06b1` identically
  in MSVC x86 and x64 builds. Regression coverage also rejects truncated,
  corrupted, and wrong-edition files.

## 2026-09-25: Original function keys and confirmation paths

- Restored the original F1–F10 in-game dispatch: Read This, Save, Load, Sound,
  Change View, Control, End Game, Quick Save, Quick Load, and Quit. The selected
  save slot becomes the quick slot after a successful save or load.
- Restored the original current-game, end-game, quick-load, and quit Y/N
  overlays. Gameplay tics stop while a gameplay confirmation is visible, while
  audio continues through its independent stream.
- Corrected the in-game control panel's historical dynamic row: `View Scores`
  becomes `End Game`, while `Back to Demo` becomes `Back to Game`. The corrected
  panel hashes to `9d4fae41d9d29488` in both editions; the shared confirmation
  fixture hashes to `0b2a93f76bdc8ce8`.

## 2026-09-25: Complete embedded-demo coverage

- Added `--demo-number 0..3` to the deterministic headless path and ran every
  recorded command in all four embedded streams from each supplied edition.
- The completed WL1 streams hash to `5b816ab21404372a`, `d0abd2087b49a774`,
  `0b21e54546425858`, and `3829f1bd054113c9`. The WL6 streams hash to
  `8b78f328d66287ab`, `4cc5578ebc5d9cfc`, `90df6b099486f7b1`, and
  `c32e04314c8181d4`.
- These eight full-stream gates complement the command-format/hash checks and
  the shorter first-demo visual checkpoint, covering 9,126 recorded commands
  in total across the two editions.

## 2026-09-25: Original PC-speaker sound mode

- Restored `SDL_PCPlaySound`/`SDL_PCService` semantics from `ID_SD.C`: six-byte
  `PCSound` headers, shared priority behavior, one pitch byte per 140 Hz service
  interval, the original `sample * 60` PIT divisor table, zero-byte silence,
  and PIT channel-2 square-wave synthesis at 1,193,182 Hz.
- Enabled PC Speaker in the original Sound menu and persisted the three-way
  None/PC/AdLib choice. Disney Sound Source remains unavailable because it is
  a separate digitized-output device rather than the game's PC-speaker effects
  mode.
- Every PC-sound chunk in both supplied editions now passes format/playback
  validation. A synthetic timing fixture hashes to `27841e7de4f37983`, and
  the real PC-speaker pistol mixed over E1M1 music hashes to
  `67e7ee416105629a` on both editions and architectures. The enabled menu hashes
  are `f46530fbaa7a0775` (WL1) and `bd51c51cceb48736` (WL6).

## 2026-09-25: Wolfenstein 3D v1.0 release

- Froze the completed first-release scope around the original Apogee shareware
  (`WL1`) and GT/ID/Activision full (`WL6`) v1.4 editions. Spear of Destiny,
  joystick input, and Disney Sound Source remain explicit post-1.0 targets.
- Published the generic host contract, exact supported-data hashes, changelog,
  and release checklist; added data-free GCC, Clang, and Clang sanitizer jobs.
- Reviewed the full tree for licensing, generated files, commercial data, stale
  implementation notes, and static-analyzer findings. No proprietary assets or
  generated runtime artifacts are tracked.
- Fresh strict-warning MSVC Release builds pass all 93 tests on both x64 and
  x86, including all four complete demo streams and both supplied data corpora.

## 2026-09-26: Portable joystick input

- Added `ID_IN.c` under its original source basename. Hosts report two
  normalized device slots, while the engine reproduces `IN_SetupJoy`'s central
  two-thirds dead zone and `INL_GetJoyDelta`'s signed `-127..127` outer-third
  scaling rather than assigning Wolf3D behavior to each platform backend.
- Restored Joystick Enabled, Use joystick port 2, and Gravis GamePad Enabled;
  the selected device contributes its original discrete movement alongside
  keyboard and mouse input. Two-button mode exposes buttons 0/1, while Gravis
  mode exposes all four original configurable action bindings.
- Joystick axes and buttons navigate menus, acknowledgements, articles, pause,
  and confirmation paths. The Win32 host dynamically discovers XInput without
  a new link-time dependency and maps the left stick/D-pad plus A/B/X/Y.
- Configuration version 3 persists joystick selection, mode, and bindings while
  continuing to decode version-2 files with original joystick defaults. Its
  nontrivial serialization fixture hashes to `7ae7b2e1e945a7a6`.
- The enabled control panel hashes to `6a7611870fce1ab5` (WL1) and
  `07dc46006dc97b99` (WL6). The project-specific GitLab CI file was removed.

![Original control panel with joystick enabled](../build/artifacts/joystick-menu-wl6.png)

Regenerate the capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --joystick-menu-view --frame-hash --dump-frame out\joystick-menu.ppm
```

## 2026-09-26: Spear runtime profiles and first visual milestones

- Added one runtime data-profile system for `WL1`, `WL6`, `SOD`, `SDM`,
  `SD1`, `SD2`, and `SD3`. `--game EXT` is authoritative; otherwise a
  `wolf*` executable name prefers Wolf3D data and `spear*`/`sod*` prefers
  Spear data, with unambiguous archive detection as the fallback.
- Kept a single binary and source tree. Mission-disk profiles use their
  `SD1`/`SD2`/`SD3` map and page extensions while retaining the original
  shared `.SOD` graphics and audio convention.
- Decoded the original two-part SOD title and VGA palette directly from the
  archives, added the nine-row Spear options menu, and restored Spear's direct
  New Game-to-difficulty flow (there is no Wolf episode selector).
- Added SOD status-bar/menu chunk mappings, the original 21-map music order,
  profile-specific audio bases, the four-sprite shift for ordinary enemies,
  the 25-round ammunition box, and Spear artifact collection state.
- The current strict MSVC test matrix contains 99 tests. SOD archive parsing,
  all 21 Mission 1 maps, title/palette hashes, main menu, difficulty menu,
  ordinary enemy shape transitions, and Spear-only items are now regression
  checked. Spear bosses, intermission/victory sequences, and semantic sound
  remapping remain the next fidelity tranche.

![Spear of Destiny title decoded from the original archives](../build/artifacts/sod-title.png)

![Spear of Destiny options menu](../build/artifacts/sod-main-menu.png)

![Spear of Destiny difficulty menu](../build/artifacts/sod-difficulty.png)

## 2026-09-26: Spear palette, SDM corpus, and selectable SIGNON screens

- Replaced every unconditional Wolf palette restoration with runtime game
  palette selection. Comparison against the original `GAMEPAL_*.OBJ` files
  identified Spear's two changed dark-green entries at indices 166 and 167;
  normal rendering and damage/bonus shifts now preserve them.
- Added the supplied two-map SDM release to the external regression corpus,
  including its 666 page archive, 125-picture graphics dictionary, distinct
  demo title, maps, menu, and exact archive hashes.
- Added optional `--signon FILE` startup display for original raw 320x200
  SIGNON screens. Publisher art is selected at runtime and remains external;
  `--signon-palette wolf|spear` can override filename/game-based selection.
- Regression fixtures now cover all five supplied SIGNON variants and their
  correct Wolf or Spear palette, alongside SOD and SDM menu palette hashes.

![Original Spear hardware-detection SIGNON screen](../build/artifacts/signon-spear.png)

## 2026-09-26: Original Spear actors and projectiles

- Restored the six `SPEAR`-conditional actor classes from the original
  `WL_ACT2.C`: Spectre, Angel of Death, Trans Grosse, Ubermutant, Wilhelm
  Strasse, and Death Knight. Their original map codes now replace the Wolf3D
  boss codes only when a Spear-family profile is active.
- Transcribed the original difficulty hit points, score awards, key drops,
  walk and attack frames, state durations, projectile actions, death frames,
  and chase speeds. The archive-wide SOD gate proves that all six classes are
  present and every initial sprite resolves inside the real page archive.
- Added Spear's separate rocket/smoke/explosion sprite ranges, Death Knight's
  angled heavy rockets, Angel sparks, Ubermutant's close-range bonus damage,
  Angel's repeated three-volley/rest cycle, and the Spectre fade/dormancy/wake
  cycle.
- Added a deterministic real-map Trans Grosse render at Mission 1 map 5. The
  indexed frame hashes to `dd1b3992157d8354` under Spear's corrected palette
  and original map-specific ceiling color.

![Trans Grosse decoded and rendered from the original SOD archives](../build/artifacts/sod-trans-boss.png)

## 2026-09-26: Spear campaign, intermission, and ending flow

- Restored Spear's 20-entry level-ratio history, its original par-time table,
  six boss/secret-floor completion messages, 14-floor victory averages, and
  secret routes (maps 4/12 to 19/20, returning to maps 5/13). Portable saves
  are now version 2 while version-1 eight-ratio saves remain readable.
- Artifact collection now transfers the campaign into map 21 at the exact
  pickup position and angle, retains the campaign state, and grants the gold
  key. Angel of Death's terminal state completes the campaign.
- Restored the four-frame BJ collapse with its original 140/105/105/210-tic
  holds, the Spear victory summary, all ten illustrated ending steps and their
  nine embedded palettes, the two original debrief captions, and the Spear
  high-score backdrop/layout. The ending-screen decoder directly reproduces
  `CA_CacheScreen`'s four VGA planes from the original archives.
- SDM now stops after completing its second and final map and proceeds to high
  scores, matching the `SPEARDEMO` branch rather than attempting map 3.
- The strict MSVC matrix now passes all 114 tests. New visual gates cover the
  boss intermission, collapse, victory summary, illustrated ending, SOD high
  scores, and SDM final intermission with their proper palettes.

![Spear boss-floor intermission](../build/artifacts/sod-trans-intermission.png)

![Spear victory summary](../build/artifacts/sod-victory.png)

![Spear illustrated ending debrief](../build/artifacts/sod-end-1.png)

![Spear high-score table](../build/artifacts/sod-high-scores.png)

## 2026-09-26: Semantic Spear sound mapping

- Replaced the accidental assumption that Wolf3D and Spear share every sound
  number with a runtime semantic mapping. Common effects retain their original
  numbers, while missiles, later guard death cries, the ammunition box, and
  every Spear boss voice/effect resolve to the corresponding `AUDIOSOD.H`
  entry.
- Restored Spectre sight/fade, Angel sight/fire/death/breathing, Trans sight
  and death, Wilhelm sight and death, Ubermutant death, Death Knight sight,
  death, and missile, artifact pickup, and Angel's intermediate slurpie cue.
- Added Spear's separate digitized-sound map, including SDM's reduced set and
  AdLib fallback. The headless WAV diagnostic now accepts the proper 0-80 SOD
  sound range, and archive tests validate all 81 PC/AdLib effect pairs, all 24
  full-game music tracks, all 40 full-game digitized samples, and the smaller
  SDM payload actually present on disk.

## 2026-09-26: Spear mission-pack corpus validation

- Added support for both historical mission-pack naming (mission-specific
  `.SD1`/`.SD2`/`.SD3` maps and pages plus shared `.SOD` graphics/audio) and
  the GOG layout, where each isolated mission directory renames every archive
  to `.SOD`. An explicit `--game SD1`, `SD2`, or `SD3` preserves campaign
  identity in either layout.
- Restored the original engine's tolerance for isolated or clustered ambush
  markers with no adjacent area tile. Return to Danger and Ultimate Challenge
  contain several such markers; the DOS setup code retained an unassigned
  area byte rather than rejecting the map.
- Added archive and headless gates for all three missions. Every one of the 63
  maps decodes and builds, all Spear actor classes and resources remain valid,
  and the two expansion VSWAP archives each expose 732 pages.
- Kept save/config identity separate from the physical archive suffix. A GOG
  M2 directory may contain `VSWAP.SOD`, but selecting `--game SD2` still uses
  `CONFIG.SD2` and `SAVEGAM?.SD2`, preventing cross-campaign collisions.
- Recorded exact hashes for the tested GOG archives. The Return to Danger and
  Ultimate Challenge opening frames hash to `952d61e2cbbc6545` and
  `4f0e225bd4412cbc`; both use the corrected Spear palette and original
  ceiling-color table.

![Return to Danger opening map](../build/artifacts/sd2-opening.png)

![Ultimate Challenge opening map](../build/artifacts/sd3-opening.png)

## 2026-09-26: Spear attract loop and SDM conclusion

- Replaced the remaining Wolf-specific attract resource assumptions with
  profile data. Full Spear uses embedded demo chunks 164-167, SDM uses its
  sole chunk 132, and the demo automatically repeats that stream as in the
  original `SPEARDEMO` build.
- Corrected the credits screen to chunk 92 for full Spear and chunk 78 for
  SDM. Both archive editions reproduce the same original credits frame at
  hash `9a2558b3b98c49d0` with the Spear palette.
- Restored SDM's original four-line purchase message after completing Floor 2.
  The first acknowledgement now opens that message over the completed-floor
  screen; the next proceeds to high scores. Its deterministic frame hashes to
  `e8186a081eb15e49`.
- Added real-archive gates for the full SOD and SDM demo streams, each hashing
  to `525cbfb8e59bee04` after 70 recorded commands.

![Spear embedded attract demo](../build/artifacts/sod-demo.png)

![Spear credits](../build/artifacts/sod-credits.png)

![SDM completion message](../build/artifacts/sdm-conclusion.png)

## 2026-09-26: Original Spear ceiling colors

- Restored the original `SPEAR`-conditional 21-entry `vgaCeiling` table instead
  of indexing Wolf3D's 60-map table. The renderer selects the table from the
  runtime game family, so the correction applies equally to SOD, SDM, and all
  three mission profiles without compile-time branches.
- Added a renderer-level assertion for Spear map 1's `0x6f` ceiling and updated
  the affected Trans Grosse and attract-demo visual hashes. UI-only screens
  remain byte-identical.

## 2026-09-26: Minimal Win32 release folders

- Added a `win32-release` build target that stages an architecture-labelled
  folder containing only `wolf3dgeneric.exe`, `Nuked-OPL3.dll`, the project and
  LGPL license texts, third-party provenance, and a runtime guide. Headless
  tools, tests, import libraries, symbols, and intermediate files remain in the
  build tree.
- Changed the Windows OPL boundary from static linkage to a replaceable DLL.
  The official vendored Nuked-OPL3 revision remains the reference and default;
  the Windows executable now imports its public emulator entry points instead
  of embedding them.
- Made interactive hosts use the executable's directory as the default data
  root when `--data` is absent. The Win32 host obtains the canonical module
  filename so this remains correct when launched through Explorer, a shortcut,
  a relative command, or `PATH`.
- Enabled the static MSVC runtime by default so a staged folder has no Visual
  C++ redistributable dependency. Dependency inspection leaves only Windows
  system DLLs plus the staged Nuked-OPL3 DLL.
- Rebuilt and passed all 130 tests with strict warnings on both MSVC x86 and
  x64 after introducing the DLL and adjacent-data paths.

## 2026-09-26: Shared generic engine boundary

- Changed the `wolf3dgeneric` CMake target from a static archive to the actual
  platform-neutral shared engine: `wolf3dgeneric.dll` on Windows and
  `libwolf3dgeneric.so` on ELF platforms.
- Replaced the core's implicit references to executable-defined `WG_*`
  functions with public, versioned `wg_platform_api_t` callback registration.
  A host now registers its video, input, timing, error, and PCM functions before
  engine creation, leaving no circular DLL/executable symbol dependency.
- Reduced the Windows DLL's public surface to six symbols: the four lifecycle
  functions and the engine-owned screen and palette pointers. The Win32 host's
  dependency table now names `wolf3dgeneric.dll`, which in turn names the
  independently replaceable `Nuked-OPL3.dll`.
- Added `library-release`, a host-free package containing the shared engine,
  public header, Windows import library where applicable, Nuked-OPL3 shared
  dependency, provenance, and licenses. The existing `win32-release` package
  now adds the engine DLL beside its thin executable.
- Kept the internal static test archive private to the build tree so the
  headless diagnostic executable and unit suite can exercise non-public engine
  modules without widening the shipped ABI. Both MSVC x86 and x64 continue to
  pass all 130 tests under strict warnings.

## 2026-09-26: Predictable repository workspace

- Added checked-in CMake configure, build, and test presets for Windows x86 and
  x64 development, Win32 releases, standalone library releases, and native
  Linux development/library builds.
- Consolidated every CMake tree beneath `build/<preset>`, moved all 305 local
  visual/audio diagnostics to `build/artifacts`, and verified all 68 milestone
  links still resolve locally. The empty `cmake` directory and seven historical
  root-level build directories were removed.
- Moved clean staged products out of compiler trees and into `dist/`, with
  separate architecture-labelled runtime and library folders. Both generated
  roots remain ignored by Git.
- Recreated all six Windows preset trees from scratch, staged all four x86/x64
  packages, and passed the complete 130-test suite in both development builds.

## 2026-09-26: Guard death sprite-range invariant

- Compared every WL1 guard death page with WL6 and the original `WL_ACT2.C`
  state chain. The intended sequence remains sprites 91, 92, 93, and 95;
  sprite 99 is the first dog walking frame.
- Made the renderer derive the original non-rotating property from death state
  as well as the cached rotation field. A stale rotation value can therefore
  no longer add four frames to the dead guard and enter the dog sprite range.
- Extended the actor test through all 45 individual death tics and added visual
  gates for the first and terminal frames. The WL1 checkpoints hash to
  `71b19599a4c9bc49` and `fb94e191f92153c3`, respectively.

## 2026-09-26: Original actor awareness in the live play loop

- Audited `PlayLoop`, `DoActor`, `T_Stand`, `T_Path`, `SightPlayer`,
  `CheckSight`, `CheckLine`, `FirstSighting`, and the door-area graph directly
  against the original sources. The portable routines were present, but the
  interactive actor loop had never called awareness from standing or path
  states; ordinary guards therefore noticed the player only when damaged.
- Restored awareness at the original state-thinker boundaries. Standing actors
  check every simulation tic; patrols check only on `T_Path` states, preserving
  the original `path1s`/`path3s` pauses and reaction countdown timing.
- Restored `madenoise` as a one-frame event. Gunfire and successful knife hits
  alert all non-ambush actors in player-connected areas with no distance
  attenuation. Ambush actors still require line of sight. Opening a door does
  not itself alert enemies, but its first movement step joins the adjacent
  areas so subsequent sight and noise can cross it.
- Restored `objtype.active` scheduling semantics: patrols and transient actors
  start active, rendering permanently activates an actor, and other actors are
  frozen while their area remains disconnected. Chase movement also stops
  during the victory sequence as in `T_Chase`.
- The newly exercised firing path exposed a stale rotation flag on attack and
  dog-jump states. Original state tables mark both as non-rotating; retaining
  the walking rotation could add a view-direction offset into the next sprite
  group, making a firing guard briefly appear as a dog. State transitions and
  renderer invariants now both enforce the original rotation property.
- Added focused tests for connected-area noise, closed and just-commanded door
  isolation, door sound not acting as an alert, reaction delays, inactive-area
  freezing, and attack/jump rotation. Refreshed all real-data demo hashes after
  the actors began responding during normal playback.

## 2026-09-27: Player, interaction, and area parity pass

- Extended the original/Chocolate/portable audit from actor state callbacks
  through `T_Player`, `T_Attack`, movement, use, doors, pushwalls, pickups,
  area initialization, and the live session scheduler.
- Restored the original `VictorySpin` view behavior during BJ's exit run: the
  player now turns toward 270 degrees and advances to the source-defined camera
  position while the BJ actor completes his animation. Damage is ignored once
  the victory flag is active, and attack animation no longer advances after an
  exit tile is crossed in the same player think.
- Restored original button-state semantics. Use is evaluated on every held
  frame, while doors and elevators retain their internal previous-frame guard;
  holding Use while approaching a pushwall therefore works again. New attack
  and use presses made during `T_Attack` are cleared exactly as in the DOS
  thinker, allowing a physically held attack to become eligible when the
  current animation finishes.
- Added the source-defined do-nothing and level-complete sounds, and restored
  the extra-life sound generated by `GiveExtraMan`, including score-threshold
  awards. Door sounds are now limited to player-connected areas.
- Initialized `area_by_player` with only the starting area, matching
  `SpawnPlayer` followed by `InitAreas`; open doors continue to rebuild the
  full graph on their first movement step.
- Corrected two session-level details found beside those call paths: BJ's face
  does not randomly animate during victory, and Spear's idle-face timer resets
  on attempted thrust (including movement blocked by a wall), not only when
  the player's coordinates actually change.
- Added deterministic regression coverage for victory camera movement,
  victory damage immunity, use/elevator sounds, held-use pushwalls, attack
  press carry-through, and extra-life audio.

## 2026-09-27: Transition, pickup, and HUD timing parity

- Restored the original elevator exit pause without introducing a blocking
  DOS-style service loop. Gameplay now freezes after the switch is activated,
  lets the level-done effect finish through the portable audio stream, and
  enters the intermission only afterward.
- Made all sound-completion waits account for PC-speaker playback as well as
  AdLib and digitized effects. In particular, a PC-speaker death sound can no
  longer be cut short by the level reload.
- Added an offset/stride-aware fizzle primitive and used it for player death.
  The red death fizzle is once again confined to the selected 3D viewport;
  smaller view sizes preserve their original surrounding border.
- Restored `DrawScaleds` bonus acquisition. Pickups now use the original
  visibility and projection-distance test (less than one tile ahead and half
  a tile laterally) instead of waiting for the player's center to enter the
  object's tile.
- Reconnected the render-time chaingun pickup to the status face and replaced
  its fixed-duration approximation with the original sound-driven hold. The
  face remains fixed while the actual pickup sound is pending or playing and
  resumes its normal random sequence when that sound ends.
- Extended the call-graph mappings through the reviewed player movement,
  pickup, render, death, and game-loop translations, and added regression
  coverage for fizzling into a subregion of a larger framebuffer.

## 2026-09-27: Original level-entry presentation

- Restored `PreloadGraphics` as a non-blocking session phase. Ordinary new
  levels, loaded saves, and demos now display the edition-correct embedded
  `GETPSYCHEDPIC`, the original status bar and completed preload bar, followed
  by the source-defined one-second input-skippable hold.
- Preserved the original `GameLoop` exceptions: restarting after a death skips
  “Get Psyched!” but still fuzzles into the new life, while taking the Spear
  jumps directly to map 20 without either preload presentation or entry
  fizzle.
- Restored the first-view 20-frame fizzle as an offset-aware, non-blocking
  transition for every configured viewport size. Simulation remains frozen
  until the transition completes.
- Added real-data validation of the WL1, WL6, SDM, and SOD Get Psyched graphic
  mappings and recorded the scheduler-separated render/fizzle edges in the
  call-graph audit.

## 2026-09-27: Fast-host presentation pacing

- Added non-blocking minimum presentation intervals around the Wolf3D SIGNON
  sequence. The embedded black startup message now remains visible for 750
  milliseconds before the yellow “Press a key” prompt, and the green
  “Working...” response remains visible for another 750 milliseconds after
  acknowledgment. Spear retains its original three-second SIGNON hold.
- Recreated the visible `PM_Preload` progress on modern storage by filling the
  Get Psyched bar over one logical second before the original one-second,
  input-skippable completed-bar hold. The simulated work interval cannot be
  skipped, while its completed hold retains original input behavior.
- Kept both effects entirely in presentation state: neither changes gameplay
  tics, demo input timing, random-number consumption, or audio scheduling.

## 2026-09-27: Live-game random initialization parity

- Restored `SetupGameLevel`'s distinction between deterministic demos and
  time-varied live play. Demo construction starts the original lookup-table
  generator at index zero; ordinary sessions derive the original 0--99
  hundredths phase from the portable host clock.
- Made the selected index an explicit level-builder input and install it before
  actor scanning. This preserves the original ordering for randomized actor
  animation tics instead of trying to reseed after construction.
- Added boundary tests for the millisecond-to-hundredths conversion and an
  actor-spawn fixture proving that a nonzero seed affects construction. All
  complete embedded-demo gates retain their deterministic index-zero behavior.
- Regenerated the source-parity call-graph report. Its mapped-function totals
  are now 111/111/103/103 for WL1/WL6/SDM/SOD, with no path gaps or unmapped
  state callbacks.

## 2026-09-27: Independent-audit corrections

- Added coordinate validation before legacy version-1/2 portable saves rebuild
  the 64x64 `actor_at` occupancy grid, with a checksum-valid rejection fixture
  covering the previously unchecked index.
- Changed the Win32 event pump to return translated events before dispatching
  further host messages. Since one Windows message produces only a small event
  group, host backlogs can no longer fill the ring and discard digital release
  transitions.
- Added ASCII case-insensitive read fallback for the generated DOS data
  filenames on case-sensitive filesystems, plus a portable file-I/O regression
  test. Exact-case opens remain the fast path, and write paths are unchanged.
- Made the file-size overflow guard conditional on data models where `long`
  can actually exceed `size_t`, retaining the check without MinGW's
  always-false strict-warning diagnostic.

## 2026-09-27: Intermission timing and acknowledgment parity

- Replaced the portable intermission's immediate final-value screen with a
  non-blocking translation of the original `LevelCompleted` sequence. The
  time bonus and all three completion ratios now count up in order at the
  70 Hz presentation rate, including the original 30-tic pauses and perfect,
  zero, and ordinary completion sound cues.
- Bonus points are now awarded only after the count finishes or is skipped.
  During the count, the bonus field advances while the status-bar score stays
  unchanged; the score updates only when the completed bonus is awarded.
- Restored the original two-stage acknowledgment: the first press during the
  sequence fills in the final values, while a subsequent press advances to
  the next map. Secret and boss-floor summaries retain their immediate bonus
  and single acknowledgment.
- Kept BJ's breathing animation active while counting and while sound effects
  finish, and added real-data rendering coverage for distinct initial and
  completed intermission frames.

## 2026-09-27: High-score transition parity

- Restored the original `CheckHighScore` timeout for a score that does not
  enter the table: the roster remains visible for up to 500 tics and returns
  early on input, instead of waiting forever for a key.
- Reset the line editor's cursor and Caps Lock state for each new entry and
  restored Spear of Destiny's wider 130-pixel name limit; Wolfenstein keeps
  its original 100-pixel limit.
- Corrected the menu's Spear high-score music to use `XAWARD_MUS` (index 20)
  rather than Wolfenstein's `ROSTER_MUS` index; the post-game and menu paths
  now select the same edition-appropriate track.
- Added the high-score transition to the reviewed original-to-portable call
  graph mappings.

## 2026-09-27: Variable-tic and deterministic-math parity

- Restored the original live `CalcTics` contract. Normal gameplay now samples
  controls once, advances the complete play loop once with an elapsed value
  from 1 through `MAXTICS` (10), and renders once. A delayed host frame drops
  time beyond ten tics as the DOS code did; demos remain one authored four-tic
  command per frame and presentation-only sequences retain one-tic service.
- Exercised every legal live `tics` value against time accounting, movement,
  and fractional turning. The complete WL1, WL6, SOD, and SDM demo and visual
  suites pass in both 32-bit and 64-bit builds.
- Replaced runtime `sin`, `tan`, `atan`, and `atan2` calls with frozen verified
  tables and integer lookup. Full renderer-table hashes cover every legal view
  width; Q32 unit-circle comparisons reproduce the original single-precision
  projectile and death-camera angle conversion.
- A temporary exhaustive audit compared 1,050,624 nonzero coordinate pairs
  from -512 through 512 against the original float expression with zero
  mismatches. Only compact boundary cases remain in the permanent test suite.
- Reviewed the variable-tic multiplication paths under the restored 1--10
  bound and verified matching results on x86 and x64. Gameplay coordinates,
  fixed-point products, tic counters, and serialized values retain explicit
  fixed-width types; no gameplay or renderer floating-point expression remains.

## 2026-09-27: Native build portability

- Added a source-owned Visual Studio solution with Win32/x64 and Debug/Release
  configurations. The IDE delegates to the authoritative CMake graph and
  shares the established `build/windows-dev-*` trees with command-line builds.
- Added explicit dynamic-CRT Visual Studio configurations while retaining `/MT`
  as the redistributable-free default. Both modes keep `wolf3dgeneric.dll` and
  `Nuked-OPL3.dll` independently replaceable; dependency inspection confirms
  only `/MD` builds import the Visual C++ runtime DLLs.
- Added a root GNU Make workflow with compiler-qualified `build/linux-gcc` and
  `build/linux-clang` trees. GCC 14.2 and Clang 19.1 strict-warning builds each
  pass all 147 tests against the complete supported-data corpus.
- Staged and inspected the Linux library package: it contains only the engine,
  replaceable Nuked-OPL3 library, public header, and notices, and exports only
  the documented six-symbol ABI.
- Ran the full suite under Clang AddressSanitizer and UndefinedBehaviorSanitizer.
  The audit found and corrected one signed-left-shift in digitized-sample
  conversion; all 147 sanitizer tests now pass.
## 2026-09-28: Menu cursor and death-transition parity

- Restored `HandleMenu`'s `C_CURSOR1PIC`/`C_CURSOR2PIC` animation for the main,
  load/save, sound, control, customize, episode, and difficulty menus. The
  portable host follows the original 9-tic flash and 71-tic hold cadence,
  retains that cadence across selection moves, and redraws the current frame
  after a menu refresh.
- Traced the restart path through the original `Died`, `GameLoop`, and
  `ThreeDRefresh` functions. The DOS game fills an off-screen view with color
  index 4, fizzles it onto the displayed page over 70 tics, omits
  `PreloadGraphics` after a death, and then fizzles the new view onto that
  still-red page over 20 tics.
- Corrected the portable restart path to clear both preload pacing counters.
  This removes the orphaned Get Psyched progress bar that could appear on a
  black screen after death. Entry fizzle setup now retains a red view region
  specifically for death restarts while ordinary level entry remains black.
- Added real-resource coverage proving that the second cursor frame is
  present and visually distinct. The complete 150-test corpus passes in both
  x64 and x86 MSVC builds.

## 2026-09-29: Original menu sound feedback

- Verified the menu path against the original `HandleMenu`, `DrawHalfStep`,
  `DrawGun`, `ShootSnd`, mouse-sensitivity, and customization routines.
- Restored `MOVEGUN1SND` followed eight 70 Hz tics later by `MOVEGUN2SND` for
  ordinary menu movement. The delay is scheduled rather than busy-waited, so
  host input, presentation, and audio pumping remain responsive.
- Corrected menu confirmations to use `SHOOTSND` rather than the unrelated
  in-game pistol-attack sound and applied the feedback across keyboard,
  controller-derived keyboard events, and mouse activation paths.
- Restored `ESCPRESSEDSND` on the original backward-navigation paths,
  including load/save prompts, Change View, control customization, mouse
  sensitivity, and the main menu hierarchy; secondary mouse-button exits use
  the same feedback.

## 2026-10-01: Linux release workflow and SDL compatibility

- Added self-documenting GNU Make help, explicit distribution aliases, and
  independent compiler-qualified build trees for the library, console, and
  SDL3 packages. Plain `make` now stages the dependency-light library package.
- Removed the direct-console dependency from ordinary core development and
  library configuration. DRM/ALSA packages are now required only when the
  console release is actually requested.
- Accepted system SDL releases from the first stable SDL3 ABI series (3.2 or
  newer), while retaining pinned SDL 3.4.16 as the reproducible package
  default. System-SDL packaging no longer requires an initialized submodule
  merely to obtain its license notice.
- Added `make dependencies` to fetch the recorded SDL submodule revision.
  Pinned builds now stop before configuration with exact recovery instructions
  when SDL is absent, while system-SDL builds remain submodule-independent.
- Documented that the Linux SDL package bundles SDL and uses `$ORIGIN`, but
  remains tied to its CPU, ELF/libc build baseline, and ordinary Linux desktop
  facilities rather than promising a universal static binary. Debian 13
  validation also showed why the pinned build remains the redistributable
  default: its SDL has only libc/libm direct dependencies, whereas Debian's
  system SDL retains its distribution X11, Wayland, audio, and graphics
  runtime dependencies.
- Measured the Debian 13 pinned-package symbol floors independently: SDL host
  `GLIBC_2.34`, engine `GLIBC_2.14`, Nuked-OPL3 `GLIBC_2.2.5`, and SDL itself
  `GLIBC_2.38`. Documented older-container builds as the correct way to lower
  that floor rather than attempting to bundle or override glibc.
- Added and validated a digest-pinned Debian 10 portable-release container.
  It uses GCC 8.3 and glibc 2.28 while supplying checksum-pinned CMake 3.31.6
  only as a build tool. The staged package runs on Debian 10 and Debian 13;
  its highest actual symbol requirement is SDL's `GLIBC_2.27`, enforced by an
  automatic post-stage audit capped at the container's `GLIBC_2.28` baseline.
- Supplemented Debian 10's Wayland 1.16 development files with a
  checksum-pinned Wayland 1.18 scanner/header build. Confirmed that the
  compatibility SDL now includes dynamically loaded native Wayland, X11,
  KMS/DRM, ALSA, and PulseAudio backends without adding a Wayland library to
  the staged package or raising its `GLIBC_2.27` symbol floor.
- Forced `SDL_VIDEODRIVER=wayland` and ran the packaged WL1 game for eight
  seconds against a disposable headless Weston compositor on Debian 13. The
  runtime test remained live until its intentional timeout, proving that the
  native Wayland path initializes without silently falling back to X11.
- Extended `make clean` with project-scoped portable cleanup. It removes the
  container-generated portable CMake trees and named Debian 10 builder image,
  is harmless when Docker or those artifacts are absent, and refuses unsafe
  portable-tree paths rather than applying a broad Docker cache prune.
- Extended the Debian 10 builder to stage and ABI-audit the shared-library and
  direct-console distributions as well as SDL3. Added a `portable` aggregate,
  `portable-{library,console,sdl3}` aliases, and matching explicit release
  names. Shortened the public native Make target to `console-release` while
  preserving `linux-console-release` as a compatibility alias and retaining
  the explicit Linux platform name in the staged artifact. Full container
  builds measured package floors of `GLIBC_2.14`, `GLIBC_2.17`, and
  `GLIBC_2.27`, respectively, and packaged console/SDL help smoke tests passed.
- Re-ran the complete 149-test asset-backed suite successfully with both GCC
  and Clang on the Debian build host after the workflow changes.

## 2026-10-01: Authoritative 1.4 revision sequence

- Added a root `VERSION` file and made CMake, GNU Make, generated package
  metadata, and distribution paths consume it as their single version source.
- Assigned revisions 1.4.1 through 1.4.16 retrospectively to the sixteen dated
  milestones after the original 1.4.0 promotion, then established 1.4.17 as
  the first strictly incremented main-branch revision.
- Added configure-time and `make check-version` validation for the required
  `1.4.REVISION` form and matching changelog entry.
- Validated 1.4.17 with all 150 configured Windows tests, all 149 Linux tests
  under both GCC and Clang, and a Debian 10 library package whose ELF symbols
  require no newer than `GLIBC_2.14`.

## 2026-10-01: Public host boundary

- Removed the private `src/` include path from the shared production-host
  configuration and changed the Win32 and direct-console wrappers to include
  only the public `WOLF3DGENERIC.h` interface.
- Retained explicit private-header access for the headless validation host and
  asset-backed test executable, whose purpose is to exercise internal engine
  behavior rather than serve as an example library consumer.
- Inspected generated compiler flags on both platforms: the Win32 wrapper sees
  only `include/`, the Linux console wrapper sees `include/` plus libdrm, and
  SDL3 sees `include/` plus SDL's public/generated headers. None of the three
  production wrappers can include the engine's private `src/` headers.
- Rebuilt the Win32, SDL3, console, and headless hosts successfully. All 150
  configured Windows tests pass, as do all 149 Linux tests under both GCC and
  Clang.

## 2026-10-01: Wolf3D public library identity

- Renamed the public engine boundary from `WOLF3DGENERIC.h` and
  `wolf3dgeneric_*`/`wg_*` symbols to `WOLF3D.h` and a consistent
  `wolf3d_*`/`WOLF3D_*` ABI. The original-source `WL_*` and `ID_*` names and
  portable implementation `WG_*` names remain unchanged.
- Renamed the shared artifact and import library to `wolf3d`, exposed the
  namespaced in-tree target `wolf3d::wolf3d`, and changed every production
  wrapper to link that target exclusively.
- Added ELF ABI versioning. Linux builds now emit `libwolf3d.so` pointing to
  SONAME `libwolf3d.so.1`, which points to the revisioned
  `libwolf3d.so.1.4.19`; all three names are staged in every applicable
  library and wrapper distribution.
- Confirmed the Windows and ELF libraries export only the intended six-symbol
  surface: create, run, platform registration, shutdown, screen buffer, and
  palette. Windows library and Win32 packages contain `wolf3d.dll`, and the
  standalone SDK contains `WOLF3D.h` plus `wolf3d.lib`.
- Rebuilt the native library, Win32, SDL3, Linux-console, and headless targets.
  All 150 Windows tests and all 149 Linux tests under both GCC and Clang pass.
  The Debian 10 portable package also passes its ABI audit with a maximum
  requirement of `GLIBC_2.14`.
