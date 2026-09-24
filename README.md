# wolf3dgeneric

`wolf3dgeneric` is a work-in-progress portable Wolfenstein 3D engine core in the
spirit of [doomgeneric](https://github.com/ozkl/doomgeneric). A platform host
provides a small video, input, timing, and audio boundary; the engine retains the
original game's 320x200 indexed rendering, 70 Hz timing, data formats, and game
behavior.

The project deliberately does not add modern gameplay or rendering features.
It requires separately supplied original game data and does not include any
Wolfenstein 3D assets.

Development is currently in the bootstrap phase. See
[`DEVELOPMENT_PLAN.md`](DEVELOPMENT_PLAN.md) for the staged implementation and
acceptance gates. [`docs/source-layout.md`](docs/source-layout.md) maps each
portable translation unit to its original Wolfenstein 3D source owner, and the
[`development log`](docs/development-log.md) records deterministic visual
milestones. Third-party code and exact revisions are recorded in
[`THIRD_PARTY.md`](THIRD_PARTY.md).

## Bootstrap build on Windows

Configure with a Visual Studio developer command prompt:

```text
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

The current vertical slice validates both v1.4 data editions and displays the
original title screen. To exercise it:

```text
wolf3dgeneric-win32 --data "C:\path\to\Wolf3D data"
```

Add `--play-view` to enter the current interactive game session. The original
keyboard defaults are active: arrows move and turn, Alt+left/right strafes,
Shift runs, Control attacks, Space uses, 1-4 select weapons, and Escape quits.
The game simulation advances at the original 70 Hz while the host remains free
to present frames independently. The Win32 host streams each map's original IMF
music through the official Nuked-OPL3 implementation at 48 kHz:

```text
wolf3dgeneric-win32 --data "C:\path\to\Wolf3D data" --play-view
```

The headless host can export its current indexed frame for inspection without a
window:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --dump-frame title.ppm
```

It can also render ten deterministic seconds of the selected map's music to a
standard 16-bit stereo WAV without opening an audio device:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --map 0 --dump-music e1m1.wav
```

Add `--sound N` to start one of the original zero-based AdLib effects in that
render; for example, `--sound 24` mixes the pistol with E1M1's music. Add
`--digitized` to select the corresponding original Sound Blaster sample from
VSWAP instead:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --map 0 --sound 24 --digitized --dump-music e1m1-pistol.wav
```

For mixer diagnostics, `--left-position N` and `--right-position N` accept the
original Sound Blaster Pro attenuation values from 0 (full) to 15 (silent).

The current renderer checkpoint can export the initial Episode 1, Floor 1 wall
view, static scenery, ordinary enemies and dead guards, ready pistol, and status
bar. Patrol movement, actor awareness, ordinary enemy chase movement, and the
original guard/officer/mutant/SS hitscan and dog melee attack states are active.
Hans, Gretel, Mecha-Hitler, and Hitler also retain their original hitscan burst
states, and Dr. Schabbs throws moving, colliding syringe projectiles. The other
two rocket bosses—Giftmacher and Fatface—now retain their throws, smoke trails,
explosions, and projectile damage. Fake Hitler also retains his original
eight-flame burst. Pushwall activation, movement, blocking, and moving-plane
rendering are active. Ordinary and boss damage/death paths are active, and the
player pistol, machine gun, chaingun, and knife now use their original attack
cadence and targeting rules. Player turning, thrust, collision, item collection,
cardinal use actions, locked doors, sliding-door timing, elevators, and exit
tiles now follow their original gameplay rules. Normal and secret elevators
load the correct next floor, including the episode-specific return from floor
10. Death restarts the current floor with the original score/inventory reset
and life accounting; the full death camera/fade and level intermission screens
remain to be ported:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --dump-frame view.ppm
```

Pass a zero-based `--map` number to capture another original map. The headless
host can also print the indexed framebuffer's deterministic FNV-1a value with
`--frame-hash`.

The deterministic `--forward-tics N` diagnostic holds the original forward
control for exactly `N` complete play-loop tics, including doors, pushwalls,
player movement, weapon state, and actors.

For a renderer diagnostic that exposes the scenery beyond the initial closed
door, add `--open-doors`. This only selects a fully-open door state for the
captured frame; it does not change normal level initialization.

The `--guard-view` diagnostic selects a fixed E1M1 pose facing one of the
originally spawned guards, making actor projection and rotation directly
inspectable:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --guard-view --dump-frame guard.ppm
```

The corresponding boss diagnostic finds a boss on the selected map and chooses
a clear three-tile viewing pose. For example, this renders Hans Grosse on
Episode 1, Floor 9:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --map 8 --boss-view --dump-frame hans.ppm
```

To execute one seeded shot from Hans's original six-shot burst and render the
third firing frame and resulting health change, use:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --map 8 --boss-fire-view --frame-hash --dump-frame hans-firing.ppm
```

With the full WL6 data, the projectile diagnostic executes Schabbs's original
throw action on E2M9 and advances the four-frame syringe actor into flight:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --map 18 --needle-view --frame-hash --dump-frame schabbs-needle.ppm
```

The E4M9 rocket diagnostic holds Giftmacher in his throw frame and advances a
rotating rocket and its original smoke trail toward the player:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --map 38 --rocket-view --frame-hash --dump-frame gift-rocket.ppm
```

The E3M9 flame diagnostic executes four consecutive actions from Fake Hitler's
original eight-flame burst and renders their cumulative damage:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --map 28 --flame-view --frame-hash --dump-frame fake-flames.ppm
```

The pushwall diagnostic activates the first E1M1 secret and captures its wall
plane halfway across the first tile:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --pushwall-view --frame-hash --dump-frame moving-pushwall.ppm
```

The ordinary-enemy death diagnostic applies the original surprise-hit damage
rule to an E1M1 guard, advances his collapse by 30 tics, and renders the ammo
clip dropped at his tile:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --death-view --frame-hash --dump-frame guard-death.ppm
```

The boss-death diagnostic applies lethal surprise damage to Hans on E1M9 and
advances his original collapse sequence by 30 tics:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --map 8 --boss-death-view --frame-hash --dump-frame hans-death.ppm
```

The player-fire diagnostic projects the E1M1 actors, runs the original pistol
attack states through the shot action, and captures the recoil frame, spent
ammo, earned score, and struck guard:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --player-fire-view --frame-hash --dump-frame player-pistol.ppm
```

The pickup diagnostic collects the first E1M1 cross through the player-tile
bonus path, then frames its treasure room with the consumed cross absent and
the live score updated to 100:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --pickup-view --frame-hash --dump-frame pickup-cross.ppm
```

The door-use diagnostic invokes the original use-button path on an E1M1 door
and advances its sliding plane through half of the opening cycle:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --door-use-view --frame-hash --dump-frame door-use.ppm
```

To advance E1M1's patrol actors by a deterministic number of original 70 Hz
tics and frame the first patrol, use:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --open-doors --patrol-view --actor-tics 127 --dump-frame patrol.ppm
```

The `--alert-view` diagnostic places the player in front of an E1M1 guard and
advances awareness through the original reaction delay into the first chase
state:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --alert-view --frame-hash --dump-frame alerted.ppm
```

To continue that guard through nine tics of the original chase thinker and
six-state chase animation, use `--chase-view`. The current chase checkpoint
selects directions, dodges, reserves destination tiles, waits at closed doors,
and moves while preserving the minimum player distance. The four ordinary
ranged enemy classes continue into their class-specific attack states.

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --chase-view --actor-tics 9 --frame-hash --dump-frame chase.ppm
```

The firing diagnostic selects the guard's original shot action, applies its
seeded hit and damage roll, and renders the resulting attack frame and updated
health display:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --fire-view --frame-hash --dump-frame firing.ppm
```

The dog diagnostic selects an original E1M1 dog, places the player at the
two-tile bite boundary, executes the seeded `T_Bite` action, and renders the
third leap frame and resulting health change:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --bite-view --frame-hash --dump-frame dog-bite.ppm
```

## License

The project is licensed under GPL-2.0-only. Game data is not covered by this
license and must not be committed to the repository. Third-party components keep
their own compatible licenses and notices.
