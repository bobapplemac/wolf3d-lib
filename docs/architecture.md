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
closed doors until the door reaches its fully open position. Special-boss
movement and player/actor contact remain subsequent slices. As in
`DoActor`, only path states with a `T_Path` thinker move; the short `path1s` and
`path3s` states update animation time without movement.

The awareness slice retains all 37 original area numbers separately from the
collision byte map. An ambush marker (plane-0 tile 106) is cleared and assigned
the neighboring area using `SpawnStand`'s original right/up/down/left overwrite
order. A bounded traversal of the original door-to-area graph derives the areas
connected to the player. `CheckLine` retains the original 1/256-tile two-axis
trace and door-position test, while `CheckSight`, `SightPlayer`, and
`FirstSighting` retain close-range detection, cardinal facing checks, ambush and
noise rules, class-specific randomized reaction delays, chase speeds, and the
first chase frame.

The first chase slice adds the original six-state `10/3/8/10/3/8` animation,
`SelectChaseDir`, `SelectDodgeDir`, `TryWalk` destination reservation, and the
movement portion of `T_Chase` for guards, officers, mutants, SS, Hans, Gretel,
Mecha Hitler, and Hitler. It retains first-attack turnaround permission, random
dodge ordering, cardinal/diagonal collision rules, closed-door waiting,
tile-center correction, and `MINACTORDIST` player separation. The ranged-attack
probability consumes the original random value; class-specific thinkers replace
the earlier explicit pending states.

Ordinary ranged combat retains the guard, officer, mutant, and SS attack state
tables from `WL_ACT2.C`, including their distinct durations, sprite sequences,
and one, two, or four `T_Shoot` actions. The shot calculation preserves area and
line checks, SS distance advantage, visible/running accuracy branches, original
random consumption, and distance-scaled damage. `TakeDamage` owns player health,
baby-mode quarter damage, death state, and accumulated red-flash damage in
`WL_AGENT.c`. Actor flag values now exactly match the original bit layout, and
the renderer maintains `FL_VISABLE` as the DOS `DrawScaleds` pass did.

Dog combat follows its separate original branch: `T_DogChase` always uses
`SelectDodgeDir`, cannot cross doors, and begins the five-state jump when the
next movement step reaches `MINACTORDIST` on both axes. The `10/10/10/10/10`
jump sequence invokes `T_Bite` from its second state, retaining the two-tile
axis bounds, `180/256` hit roll, and `US_RndT() >> 4` damage. Sounds and the
complete restart flow remain subsequent slices.

Player combat in `WL_AGENT.c` retains the four original `attackinfo` tables and
their six-tic frame cadence. `DrawScaleds` stores the projected `viewx` and
`transx` values used by `KnifeAttack` and `GunAttack`; targeting keeps the
original center-screen window, nearest-target choice, wall trace, knife range,
tile-distance hit roll, and random damage divisors. Machine-gun and chaingun
hold behavior loops the same attack-table entries, and empty guns fall back to
the knife. Runtime weapon, ammo, health, and score now feed both the first-person
sprite and status bar. `--player-fire-view` captures the pistol recoil frame
after a deterministic E1M1 shot.

The four hitscan bosses reuse `T_Shoot` with their original state tables. Hans
and Gretel use eight firing states with six shot actions; Mecha-Hitler and
Hitler use six states with five actions. All retain the `30`-tic windup,
alternating second/third firing sprites, `10`-tic burst cadence, and class
return to the first chase state. Hans alone retains the original boss accuracy
and damage-distance advantage.

Schabbs uses his separate `T_Schabb` attack probability and two-state throw
sequence. `T_SchabbThrow` quantizes the player bearing to the original 360-angle
domain and creates a reusable non-blocking syringe actor. Its four six-tic
`SPR_HYPO` frames run `T_Projectile` at speed `0x2000`, using the original
`PROJSIZE` wall box and `PROJECTILESIZE` player box. A player hit deals
`(US_RndT() >> 3) + 20` damage; wall and player impacts remove the projectile.
Removed slots are reused so repeated throws remain bounded by the original
150-actor capacity.

Giftmacher and Fatface share the original `T_GiftThrow` rocket construction and
their close-range `SelectRunDir` retreat behavior. Giftmacher uses a two-state
throw; Fatface follows the throw with four hitscan `T_Shoot` actions. Rockets
use the original eight directional `SPR_ROCKET` views, three-tic smoke action,
`0x2000` speed, and `(US_RndT() >> 3) + 30` player damage. Wall impacts enter
the three six-tic `SPR_BOOM` states, while player impacts remove the rocket.
Four smoke states age independently and reuse the same bounded transient slots.

