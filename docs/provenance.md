# Source provenance

The implementation follows the source priority and lookup rules in
[`reference-index.md`](reference-index.md).

The authoritative gameplay source is id Software's Wolfenstein 3D release at
commit `05167784ef009d0d0daefe8d012b027f39dc8541`. Portable translations may
consult Wolf4SDL and Chocolate Wolfenstein 3D; copied or adapted routines must
retain applicable notices and remain compatible with GPL-2.0-only.

Nuked-OPL3 will be maintained as a separately identified LGPL-2.1-or-later
third-party component when audio implementation begins. Its source and license
will accompany distributed builds. The MAME and DOSBox OPL implementations are
not used.

`src/wolfpal.inc` is the original Wolfenstein 3D 256-color VGA palette in the
portable initializer format used by Chocolate Wolfenstein 3D. Its 6-bit channel
values are converted to 8-bit values at compile time.

The resource readers follow the original `ID_CA.C`, `ID_PM.C`, and generated
graphics/audio headers. Wolf4SDL's `id_ca.cpp` and `id_pm.cpp` were used to
cross-check intended behavior after removing segmented-memory and stdio
assumptions. The implementations here are new, bounds-checked C99 code rather
than copied port code.

Wolfenstein 3D data and executables are external test inputs. No game assets are
part of this repository or covered by its license.
