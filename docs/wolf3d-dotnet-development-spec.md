# wolf3d-dotnet Development Specification

## Instructions to the implementing agent

This document is the authoritative project brief for a new, separate Git
repository named `wolf3d-dotnet`. Implement the work incrementally, preserve
every completed stage as an independently buildable product, and use the
existing `wolf3dgeneric` repository as the behavioral and architectural
reference.

Before changing source code:

1. Locate the user-provided `wolf3dgeneric` checkout and its associated
   reference-material directory. Do not assume that their paths match the
   paths from another Codex task.
2. Treat `wolf3dgeneric` as read-only unless the user explicitly authorizes a
   change to that repository.
3. Read its public header, architecture documentation, build documentation,
   supported-data documentation, source-layout map, development log, parity
   audit, tests, and license.
4. Inventory the available .NET SDK, C/C++ build tools, native libraries, and
   target architectures.
5. Present a concrete implementation plan, proposed initial SDK and package
   versions, identified risks, and verified tool availability for user review.
6. After approval, work autonomously within this specification. Commit small,
   coherent milestones and keep the repository buildable.

When evidence conflicts, use this priority order:

1. The original Wolfenstein 3D/Spear of Destiny source and shipped data
2. Verified behavior from an original DOS executable
3. The preservation-oriented `wolf3dgeneric` implementation and test corpus
4. Chocolate Wolfenstein or another fidelity-oriented modern port
5. Other ports and secondary documentation

Do not silently substitute conventional modern-engine behavior for behavior
that can be established from those references.

## Mission

Create a managed Wolfenstein 3D implementation in three preserved stages:

1. **Stage 1 — Native hosting:** thin C# hosts for the existing
   `wolf3dgeneric` native library.
2. **Stage 2 — Faithful managed port:** a direct, behaviorally exact C# port of
   the native engine that deliberately retains the historical structure and
   naming.
3. **Stage 3 — Readable managed engine:** a clean, explanatory C# engine using
   extracted modern-format assets while retaining the original gameplay,
   timing, fixed-point mathematics, and canonical rendering behavior.

The repository is both working software and an educational cross-reference.
It should show the progression from hosting the C library, through a literal
managed translation, to a modern and readable expression of the same engine.

## Non-negotiable principles

- Every stage remains present, buildable, testable, and runnable after later
  stages begin.
- Stage 2 is never overwritten by Stage 3 cleanup.
- Stage 1 always remains capable of exercising the C oracle.
- The three engines must not achieve apparent parity by sharing their engine
  implementations.
- Shared platform contracts, platform backends, trace schemas, and test
  vectors are encouraged. Engine rules, state transitions, renderers, and
  resource implementations are not shared between Stage 2 and Stage 3.
- Original game data remains user-supplied. Do not commit or redistribute
  proprietary game files or extracted assets.
- The canonical simulation clock is 70 Hz. Audio production is independent of
  the logical game clock.
- Preserve original elapsed-`tics` batching semantics, including values 1
  through 10; do not replace a single `tics=N` update with N calls using
  `tics=1` unless equivalence is proven for that phase.
- Preserve fixed-width arithmetic, truncation, overflow, signed division,
  shifts, lookup tables, random-consumption order, and call order.
- The original gameplay RNG is the original table/index generator. The
  fizzle fade uses a separate 17-bit LFSR. Do not conflate them.
- Canonical rendering remains a 320x200, 8-bit indexed framebuffer with a
  256-entry RGB palette. Aspect correction belongs to the host and displays
  that image in a 4:3 viewport.
- Fidelity fixes must be supported by reference evidence and permanent tests.
- Readability work must not be allowed to change gameplay accidentally.

## Repository and solution layout

Use one repository and one primary solution. The exact project names may be
adjusted for valid .NET naming, but preserve the following separation:

