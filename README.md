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
milestones.

## Bootstrap build on Windows

Configure with a Visual Studio developer command prompt:

```text
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

The initial vertical slice validates both v1.4 data editions and displays the
original title screen. It is not yet a playable engine. To exercise it:

```text
wolf3dgeneric-win32 --data "C:\path\to\Wolf3D data"
```

The headless host can export its current indexed frame for inspection without a
window:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --dump-frame title.ppm
```

The current renderer checkpoint can export the initial Episode 1, Floor 1 wall
view, static scenery, ordinary enemies and dead guards, ready pistol, and status
bar. Patrol movement, actor awareness, ordinary enemy chase movement, and the
original guard/officer/mutant/SS hitscan and dog melee attack states are active.
Hans, Gretel, Mecha-Hitler, and Hitler also retain their original hitscan burst
states. Projectile bosses, boss death transitions, and pushwall motion are not
yet active:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --dump-frame view.ppm
```

Pass a zero-based `--map` number to capture another original map. The headless
host can also print the indexed framebuffer's deterministic FNV-1a value with
`--frame-hash`.

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
