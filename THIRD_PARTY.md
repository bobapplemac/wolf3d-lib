# Third-party software

## Nuked-OPL3

- Upstream: `nukeykt/Nuked-OPL3`
- Vendored revision: `765ec962e473aeb767e4cba74ffdc8f588ffbfe8`
- License: LGPL-2.1-or-later
- Location: `third_party/Nuked-OPL3`

The official implementation is the project's OPL reference and default
synthesizer. It remains an independently replaceable shared library
(`Nuked-OPL3.dll` on Windows and the corresponding shared object elsewhere).
Binary distributors must satisfy the LGPL's applicable notice, source,
modification, and relinking requirements; consult the included license when
preparing a release.

No MAME or DOSBox OPL implementation is included.

## SDL3

- Upstream: `libsdl-org/SDL`
- Vendored release: `3.4.16`
- License: Zlib
- Location: `third_party/SDL3` (Git submodule)

SDL3 is an optional host-layer dependency used only by the portable GUI
wrapper. It remains a separate shared library in staged SDL3 packages. The
native Win32, Linux direct-console, headless, and engine-library targets do
not require it. Configure with `WG_USE_SYSTEM_SDL3=ON` to use an installed
SDL3 package instead of the pinned submodule.