```text
wolf3d-dotnet/
  src/
    Shared/
      Wolf3D.Platform/
      Wolf3D.TestVectors/

    Platforms/
      Wolf3D.Platform.Headless/
      Wolf3D.Platform.WinForms/
      Wolf3D.Platform.Sdl3/
      Wolf3D.Platform.Sfml/

    Stage1.Native/
      Wolf3D.NativeInterop/
      Wolf3D.Native.Headless/
      Wolf3D.Native.WinForms/
      Wolf3D.Native.Sdl3/
      Wolf3D.Native.Sfml/

    Stage2.Faithful/
      Wolf3D.Faithful.Core/
      Wolf3D.Faithful.Audio/
      Wolf3D.Faithful.Headless/
      Wolf3D.Faithful.WinForms/
      Wolf3D.Faithful.Sdl3/
      Wolf3D.Faithful.Sfml/

    Stage3.Readable/
      Wolf3D.Readable.Game/
      Wolf3D.Readable.Rendering/
      Wolf3D.Readable.Assets/
      Wolf3D.Readable.Audio/
      Wolf3D.Readable.Headless/
      Wolf3D.Readable.Sfml/

  tests/
    Wolf3D.NativeInterop.Tests/
    Wolf3D.Faithful.UnitTests/
    Wolf3D.Faithful.CorpusTests/
    Wolf3D.Readable.UnitTests/
    Wolf3D.Readable.CorpusTests/
    Wolf3D.CrossEngine.Tests/

  tools/
    Wolf3D.TraceCompare/
    Wolf3D.CorpusRunner/
    Wolf3D.AssetManifest/

  docs/
    architecture.md
    building.md
    development-log.md
    fidelity-contract.md
    native-interop.md
    source-cross-reference.md
    supported-data.md

  Directory.Build.props
  Directory.Packages.props
  global.json
  wolf3d-dotnet.sln
```

The platform projects implement presentation and hardware access. Small
composition applications select one engine plus one platform. For example:

```text
Wolf3D.Native.WinForms
  -> Wolf3D.NativeInterop + Wolf3D.Platform.WinForms

Wolf3D.Faithful.Sdl3
  -> Wolf3D.Faithful.Core + Wolf3D.Platform.Sdl3

Wolf3D.Readable.Sfml
  -> Readable projects + Wolf3D.Platform.Sfml
```

This preserves distinct runnable stage outputs without copying platform code.

## Common lifecycle and platform contract

Define an engine-neutral managed lifecycle early. Its exact shape may evolve,
but it should support the equivalent of:

```csharp
public interface IWolfEngine
{
    WolfResult Create(string[] arguments, IWolfPlatform platform);
    WolfResult Run();
    void Shutdown();
}
```

The managed platform contract must cover the existing native ABI:

- Initialization and shutdown
- Indexed framebuffer and palette presentation
- Monotonic millisecond time
- Short sleeps/yields
- Keyboard, mouse, joystick, and quit events
- Interactive/headless determination
- Window title
- Informational and error messages
- IBM text-cell presentation for DOS exit/error screens
- PCM initialization, capacity, submission, and shutdown

Preserve the native key namespace: IBM PC set-1 scan codes cross the engine
boundary. Platform-specific key identifiers must remain inside the host.

Platform implementations should be independently testable with a synthetic
engine that generates frames, palette changes, audio, text output, and input
echoes. Do not require the Wolf engine to test basic host behavior.

## Stage 0 — Foundation and oracle

### Objectives

- Establish repository conventions and deterministic builds.
- Freeze a known `wolf3dgeneric` revision as the native oracle.
- Define trace, fixture, and comparison formats before translating the engine.
- Prove the managed platform abstraction without committing to a GUI backend.

### Required work

1. Select a supported .NET LTS SDK and pin it with `global.json`.
2. Enable nullable reference types, deterministic builds, package lock files,
   warnings as errors, and a documented analyzer policy.
3. Allow explicit, narrowly documented warning suppressions for historical
   Stage 2 naming and structure.
4. Record the exact `wolf3dgeneric` commit used as the oracle.
5. Provide configuration for native library and original-data locations
   without embedding machine-specific absolute paths.
6. Port or consume the existing non-proprietary unit vectors for fixed math,
   fizzle behavior, RNG, decompression, and renderer tables.
7. Define a versioned, endian-independent trace schema containing at least:
   - Logical tic/frame number
   - Map and difficulty
   - Player position, angle, health, weapon, ammo, score, and lives
   - RNG index
   - Actor states and positions in stable pool order
   - Doors and pushwall state
   - Gameplay flags and current phase
   - Framebuffer and palette hashes
   - Optional audio-command and PCM hashes