Fake Hitler keeps his separate `T_Fake` thinker: a clear line consumes the
original `US_RndT() < (tics << 1)` attack roll, while movement always uses
`SelectDodgeDir` and refuses doors. His nine eight-tic firing states invoke
`T_FakeFire` from the first eight states. Each non-rotating two-frame flame is
aimed in the original 360-angle domain, moves at `0x1200`, disappears on a wall
or player impact, and deals the original `US_RndT() >> 3` damage.

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
position tests, lock/elevator faces, and adjacent jamb textures. `PushWall` and
`MovePWalls` retain the original `1/128/256` state thresholds, destination
reservation, obstruction checks, secret accounting, vacated-tile area handoff,
and `pwallpos = (pwallstate / 2) & 63` offset. The vertical and horizontal
pushwall hit paths move the sampled wall plane by that offset before performing
the same texture mirroring and perpendicular-height calculation.

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
updates feed this same rendering path. The isolated
`--alert-view` diagnostic advances the selected guard through awareness and its
reaction delay. `--chase-view` then advances the original chase animation and
movement. `--fire-view` selects the original guard shot action and renders its
resulting attack sprite and health change. `--bite-view` does the same for an
original E1M1 dog's leap and bite. `--boss-fire-view` renders Hans's seeded
hitscan attack on E1M9. `--needle-view` renders Schabbs's syringe in flight on
E2M9, and `--rocket-view` renders Giftmacher's rotating rocket and smoke trail
on E4M9. `--pushwall-view` activates the first E1M1 secret and renders it at a
half-tile offset without altering normal captures. Ordinary guards, officers,
mutants, SS, and dogs now carry the original difficulty-indexed hit points and
run through their pain, death, scoring, and item-drop paths. `--death-view`
captures an E1M1 guard in the third collapse frame with his dropped clip. All
Wolf3D bosses use their original score, item-drop, and death sequences. The
long terminal sequences set victory and level-complete state on successive
death-camera actions; Mecha Hitler instead leaves a corpse and spawns the
independently damageable second phase. `--boss-death-view` captures Hans in the
third collapse frame on E1M9.
Map-authored and enemy-dropped bonuses share a single item type and the
original `GetBonus` inventory rules. Consumed objects remain in the fixed
static array with a removal flag, mirroring the original negative-shape marker
without requiring a signed sprite index. `WL_CollectPlayerTileBonuses` is the
movement-facing collection boundary; the `--pickup-view` checkpoint exercises
it and renders the resulting live score, lives, keys, ammo, and weapon state.
The same player layer now owns original-scale turning and thrust, fixed-radius
collision against walls, closed doors, blocking statics, and live actors, plus
axis-by-axis wall sliding. Cardinal use dispatches to pushwalls, elevators, or
the door state machine. Doors connect areas as soon as opening begins, admit
movement only when fully open, wait 300 tics before closing, and reverse when
the player or an actor obstructs the moving plane. `--door-use-view` renders a
real E1M1 door halfway through its use-triggered opening cycle.

Audio will follow the same model: the core will produce PCM through the host
contract. AdLib synthesis will use the upstream Nuked OPL3 implementation,
with its LGPL terms and source separation preserved. The fast fork remains a
measured-performance fallback, not the default.

The first music slice now follows that design. `ID_SD.c` parses the original
length-prefixed IMF event stream, preserves same-tic zero-delay register
batches and end-of-sequence looping, and services it at 700 Hz. A rational
sample accumulator splits Nuked-OPL3 generation at exact event boundaries; it
therefore produces 700 services over 48,000 output frames without making the
70 Hz gameplay loop run ten times faster. The generic layer submits signed
48 kHz stereo PCM, while Win32 transports it through four reusable `waveOut`
buffers and headless mode can emit the identical samples as a WAV fixture.
AdLib effects share Nuked's channel 0 with the music chip, retain original
priority replacement and instrument programming, and consume one pitch byte at
140 Hz (every fifth IMF service), just as the fast DOS timer ISR did.

Digitized effects retain the original VSWAP layout and `wolfdigimap` selection.
`ID_SD.c` reads the terminal `(start page, byte length)` table, joins each
page-spanning unsigned 8-bit sample, applies the original priority rules and
Sound Blaster Pro 0-15 stereo attenuation, and mixes it into the same signed
48 kHz stream. The zero-order hold advances at 7,042 Hz, matching the effective
rate produced by the original integer DSP time constant for its nominal 7 kHz
configuration. Missing sample pages in the shareware archive fall back to the
corresponding AdLib effect at runtime.
