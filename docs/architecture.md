# Architecture

wolf3dgeneric keeps the original game's indexed 320x200 presentation and game
rules while replacing assumptions that only hold in 16-bit DOS. The portable
core is C99 and communicates with a host through the deliberately small
`WG_PLATFORM.h` contract.

Translated files retain their original uppercase `ID_*` and `WL_*` basenames
when they have a clear historical owner. New generic boundaries use uppercase
`WG_*`; extensions remain lowercase for modern toolchain portability.
The complete mapping and naming policy is recorded in
[`source-layout.md`](source-layout.md).

## Resource boundary

The resource modules parse the original, unmodified data files into explicitly
sized host-memory objects:

- `wg_graphics`: VGAHEAD/VGADICT/VGAGRAPH Huffman chunks, picture dimensions,
  and planar-to-chunky picture conversion.
- `wg_maps`: MAPHEAD/GAMEMAPS headers and Carmack plus RLEW map-plane decoding.
- `wg_pages`: bounds-checked VSWAP wall, sprite, and digitized-sound pages.
- `wg_assets`: 64x64 wall textures and validated compiled-sprite post streams,
  including the original signed pixel-pool offsets.
- `wg_audio`: AUDIOHED/AUDIOT chunk lookup for PC speaker, AdLib, digitized
  sound metadata, and IMF music.

The file formats are read byte-by-byte with little-endian helpers. No compiler
packing, native pointer width, or unaligned host access is part of the format
contract. Malformed offsets and compressed streams fail at the resource
boundary instead of propagating unchecked pointers into game code.

The generated headers assign an implicit 4,608-byte expanded size to the TILE8
chunk. The supplied v1.4 streams do not produce that many bytes before their
record boundary under the otherwise verified Huffman tree. The original cache
routine has no input bound and can read into its scratch buffer. wolf3dgeneric
does not reproduce that unsafe over-read: TILE8 remains isolated pending a
reference-behavior capture. All explicit-size graphics chunks used by the title,
menus, status bar, and game flow are decoded and corpus-tested.

## Presentation boundary

The core owns one 320x200 byte-per-pixel framebuffer and a 256-entry RGB
palette. A host presents that pair, reports input events, supplies monotonic
time and sleeping, and reports fatal errors. The Win32 reference host uses GDI;
the headless host supplies deterministic virtual time for tests.

The portable video layer implements clipped plots, bars, picture blits,
proportional bitmap fonts, VGA-precision palette interpolation, and the
original 17-bit fizzle LFSR. The logical operations are tested without a host;
the headless host can also export any presented indexed frame as a PPM image.
Keyboard events use the original IBM PC set-1 scan-code values so gameplay and
menu code do not depend on a platform's virtual-key numbering.

## Gameplay state boundary

Map planes are converted into an explicitly sized 64x64 runtime level. Plane 0
values below the original `AREATILE` value become collision tiles; area numbers
remain zero in that byte map. Plane 1 is retained verbatim for actor and object
spawning. The initial player position and direction use the original tile codes
19 through 22 and 16.16 tile-center coordinates.

Static-object codes 23 through 71 are scanned in original map order into the
original 400-object capacity. Their shape numbers preserve the `SPR_DEMO`,
`SPR_DEATHCAM`, `SPR_STAT_0` enumeration relationship rather than using a new
asset-name table. Code 71 retains the original final `SPR_STAT_26` duplicate
ammo-clip entry from `statinfo`.

The active-object setup recognizes standing and patrolling guards, officers,
SS, dogs, and mutants at the original easy, medium, and hard map-code
thresholds, plus inert dead guards. The seven original bosses and four ghost
states use their fixed initial shapes and original north, south, or east
directions. Actors retain their tile-center 16.16 position, eight-way direction,
initial state shape, the original 150-object capacity, and `SpawnPatrol`'s
one-tile destination advance. The original map format reserves standing-dog
codes although the DOS `SpawnStand` switch omits the dog case; the portable
setup gives those entries the dog's path pose without advancing it, avoiding the
original undefined stale-object behavior. The legacy `WG_LevelBuild` entry
point selects medium difficulty; an explicit difficulty entry point supports
deterministic setup testing. Corpus validation checks that every actor spawned
by all 10 WL1 and 60 WL6 maps names a sprite page present in that edition.