8. Prefer an external test adapter or existing diagnostics over modifying the
   oracle repository. If a new native diagnostic export is truly required,
   stop and request permission before changing `wolf3dgeneric`.

### Stage 0 acceptance gate

- Clean restore/build/test from documented commands.
- Headless platform contract tests pass on Windows and Linux.
- Native oracle revision and data-corpus configuration are documented.
- At least one deterministic native demo or scripted scenario produces a
  stable trace that can be parsed by managed tooling.

## Stage 1 — Native C library hosted by C#

### Native interop project

Map the `wolf3dgeneric` public ABI exactly. The implementing agent must inspect
the current header rather than rely only on this summary.

Interop requirements:

- Use sequential native layout for callback and event structures.
- Represent C `size_t` as `nuint`.
- Represent C enums and `int` fields as 32-bit integers, not marshalled C#
  `bool` values.
- Use the correct C calling convention on every export and callback.
- Marshal `argv` as explicitly allocated, null-terminated UTF-8 strings and
  free it after native creation no longer requires it.
- Root callbacks for the entire native-engine lifetime.
- Never permit a managed exception to unwind through native code. Catch it,
  store the failure, and return a safe error value.
- Treat framebuffer, palette, text-cell, and PCM pointers as borrowed memory
  valid only for the duration of the callback.
- Make lifecycle shutdown idempotent.
- Do not unload the native DLL while a callback or engine thread can still run.
- Resolve platform-appropriate native library names and architecture-specific
  packages predictably.
- Test structure sizes and field offsets on Windows x86, Windows x64, and
  Linux x64 where supported.

### Stage 1A — Managed headless host

Implement this first. It should provide:

- Deterministic virtual time for tests
- Scripted input
- Frame, palette, and PCM hashing
- Optional frame export for local diagnostics
- Captured text-screen and error output
- No dependency on a desktop session

Use it to execute native smoke tests and the supported demo corpus before
building GUI hosts.

### Stage 1B — WinForms/GDI host

This is a serious supported experiment, not a throwaway example. The workload
is small enough for GDI if implemented without allocation-heavy paths.

Required design:

```text
WinForms UI thread
  - owns the window and message pump
  - receives keyboard/mouse/window events
  - paints only the newest published frame

Engine worker thread
  - executes wolf3dgeneric_Run
  - uses a monotonic Stopwatch-based clock callback
  - publishes frames through a bounded mailbox
  - submits audio independently of painting
```

Presentation requirements:

- Maintain two persistent 320x200 indexed buffers or an equivalent bounded
  latest-frame mailbox.
- Never call `SetPixel`.
- Never allocate a new `Bitmap` for each frame.
- Prefer a persistent `BITMAPINFO` and `StretchDIBits`, or a demonstrably
  equivalent persistent locked-bitmap path.
- Update the 256-entry color table only as required by palette changes.
- Use nearest-neighbor/GDI `COLORONCOLOR` scaling.
- Calculate a centered 4:3 output rectangle with letterboxing.
- Coalesce invalidation so at most one unprocessed repaint request exists.
- If rendering falls behind, discard obsolete presentation frames rather than
  delaying simulation or audio.
- Support windowed/fullscreen toggle behavior consistent with wolf3dgeneric.
- Hide the pointer when appropriate, especially in fullscreen mode.

Do not drive simulation with `System.Windows.Forms.Timer`. The native engine
owns its scheduler through the platform timing callbacks.

Input events flow from the UI thread through a bounded thread-safe queue to
the engine callback. Audio uses its own bounded ring or device queue and must
never wait on `OnPaint` or synchronous UI dispatch.

Performance gate:

- Sustain a long-running game session without unbounded allocations, event
  queue growth, audio starvation, or simulation slowdown.
- Record frame-copy, paint, dropped-presentation, and allocation measurements.
- A compositor-limited 60 Hz display is acceptable; the simulation must still
  maintain correct 70 Hz logical timing.

### Stage 1C — SDL3 host

Use the existing native SDL3 wrapper as the behavioral blueprint.

- Pin one reviewed C# SDL3 binding or implement a deliberately small local
  interop surface for only the required SDL APIs.
- Do not spread binding-specific SDL types into the shared platform contract.
- Match the existing SDL3 host's input, relative mouse, gamepad, audio,
  fullscreen, title, console, and 4:3 presentation behavior.
