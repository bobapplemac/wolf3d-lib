# Project history

This repository is a preservation-focused modernization of the original
Wolfenstein 3D source code. It is intentionally a GitHub fork of
[`id-Software/wolf3d`](https://github.com/id-Software/wolf3d), retaining the
three upstream commits and their exact source-release contents.

The immutable tag `upstream-id-software-0516778` identifies id Software's
unmodified repository head, commit
`05167784ef009d0d0daefe8d012b027f39dc8541`. Original installer, binary,
project, documentation, and source artifacts remain available there even when
they are later removed or reorganized on the active branch.

## Curated modernization history

Development initially took place in a private experimental repository, where
the engine was ported, tested, corrected, and split from its operating-system
hosts. The public `main` history is a deliberate reconstruction from the id
Software ancestor. It presents the technical evolution in reviewable stages:

1. preserve the original release and its licensing record;
2. move original runtime files before changing their contents;
3. separate mechanical 16-bit compiler cleanup from behavioral translation;
4. establish checked data, rendering, input, timing, and audio boundaries;
5. restore gameplay and presentation against the original source and data;
6. add deterministic validation and the shared-library API; and
7. add the supported compiler and packaging matrix.

This reconstructed sequence is not represented as the literal chronology of
experimentation. Commit messages use `Reconstructed-From` trailers where a
private checkpoint materially informed the public step.

The exact private chronology is preserved on the public branch
`archive/gitlab-development-history`. Its final engine checkpoint is
wolf3d-lib 1.4.52 at
`79cac7556771bb59638ac50183ccb3db3c17371e`.

## Naming and lineage

Original descendants retain their historical `ID_` and `WL_` filenames
wherever practical. Lowercase extensions allow modern tools to recognize C
sources normally. The `WG_` prefix means “Wolf3D Generic” and identifies new
internal portability interfaces or subsystems extracted from multiple
original files. The stable public API uses `wolf3d_` functions and `WOLF3D_`
types and constants.

Commercial game data is not included. Users must supply compatible original
WL1, WL6, SDM, SOD, SD1, SD2, or SD3 data files.
