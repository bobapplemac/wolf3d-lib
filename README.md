# wolf3dgeneric

`wolf3dgeneric` is a portable Wolfenstein 3D v1.4 engine core in the spirit of
[doomgeneric](https://github.com/ozkl/doomgeneric). A platform host
provides a small video, input, timing, and audio boundary; the engine retains the
original game's 320x200 indexed rendering, 70 Hz timing, data formats, and game
behavior.

The project deliberately does not add modern gameplay or rendering features.
It requires separately supplied original game data and does not include any
Wolfenstein 3D assets.

The first-release scope supports the Apogee shareware (`WL1`) and
GT/ID/Activision full (`WL6`) v1.4 data sets. Spear of Destiny, joystick input,
and Disney Sound Source output are outside this Wolf3D release scope. See
[`DEVELOPMENT_PLAN.md`](DEVELOPMENT_PLAN.md) for the completed staged plan.
[`docs/source-layout.md`](docs/source-layout.md) maps each
portable translation unit to its original Wolfenstein 3D source owner, and the
[`development log`](docs/development-log.md) records deterministic visual
milestones. Third-party code and exact revisions are recorded in
[`THIRD_PARTY.md`](THIRD_PARTY.md). The small host API is documented in the
[`porting guide`](docs/porting-guide.md), and exact tested asset hashes are in
[`supported data`](docs/supported-data.md). Release changes and final acceptance
gates are recorded in [`CHANGELOG.md`](CHANGELOG.md) and the
[`release checklist`](docs/release-checklist.md).

## Build on Windows

Configure with a Visual Studio developer command prompt:

```text
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

The same CMake project builds on Linux with GCC or Clang; the Win32 host is
automatically omitted there. Strict Linux compiler and sanitizer jobs are
defined in `.gitlab-ci.yml`.

To run the interactive Win32 host:

```text
wolf3dgeneric-win32 --data "C:\path\to\Wolf3D data"
```

Press a key on the title screen to open the original control-panel main menu.
The arrow keys move its gun cursor; Enter selects an item and Escape returns to
the title. `New Game` opens the original episode and difficulty panels, then
starts Floor 1 of the chosen episode with that difficulty. The shareware data
retains its original locked presentation for Episodes 2 through 6.

With no input, the front end follows the original attract sequence: 15 seconds
of the title, 10 seconds of credits, 10 seconds of high scores, then one of the
four embedded demos. Completed demos return to the title and rotate to the next
stream. Any key or mouse-button press enters the control panel.
The title sequence retains `NAZI_NOR_MUS`; the control panel uses
`WONDERIN_MUS`, Read This uses `CORNER_MUS`, and View Scores uses
`ROSTER_MUS`, all through the same Nuked-OPL3 PCM boundary as gameplay.

Add `--play-view` to enter the current interactive game session. The original
keyboard defaults are active: arrows move and turn, Alt+left/right strafes,
Shift runs, Control attacks, Space uses, and 1-4 select weapons. Escape opens
the in-game control panel; `Back to Game` or a second Escape resumes the exact
paused session.
The original in-game function keys are also active: F1 Read This, F2 Save,
F3 Load, F4 Sound, F5 Change View, F6 Control, F7 End Game, F8 Quick Save,
F9 Quick Load, and F10 Quit. Destructive choices retain their Y/N confirmation
dialogs. After a slot has been chosen, quick save/load reuse it as in the DOS
game.
Raw mouse motion turns and moves with the original default sensitivity; left
button attacks, right button strafes, and middle button uses.
The Pause key displays the original pause plaque, freezes game tics, and
temporarily silences the IMF sequencer until the next key or mouse-button press.
The game simulation advances at the original 70 Hz while the host remains free
to present frames independently. The Win32 host streams each map's original IMF
music through the official Nuked-OPL3 implementation at 48 kHz:

```text
wolf3dgeneric-win32 --data "C:\path\to\Wolf3D data" --play-view
```

Embedded demos decode their original map, button bits, and signed movement
bytes. `--demo-number 0` through `3` selects a stream; all four are exercised
to completion for both supported editions by the regression suite. A
deterministic first-demo checkpoint can be rendered with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --demo-view --demo-number 0 --demo-commands 70 --frame-hash --dump-frame demo.ppm
```

Each recorded command advances one original four-tic demo frame. Automatic
title-screen attract sequencing uses the same playback path.

The credits screen can also be captured directly:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --credits-view --frame-hash --dump-frame credits.ppm
```

The headless host can export its current indexed frame for inspection without a
window:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --dump-frame title.ppm
```

The control-panel fixture uses the same original graphics and font resources:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --main-menu-view --frame-hash --dump-frame main-menu.ppm
```

Use `--episode-menu-view` or `--difficulty-menu-view` in place of
`--main-menu-view` to capture the two New Game panels.

`Load Game` and the in-game-only `Save Game` row restore the original ten-slot
panels, arrow-key navigation, slot-name entry, and `SAVEGAM?.WL1`/`.WL6`
filenames in the process working directory. The portable payload is explicitly
little-endian, versioned, edition-tagged, and checksummed; it intentionally does
not reproduce the DOS executable's pointer-sized raw structure dump, so original
DOS save payloads are not interchangeable. Use `--in-game-menu-view`,
`--load-game-view`, or `--save-game-view` for deterministic captures.

The Sound row opens the original three-section device panel. `None`, PC
Speaker, AdLib, Sound Blaster digitized effects, and music choices take effect
immediately; changing an enabled mode plays the original pistol check sound.
The PC-speaker path interprets the original sound chunks at the original 140 Hz
service rate and synthesizes the PIT channel-2 square wave. Disney Sound Source
remains disabled. Capture the panel with `--sound-menu-view`.

The Control row restores the original mouse and joystick device panel. Mouse
input can be enabled or disabled, and Mouse Sensitivity opens the original
ten-position slider; the chosen value feeds the original movement formula.
Joystick rows remain disabled because the generic event boundary does not yet
expose a joystick. Use `--control-menu-view` or
`--mouse-sensitivity-view` for deterministic captures.

`Customize controls` restores the original Mouse, Joystick/Gravis GamePad,
Keyboard, and movement-key table. The supported mouse and keyboard groups can
be edited in place: Enter selects a group and field, then the next key or mouse
button becomes its binding. The original defaults remain Control/Alt/Shift/
Space, arrow keys, and the three mouse buttons. Capture it with
`--customize-controls-view`.

`Change View` restores the original 4–19 step viewport-size panel. Arrow keys
resize its live border preview, Enter accepts, and Escape restores the previous
size. Interactive play starts at the original default size 15; walls, actors,
the weapon, targeting window, and beveled play border all use the selected
viewport. Use `--change-view` to capture the panel, or `--view-size N` with a
headless play capture to inspect any size from 4 through 20.

The main menu's `Read This!` entry decodes the original 41-page help article
directly from the selected edition's `VGAGRAPH`. Arrow keys, Enter, and Space
navigate pages; Escape returns to the control panel. Its first page can be
captured with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --help-view --frame-hash --dump-frame help.ppm
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

Use `--pc-speaker` with `--sound N` to render the corresponding original
PC-speaker effect instead.

For mixer diagnostics, `--left-position N` and `--right-position N` accept the
original Sound Blaster Pro attenuation values from 0 (full) to 15 (silent).

The headless renderer can export the initial Episode 1, Floor 1 wall
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
10. Completed normal and secret floors stop at the original intermission
screen, calculate bonuses from live statistics, play the original results
music, and wait for acknowledgement before loading the destination floor.
Death turns toward the fatal attacker at the original two degrees per tic,
removes the weapon, runs the original 70-frame red fizzle, waits for the death
sound and input timeout, then restarts the current floor with the original
score/inventory reset and life accounting:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --dump-frame view.ppm
```

The player-death diagnostic captures the transition halfway through its red
fizzle. It is identical for the supplied WL1 and WL6 data:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --player-death-view --frame-hash --dump-frame player-death.ppm
```

Defeating an episode boss now preserves the first eight floor ratios, switches
to `URAHERO_MUS`, and displays the original total-time and average-ratio victory
screen. Acknowledging it opens the episode's original two-page EndText article;
arrow keys move between pages and Escape continues to the high-score table. Its
deterministic victory diagnostic uses eight 1:15 perfect floors:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --victory-view --frame-hash --dump-frame victory.ppm
```

The first EndText page can be captured directly with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --end-text-view --frame-hash --dump-frame end-text.ppm
```

Running out of lives now ranks the final score with the original seven-entry
table, displays `DrawHighScores`, and switches to `ROSTER_MUS`. The initial
table can be captured independently:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --high-score-view --frame-hash --dump-frame high-scores.ppm
```

Qualifying scores accept the original scan-code-derived text input, including
Shift/Caps Lock, cursor movement, insertion, deletion, Enter, Escape, and the
100-pixel name limit. The editing fixture captures `BJ` and the I-bar cursor:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --high-score-entry-view --frame-hash --dump-frame high-score-entry.ppm
```

Pass a zero-based `--map` number to capture another original map. The headless
host can also print the indexed framebuffer's deterministic FNV-1a value with
`--frame-hash`.

To render the original pause plaque over the initial play view, use:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --pause-view --frame-hash --dump-frame paused.ppm
```

The completed-floor diagnostic renders a 1:15 perfect E1M1 result using the
actual authored totals discovered while building the map:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --intermission-view --frame-hash --dump-frame intermission.ppm
```

Palette-only feedback can be inspected independently of indexed pixels. These
diagnostics render the original damage-red and bonus-white shifts and print the
palette hash when requested:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --damage-flash-view --palette-hash --dump-frame damage.ppm
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --bonus-flash-view --palette-hash --dump-frame bonus.ppm
```

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