- Support Windows and Linux.
- Package the required native SDL3, wolf3dgeneric, and Nuked-OPL3 libraries
  beside the executable with licenses and notices.

SDL3 is the primary cross-platform Stage 1 frontend and the initial graphical
frontend for Stage 2.

### Stage 1D — SFML host

Implement an SFML.Net frontend after the native SDL3 host is stable.

- Keep it functionally equivalent to the other platform projects.
- Treat it as an early validation of the API and packaging approach intended
  for Stage 3.
- Do not distort the shared platform abstraction merely to imitate SFML's
  object model.
- Support Windows and Linux if the selected official package supports the
  required native targets.

### Stage 1 acceptance gate

- Native headless corpus passes.
- WinForms/GDI is measured and either accepted as supported or retained with a
  clearly documented limitation.
- SDL3 runs on Windows and Linux with video, input, audio, fullscreen, and
  console behavior.
- SFML runs at least the full supported game loop on the maintained targets.
- Every native host uses the same oracle DLL without engine modifications.
- Each frontend has a minimal distributable package and documented launch
  instructions.

## Stage 2 — Faithful C# port

### Purpose

Translate the portable C engine to C# with behavior and structure as close as
reasonably possible to the reference. This stage is a preservation artifact,
not an idiomatic C# redesign.

### Structural policy

- Retain original uppercase historical basenames using `.cs`, such as
  `WL_GAME.cs`, `WL_PLAY.cs`, `WL_ACT1.cs`, `ID_CA.cs`, and `ID_SD.cs`.
- New generic boundaries use `WG_` names where they correspond to the C port.
- Static or partial-static classes may represent C translation units.
- Retain original function, state, and variable names when doing so aids
  cross-reference.
- Maintain `docs/source-cross-reference.md` mapping original DOS symbols,
  wolf3dgeneric symbols, and faithful C# symbols.
- Do not introduce entity-component systems, actor inheritance hierarchies,
  scene graphs, dependency-injection frameworks, or speculative abstractions.
- Do not use an automatic C-to-C# converter as an authority. Mechanical tools
  may generate inventories or scaffolding, but all semantics require review.

### Arithmetic and determinism policy

- Use explicit fixed-width types. Remember that C# `long` is always 64-bit and
  must not be used merely because the C source says `long`.
- Use explicit `unchecked` contexts where the native engine relies on wrapping
  arithmetic.
- Reproduce every narrowing conversion at the same semantic point.
- Audit signed right shifts, divisions, remainders, and negative-value paths.
- Preserve the verified fixed-point multiplication and division routines.
- Port frozen trigonometric/projection tables; do not regenerate them with
  modern floating-point library functions at runtime.
- Preserve the original random table and index exactly, including the number
  and order of calls.
- Preserve the 17-bit fizzle LFSR independently.
- Preserve actor pool order, map scan order, state-transition order, collision
  order, and draw-triggered gameplay effects.
- Avoid LINQ, iterators, closures, reflection, and transient allocations in
  simulation or rendering hot paths.
- Use arrays, spans, and explicit indices where they accurately model bounded
  C arrays and object pools. Be alert to C# struct copy semantics.
- Parse and serialize binary formats explicitly little-endian; do not marshal
  compiler layouts directly.

### Translation sequence

Translate in dependency-complete slices:

1. Fixed-point helpers, endian access, RNG, fizzle, and primitive utilities
2. Data-profile discovery and file access
3. Graphics, map, page, and audio-resource decoding
4. Palette, picture, font, and primitive video operations
5. Projection tables, raycaster, wall drawing, and sprite scaling
6. Map construction, statics, actors, and state tables
7. Doors, pushwalls, collision, and area connectivity
8. Player control, weapons, damage, pickups, and play loop
9. Menus, articles, intermissions, victory, death, saves, and attract loop
10. Sound effects, IMF sequencing, digitized playback, and managed OPL
11. Complete application orchestration and all supported editions

For every slice:

1. Identify the original and native reference symbols.
2. Port or create focused tests first.
3. Translate the minimum dependency-complete implementation.
4. Compare its results against the native oracle.
5. Diagnose the first divergent operation rather than patching final output.
6. Document any intentional safety modernization.
7. Commit only after the slice is deterministic and the previous suite passes.

