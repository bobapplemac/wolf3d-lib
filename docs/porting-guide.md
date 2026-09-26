# Porting wolf3dgeneric to a new host

The engine is a static C99 library. A host supplies the functions declared in
`src/WG_PLATFORM.h`; it does not need to expose an operating-system object to
the core. `platforms/headless/WG_HEADLESS.c` is the smallest implementation and
`platforms/win32/WG_WIN32.c` is the complete interactive reference.

## Required boundary

| Function | Host responsibility |
| --- | --- |
| `WG_Init`, `WG_Shutdown` | Create and release host state. Return nonzero from init on success. |
| `WG_Present` | Present exactly 320x200 indexed pixels using the supplied 256-entry RGB palette. Nearest-neighbor 4:3 scaling preserves the intended image. |
| `WG_GetTicksMs` | Return wrapping, monotonic 32-bit milliseconds. |
| `WG_SleepMs` | Yield for approximately the requested duration; game timing does not assume exact sleeps. |
| `WG_PollEvent` | Pop one event without blocking and return nonzero, or return zero when the queue is empty. |
| `WG_IsInteractive` | Return nonzero for a live host and zero for deterministic/offline tools. |
| `WG_SetWindowTitle`, `WG_ReportError` | Publish user-facing status and errors in the host's normal way. |
| `WG_PCMInit`, `WG_PCMShutdown` | Open/close signed 16-bit stereo PCM at the requested sample rate. |
| `WG_PCMWritableFrames` | Report how many frames can be submitted immediately without blocking. |
| `WG_PCMSubmit` | Queue interleaved signed 16-bit frames and return nonzero on success. |

## Input contract

Keyboard events use IBM PC set-1 scan codes, not native virtual-key values.
Common codes are named in `include/WOLF3DGENERIC.h`; other bindable keys may be
passed as their 0–127 set-1 value. Emit both press and release events. Map a
physical Pause key to `WG_KEY_PAUSE` because its hardware sequence is not a
normal one-byte scan code.

Mouse motion is relative. Mouse buttons are numbered 1 (left), 2 (right), and
3 (middle), with press and release events. The current generic boundary
deliberately contains no joystick event because joystick support is outside the
Wolf3D v1.4 first-release host scope.

## Video and audio ownership

The pixel and palette pointers passed to `WG_Present` remain engine-owned; copy
them if presentation is asynchronous. The engine may change the palette without
changing pixel indices, notably for damage and bonus flashes.

`WG_PCMWritableFrames` may return zero. The core will try again on a later loop
iteration. `WG_PCMSubmit` must copy or consume the supplied samples before it
returns because the buffer is temporary. Audio rendering owns the 700 Hz IMF
clock and 140 Hz effect clock; a host must not derive either from video or the
70 Hz game loop.

## Build integration

Add one executable containing the host and an entry point, link it to
`wolf3dgeneric`, and apply C99 plus strict warnings. The existing
`wg_configure_host` CMake helper demonstrates the intended setup. Call
`wolf3dgeneric_Create`, then `wolf3dgeneric_Run`, and always finish with
`wolf3dgeneric_Shutdown` after successful creation.

Game data is external. Pass its directory through `--data`; never compile or
package the commercial files into a host. A new platform should first reproduce
the headless title/menu hashes, then exercise interactive input and continuous
PCM under its native event loop.