The first dynamic actor slice ports the original path-state timing and
`T_Path`/`SelectPathDir` movement into `WL_STATE.c`. Patrol actors retain the
randomized initial tic phase, six-state `20/5/15/20/5/15` animation cycle,
class-specific speed, destination-tile reservation, tile-center snapping, and
direction arrows stored in map plane 1. Cardinal and diagonal wall/actor checks
preserve the distinction between ordinary actors and dogs, and actors stop at
closed doors until the door reaches its fully open position. Chase movement,
combat states, and player/actor contact remain subsequent slices.

The awareness slice retains all 37 original area numbers separately from the
collision byte map. An ambush marker (plane-0 tile 106) is cleared and assigned
the neighboring area using `SpawnStand`'s original right/up/down/left overwrite
order. A bounded traversal of the original door-to-area graph derives the areas
connected to the player. `CheckLine` retains the original 1/256-tile two-axis
trace and door-position test, while `CheckSight`, `SightPlayer`, and
`FirstSighting` retain close-range detection, cardinal facing checks, ambush and
noise rules, class-specific randomized reaction delays, chase speeds, and the
first chase frame. Chase direction selection and attacks remain the next actor
slice.

Door codes 90 through 101 are converted in the original scan order into at most
64 runtime door records. Doors begin fully closed, retain orientation and lock
type, occupy the original `0x80 | index` tile value, and mark the adjacent wall
tiles with bit 0x40 for door-jamb texture selection. Every map in both supplied
editions is exercised through this conversion.

The gameplay random generator is the original 256-byte lookup table and byte
index progression rather than a host C library generator. Its state is held in
an engine-owned object so tests and later demo playback can reset it exactly.

## Renderer math

The view layer retains the original 16.16 coordinate unit, 3,600 fine-angle
tangent table, overlapping 360-degree sine/cosine table, focal length, minimum
distance, and per-column ray-angle calculation. Table construction intentionally
keeps the original single-precision angle accumulation and integer operation
order. Negative trigonometric values use portable two's-complement integers;
the DOS source's sign-magnitude encoding existed solely for its assembly
`FixedByFrac` routine.

The original sine-table loop writes one element beyond its declared array when
it reaches 90 degrees. The portable loop stops before that iteration and assigns
the two exact cardinal values explicitly, matching the modern ports without the
out-of-bounds write.

Wall columns are scaled by ordinary bounded C into the indexed framebuffer. The
source sampling order and the original three-bit fractional wall-height unit are
retained from `ScalePost`; a single call may reproduce the original adjacent-post
coalescing optimization without VGA plane masks or generated machine code.

The ray-traversal layer is a direct, fixed-point translation of original
`AsmRefresh`, including its quadrant-specific tangent steps, vertical/horizontal
entry switching, focal-point offset, texture mirroring, and perpendicular height
calculation. Closed and opening doors use their original half-tile planes,
position tests, lock/elevator faces, and adjacent jamb textures. Moving pushwalls
remain the one deliberately rejected wall-hit case rather than being approximated
as fixed geometry.

An owned wall cache decodes every present pre-sprite VSWAP page once into conventional
row-major 64x64 pixels. The initial renderer composes the static traversal and
wall scaler into the original 320x160 play view, using the per-level VGA ceiling
colors and color 0x19 floor from `VGAClearScreen`. A headless `--play-view`
capture makes the real E1M1 result directly inspectable. The static HUD is
composed from the original status-bar, digit, face, key, and weapon pictures by
the translated `WL_AGENT.c` routines. The ready pistol uses a bounded translation
of `SimpleScaleShape` over the original compiled sprite format. Static scenery
uses the original `TransformTile` projection, 50-entry visible-object limit,
ray-traversal `spotvis` admission, far-to-near `DrawScaleds` ordering, and
wall-height tests for per-column occlusion. The headless `--open-doors`
diagnostic exposes the scenery beyond E1M1's initially closed door without
changing normal level state. Ordinary enemies and dead guards share the same
visible-object list; live actors use the original larger `ACTORSIZE` projection
adjustment and select one of eight rotations with `CalcRotate`. Patrol state
updates now feed this same rendering path; awareness and combat AI, boss special
behavior, and moving pushwalls are still being added. The isolated
`--alert-view` diagnostic also advances the selected guard through awareness and
its reaction delay, without enabling incomplete chase logic in normal captures.

Audio will follow the same model: the core will produce PCM through the host
contract. AdLib synthesis will use the upstream Nuked OPL3 implementation,
with its LGPL terms and source separation preserved. The fast fork remains a
measured-performance fallback, not the default.
