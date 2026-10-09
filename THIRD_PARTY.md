# Third-party software

## Nuked-OPL3

- Upstream: https://github.com/nukeykt/Nuked-OPL3
- License: LGPL-2.1-or-later
- Location: `third_party/Nuked-OPL3`

Nuked-OPL3 is the project's reference OPL implementation. It is intentionally
built and distributed as a separate replaceable shared library so recipients
can exercise the LGPL relinking/replacement rights. Its upstream license is
included as `DOCS/LICENSES/LGPL-21.TXT` in staged packages.

## DBOPL C port

- Upstream: https://github.com/coelckers/prboom-plus
- Source revision: `969515162c5aebea4ad7a125ee178dcad3d576ad`
- DOSBox DBOPL revision identified by upstream: r3635
- License: GPL-2.0-or-later
- Location: `third_party/DBOPL`

PrBoom+ converted DOSBox's DBOPL implementation from C++ to C using Chocolate
Doom's `minus-minus` script and manual corrections. wolf3d-lib removes its
incidental SDL fixed-width-type dependency and exposes it only through the
private OPL adapter. DBOPL is an optional lower-resource backend; Nuked-OPL3
remains the preservation reference and default.

Wolfenstein 3D game data, SIGNON source artwork, the original DOS source,
Chocolate Wolfenstein, and other comparison repositories are development
references only. They are not vendored or redistributed by wolf3d-lib.