Do not create a production engine that crosses between native and managed code
for individual rays, actors, or ticks. The native engine remains an external
oracle; the faithful managed engine must be independently complete.

### Managed Nuked-OPL3

The user has an existing C# Nuked-OPL3 translation that will be supplied to
the project. Treat it as a starting point, not automatically current truth.

1. Diff it against the selected official Nuked-OPL3 revision.
2. Update the faithful managed implementation first.
3. Compare register writes and generated PCM against the C library.
4. Make the faithful implementation the default.
5. Add an explicitly selected fast implementation based on the reviewed
   optimization techniques from Nuked-OPL3-fast.
6. Keep both behind a small `IOpl3Core`-style boundary.
7. Keep license, provenance, and modification notices in the audio project.

Never silently select the fast core merely because a benchmark is better.

### Stage 2 frontend policy

- Headless is the correctness and corpus frontend.
- SDL3 is the first supported graphical frontend and parity reference because
  it can be compared directly with the existing native SDL3 host.
- SFML is the second graphical frontend and de-risks the Stage 3 target.
- WinForms/GDI should be composed with the faithful engine if its Stage 1
  performance experiment succeeded; otherwise preserve it as Stage 1 only.

### Stage 2 acceptance gate

- All supported data profiles load correctly.
- All embedded demos match the native engine at every recorded checkpoint and
  final state.
- Canonical frame and palette hashes match for maintained visual scenarios.
- Renderer tables, fixed math, angle quantization, RNG sequences, and fizzle
  behavior match exactly.
- Live `tics=1..10` scenarios match native state transitions.
- Save files round-trip with the documented portable format and malformed
  inputs fail safely.
- Faithful managed OPL matches the selected C reference for the permanent
  audio vectors.
- Headless, SDL3, and SFML builds pass on maintained platforms.
- The native Stage 1 projects still build and pass their tests.

## Stage 3 — Readable modern managed engine

### Purpose

Re-express the original engine for human understanding while preserving its
gameplay and canonical mathematics. Remove obsolete DOS-era implementation
constraints without turning Wolfenstein 3D into a different game.

Stage 2 remains untouched as the literal reference.

### Target architecture

Use cohesive systems with plain responsibilities, for example:

```text
Wolf3D.Readable.Game
  GameSession
  World
  Player
  ActorSystem
  DoorSystem
  PushwallSystem
  GameClock

Wolf3D.Readable.Rendering
  VisibilityPass
  Raycaster
  WallRenderer
  SpriteRenderer
  WeaponRenderer
  HudRenderer
  IndexedFramebuffer

Wolf3D.Readable.Assets
  AssetCatalog
  EditionProfile
  MapCatalog
  SpriteDefinition
  AnimationDefinition

Wolf3D.Readable.Audio
  SoundSystem
  ImfPlayer
  OplMusicPlayer
  DigitalSoundPlayer
```

Prefer clear data flow, explicit ownership, meaningful modern names, focused
types, and comments that explain why the original algorithm works.

### Asset pipeline

The user will supply a C# `WolfExtract` program capable of exporting original
assets to formats such as PNG and OGG. Integrate through a documented external
pipeline; do not commit its proprietary outputs.

Require a versioned manifest alongside media. It should retain at least:

- Source edition/profile and hashes
- Stable logical asset identifiers
- Dimensions and palette association
- Sprite origins, offsets, and transparency semantics
- Animation relationships and frame timing
- Wall/sprite/map object relationships
- Font metrics and article metadata
- Audio type, sample rate, and original logical identifier

A PNG alone is not a complete replacement for a compiled Wolf3D sprite.

Decode all required assets at startup and retain them in memory. Remove cache
eviction, page locking, near/far pointers, segmented memory, VGA planes, and
other historical mechanisms that no longer serve behavior.

Preserve the point at which live-game randomness is initialized; faster asset
loading must not accidentally move seed selection or random consumption.

### Canonical and convenience presentation

The canonical Stage 3 renderer remains 320x200 indexed software rendering.
This preserves:

- Palette shifts and fades
- Exact fizzle behavior
- Original pixel and frame hashes
- Simple nearest-neighbor 4:3 presentation
- A clear, inspectable raycasting pipeline

