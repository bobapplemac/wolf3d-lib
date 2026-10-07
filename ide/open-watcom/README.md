# Open Watcom IDE project

`wolf3d-lib.wpj` is the native Open Watcom IDE workspace for the 32-bit DOS
library. Run `open-ide.cmd` from a configured Open Watcom command prompt. The
launcher supplies the shared include paths and the same all-driver compile
definitions used by the default Docker build.

For the period-compatible release profile, select **Pentium register-based
calling** under the C compiler's **Memory Model and Processor** settings and
the IDE's release switch set. Open Watcom otherwise defaults a new DOS32 target
to Pentium Pro; the Docker release path always enforces the Pentium baseline.

The workspace has separate `WOLF3D.LIB` and `NUKEDOPL.LIB` targets. This keeps
Nuked-OPL3 independently replaceable for LGPL relinking. The native AdLib
adapter is part of `WOLF3D.LIB`, but it only calls host callbacks and performs
no port I/O itself.

The `.wpj` and `.tgt` formats are serialized IDE state and should not be edited
by hand. Regenerate them after changing the canonical source inventory with:

```text
python tools\WG_GENERATE_OPENWATCOM_IDE.py
```

The generated descriptors are checked with Open Watcom's own `ide2make`
utility during maintainer validation. The Docker/Make build remains the
reproducible release path; this workspace is the first-class native IDE path.
