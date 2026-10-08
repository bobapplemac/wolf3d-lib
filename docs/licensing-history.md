# Licensing history

Wolfenstein 3D's source has two visible licensing eras. This repository keeps
evidence of both rather than silently replacing the historical notice.

## Original source release

The original DOS source was distributed under id Software's Limited Use
Software License Agreement. The untouched text remains at
[`LICENSES/id-original-limited-use.txt`](../LICENSES/id-original-limited-use.txt)
and in its original location at the immutable
`upstream-id-software-0516778` tag. The upstream id Software repository still
publishes that release and notice:

- <https://github.com/id-Software/wolf3d>

That license is historically important and continues to describe the original
snapshot as it was first published.

## Later GPL release

John Carmack subsequently described the original Wolfenstein 3D source as
having been released first under a non-commercial license and “then later
under the GPL.” That statement appears in id Software's official source-release
materials:

- <https://github.com/id-Software/DOOM-IOS2/blob/master/readme_iDoom.txt>
- <https://github.com/id-Software/Wolf3D-iOS/blob/master/wolf3d/README.txt>
- <https://github.com/id-Software/Wolf3D-iOS>

The Wolf3D browser release is another official id Software publication that
labels its engine source as GPL:

- <https://github.com/id-Software/wolf3d-browser>

On that basis, and consistently with longstanding Wolfenstein source-port
practice, the active wolf3d-lib code and project modifications are distributed
under GNU GPL version 2. The complete terms are at [`LICENSE`](../LICENSE) and
[`LICENSES/GPL-2.0.txt`](../LICENSES/GPL-2.0.txt).

This document records provenance rather than offering legal advice. It does
not relicense proprietary game data, trademarks, artwork, music, or other
content. The embedded SIGNON templates are original game artwork retained for
faithful executable presentation and are explicitly outside the source-code
license grant.

## Third-party components

Third-party components retain their own notices. In particular:

- Nuked-OPL3 is LGPL-2.1-or-later and is built as an independently replaceable
  library in the default shared-library distribution.
- DBOPL is the GPL-compatible C conversion carried by PrBoom+ from DOSBox
  DBOPL revision 3635.

The final source tree's [`THIRD_PARTY.md`](../THIRD_PARTY.md) and component
license files provide exact versions and attribution.
