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
view, static scenery, initial ordinary enemies and dead guards, ready pistol,
and status bar. Actor behavior and pushwall motion are not yet active:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --dump-frame view.ppm
```

For a renderer diagnostic that exposes the scenery beyond the initial closed
door, add `--open-doors`. This only selects a fully-open door state for the
captured frame; it does not change normal level initialization.

The `--guard-view` diagnostic selects a fixed E1M1 pose facing one of the
originally spawned guards, making actor projection and rotation directly
inspectable:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --guard-view --dump-frame guard.ppm
```

## License

The project is licensed under GPL-2.0-only. Game data is not covered by this
license and must not be committed to the repository. Third-party components keep
their own compatible licenses and notices.