Truecolor, high-resolution, enhanced-texture, or GPU-rendered modes may be
explored only as explicit optional modes after canonical parity is complete.

OGG music and sound are convenience presentation modes, not sample-identical
replacements for live OPL and original digitized playback. Keep managed OPL as
the canonical audio mode.

### Rendering-side-effect warning

The original engine has gameplay-significant work associated with rendering,
including visibility-driven actor activation and collectible resolution. Do
not merely separate simulation from rendering and assume equivalence.

Modernize those paths in this order:

1. Reproduce the Stage 2 call order and side effects.
2. Expose each hidden side effect explicitly.
3. Add before/after state traces around it.
4. Move it into a clearly named phase.
5. Prove identical results across demos, scripted scenarios, and `tics=1..10`.

An eventual readable frame may use explicit phases such as:

```text
sample input
-> advance simulation with elapsed tics
-> determine visibility
-> perform historically visibility-triggered activation/collection
-> render walls
-> render sprites
-> render weapon and HUD
-> present
```

The exact ordering must be derived from evidence, not this illustrative list.

### Stage 3 acceptance gate

- Complete game flow works for every supported data profile.
- Gameplay traces match Stage 2 for demos and maintained scripted scenarios.
- Fixed-point calculations, collision, actor decisions, RNG use, doors,
  pushwalls, and timing remain identical.
- Canonical indexed presentation matches the defined Stage 2 visual contract.
- Managed OPL remains available and faithful; extracted audio is optional.
- All assets are cached without legacy memory-management machinery.
- SFML is the primary polished Windows/Linux frontend.
- The life of a frame is documented and understandable from input through
  presentation.
- Stage 1 and Stage 2 remain independently buildable and tested.

## Differential testing strategy

### Test layers

1. **Pure unit tests:** arithmetic, parsing, compression, RNG, fizzle, palette,
   geometry, state transitions, and validation failures.
2. **Synthetic integration tests:** tiny constructed maps and asset fixtures
   requiring no proprietary data.
3. **Private corpus tests:** user-supplied WL1/WL6/SDM/SOD/SD1/SD2/SD3 data.
4. **Demo differential tests:** compare checkpoints and final state across
   native, faithful, and readable engines.
5. **Visual tests:** compare indexed framebuffer and palette separately,
   avoiding host scaling differences.
6. **Audio tests:** compare command sequences and PCM hashes with documented
   tolerances only where exact equality is impossible and justified.
7. **Host tests:** synthetic frame/audio/input exercises for each platform.
8. **Long-run tests:** queue bounds, allocation rate, audio continuity, and
   shutdown/restart behavior.

### Cross-engine runner

The corpus runner should be able to execute the same scenario through:

```text
wolf3dgeneric native oracle
Stage 1 native hosting path
Stage 2 faithful managed engine
Stage 3 readable managed engine
```

Stage 1 validates interop and host behavior. Stage 2 validates translation.
Stage 3 validates that refactoring preserved intended behavior.

When a mismatch occurs, report the earliest divergent tic and the smallest
useful state difference. A final framebuffer mismatch without intermediate
state information is not an adequate diagnostic.

### Commercial-data handling

- Locate data using ignored local configuration, environment variables, or
  explicit command arguments.
- Never copy original or extracted data into tracked test directories.
- Permanent baselines may contain hashes, dimensions, logical metadata, and
  independently authored synthetic fixtures, but not proprietary content.
- CI must pass its public/synthetic suite without commercial data. Full corpus
  validation may run on authorized private hosts.

## Build, packaging, and target policy

- Provide normal `dotnet restore`, `dotnet build`, and `dotnet test` workflows.
- Provide a checked-in solution suitable for Visual Studio.
- Use compiler- and runtime-independent output paths that do not mix target
  architectures.
- Initially maintain Windows x64 and Linux x64. Maintain Windows x86 for the
  Stage 1 native path while the x86 native oracle is supported.
- Keep native dependencies beside packaged applications with deterministic
  resolution; do not depend on arbitrary system search paths.
- Produce minimal publish folders for each maintained stage/frontend pair.
- Document framework-dependent and self-contained publishing decisions.
- Do not enable trimming or NativeAOT until reflection, callbacks, native
  resolution, and asset loading are explicitly proven compatible.
