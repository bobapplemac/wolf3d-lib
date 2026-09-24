# Architecture

wolf3dgeneric keeps the original game's indexed 320x200 presentation and game
rules while replacing assumptions that only hold in 16-bit DOS. The portable
core is C99 and communicates with a host through the deliberately small
`wg_platform.h` contract.

## Resource boundary

The resource modules parse the original, unmodified data files into explicitly
sized host-memory objects:

- `wg_graphics`: VGAHEAD/VGADICT/VGAGRAPH Huffman chunks, picture dimensions,
  and planar-to-chunky picture conversion.
- `wg_maps`: MAPHEAD/GAMEMAPS headers and Carmack plus RLEW map-plane decoding.
- `wg_pages`: bounds-checked VSWAP wall, sprite, and digitized-sound pages.
- `wg_audio`: AUDIOHED/AUDIOT chunk lookup for PC speaker, AdLib, digitized
  sound metadata, and IMF music.

The file formats are read byte-by-byte with little-endian helpers. No compiler
packing, native pointer width, or unaligned host access is part of the format
contract. Malformed offsets and compressed streams fail at the resource
boundary instead of propagating unchecked pointers into game code.

## Presentation boundary

The core owns one 320x200 byte-per-pixel framebuffer and a 256-entry RGB
palette. A host presents that pair, reports input events, supplies monotonic
time and sleeping, and reports fatal errors. The Win32 reference host uses GDI;
the headless host supplies deterministic virtual time for tests.

Audio will follow the same model: the core will produce PCM through the host
contract. AdLib synthesis will use the upstream Nuked OPL3 implementation,
with its LGPL terms and source separation preserved. The fast fork remains a
measured-performance fallback, not the default.