- Pin NuGet and native dependency versions. Record why each dependency exists.
- Add automated checks preventing proprietary game data from entering commits
  or release archives.

Recommended executable identities include:

```text
wolf3d-native-winforms
wolf3d-native-sdl3
wolf3d-native-sfml
wolf3d-faithful-sdl3
wolf3d-faithful-sfml
wolf3d-readable-sfml
```

## Documentation requirements

Maintain throughout development:

- Build and packaging instructions for each maintained OS and architecture
- Architecture and stage-boundary explanation
- Native ABI mapping and callback lifetime rules
- Source cross-reference for Stage 2
- Fidelity contract distinguishing exact, equivalent, and intentionally modern
  behavior
- Supported data profiles and identification rules
- Asset-manifest specification
- Managed OPL provenance and accurate/fast mode behavior
- Development log with test evidence and notable discoveries
- Release checklist covering every preserved stage

Graphical milestones should be captured for the user, but do not commit
screenshots containing proprietary assets unless the user explicitly confirms
that their inclusion is appropriate.

## Licensing and provenance

- Begin from the GPL-2.0 licensing and provenance obligations of the original
  Wolfenstein-derived code and `wolf3dgeneric`, unless the user makes a
  separately reviewed licensing decision.
- Preserve original copyright notices and clearly label translated or modified
  files.
- Keep the managed Nuked-OPL3 component and modifications clearly identified
  under its applicable LGPL terms.
- Preserve SDL, SFML, and other dependency notices in source and packages.
- Do not redistribute original game data or WolfExtract output.
- Record source revisions for every imported or translated component.

This section is a conservative engineering policy, not legal advice. Escalate
uncertain redistribution or combined-work questions to the user rather than
making an undocumented assumption.

## Agent development protocol

- Prefer evidence-backed implementation over speculation.
- Use small commits with descriptive messages.
- Do not mix broad formatting changes with semantic translations.
- Keep unrelated user changes intact.
- Run focused tests during a slice and the complete available suite before a
  milestone commit.
- Build with all maintained architectures and compilers before declaring a
  stage complete.
- Record exact commands, revisions, and test totals in milestone reports.
- Show visual milestone output to the user when a new host or renderer becomes
  functional.
- If an apparent cleanup changes a trace, stop and explain the divergence;
  never normalize the baseline to hide it.
- If required authority, proprietary inputs, credentials, or external systems
  are unavailable, exhaust safe alternatives and then report the precise
  blocker.
- Do not begin a later stage merely because an earlier frontend displays a
  frame. Satisfy the complete acceptance gate first.

## Milestone sequence

1. Repository foundation, reference inventory, and approved plan
2. Shared platform contract and synthetic headless platform
3. Native interop plus managed headless oracle execution
4. Native WinForms/GDI host and performance report
5. Native SDL3 Windows/Linux host
6. Native SFML Windows/Linux host
7. Stable trace/corpus comparison tooling
8. Faithful utilities and resource pipeline
9. Faithful canonical renderer
10. Faithful gameplay simulation
11. Faithful menus, saves, intermissions, and complete flow
12. Faithful managed Nuked-OPL3 and complete audio
13. Faithful SDL3/SFML releases and Stage 2 parity sign-off
14. Modern asset manifest and WolfExtract integration
15. Readable simulation and world model
16. Readable canonical renderer with explicit frame phases
17. Readable audio and complete game flow
18. SFML-focused Stage 3 release and cross-stage final validation

## Final definition of done

The overall project is complete when:

- All three stages remain independently buildable and runnable.
- Stage 1 provides working native headless, GDI where viable, SDL3, and SFML
  hosts.
- Stage 2 is an independently complete managed engine with preservation-grade
  parity against the native oracle.
- Stage 3 is an independently complete, readable engine with documented modern
  architecture and proven gameplay parity against Stage 2.
- The canonical presentation retains original 320x200 indexed behavior and
  4:3 host output.
- Faithful managed OPL is the default, with optional explicit fast and
  extracted-audio modes.
- Windows and Linux packages are minimal, documented, and contain all required
  redistributable dependencies and notices but no game data.
- Public tests run without proprietary assets, and the authorized private
  corpus passes across maintained engines and hosts.
- A reader can follow the source progression and understand both the original
  implementation and the readable life of a rendered frame.

