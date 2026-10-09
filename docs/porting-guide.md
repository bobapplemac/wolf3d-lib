# Porting wolf3d-lib to a new host

The engine is a shared C99 library (`wolf3d.dll` on Windows and
`libwolf3d.so` on Linux). A host fills the public
`wolf3d_platform_api_t` callback table from `WOLF3D.h` and passes it to
`wolf3d_SetPlatform` before creating the engine. The library therefore
has no unresolved dependency on symbols supplied by its executable.
`platforms/headless/WG_HEADLESS.c` is the smallest internal implementation.
The companion `wolf3d-portable` repository contains production SDL3, native
Win32, and Linux direct-console examples using only this public boundary.

## Required boundary

| Callback | Host responsibility |
| --- | --- |
| `init`, `shutdown` | Create and release host state. Return nonzero from init on success. |
| `present` | Present exactly 320x200 indexed pixels using the supplied 256-entry RGB palette. Nearest-neighbor 4:3 scaling preserves the intended image. |
| `get_ticks_ms` | Return wrapping, monotonic 32-bit milliseconds. |
| `sleep_ms` | Yield for approximately the requested duration; game timing does not assume exact sleeps. |
| `poll_event` | Pop one event without blocking and return nonzero, or return zero when the queue is empty. |
| `is_interactive` | Return nonzero for a live host and zero for deterministic/offline tools. |
| `set_window_title` | Publish the current game/profile name in the host's normal way. |
| `print_message`, `report_error` | Write ordinary console/debug text and report errors. `report_error` may additionally use a native dialog. |
| `present_text` | Preserve an 80x25 DOS text-mode screen supplied as interleaved CP437 character/VGA-attribute bytes. Native console cells are ideal; ANSI color plus CP437-to-Unicode conversion is the portable fallback. Copy the cells if presentation is deferred until shutdown. |
| `pcm_init`, `pcm_init_ex`, `pcm_shutdown` | Open/close signed 16-bit stereo PCM. `pcm_init_ex` reports the application-facing rate actually obtained; retain `pcm_init` as the compatibility callback. |
| `pcm_writable_frames` | Report how many frames can be submitted immediately without blocking. |
| `pcm_submit` | Queue interleaved signed 16-bit frames and return nonzero on success. |

## Input contract

Keyboard events use IBM PC set-1 scan codes, not native virtual-key values.
Common codes are named in `include/WOLF3D.h`; other bindable keys may be
passed as their 0–127 set-1 value. Emit both press and release events. Map a
physical Pause key to `WOLF3D_KEY_PAUSE` because its hardware sequence is not a
normal one-byte scan code.

Mouse motion is relative. Mouse buttons are numbered 1 (left), 2 (right), and
3 (middle), with press and release events.

Joystick hosts emit `WOLF3D_EVENT_JOYSTICK` whenever one of the first two devices
connects, disconnects, or changes state. `joystick` is zero or one, `connected`
states whether that slot is available, `x` and `y` span `INT16_MIN` through
`INT16_MAX` with negative Y meaning up, and the low four bits of `buttons`
represent buttons 0 through 3. Send a disconnected event with centered axes and
no buttons when a device disappears. The core applies `ID_IN.C`'s calibrated
outer-third scaling and the game's additional 64-unit movement threshold; hosts
must not add a second dead zone unless required by their device API.

## Video and audio ownership

The pixel and palette pointers passed to `WG_Present` remain engine-owned; copy
them if presentation is asynchronous. The engine may change the palette without
changing pixel indices, notably for damage and bonus flashes.

The cell pointer passed to `WG_PresentText` is likewise temporary. This
callback is made before platform shutdown so a graphical host can close its
display, restore the terminal, and then emit the original colored DOS quit or
error screen. Attribute bits 0--3 are the VGA foreground, bits 4--6 are the
background, and bit 7 is blink. Hosts that cannot represent color should still
emit the CP437 text in a readable encoding.

`WG_PCMWritableFrames` may return zero. The core will try again on a later loop
iteration. `WG_PCMSubmit` must copy or consume the supplied samples before it
returns because the buffer is temporary. Audio rendering owns the 700 Hz IMF
clock and 140 Hz effect clock; a host must not derive either from video or the
70 Hz game loop.

The engine requests the configured preferred rate (48 kHz by default), then
constructs the active OPL driver at the rate returned by `pcm_init_ex`. A host
that performs transparent device-side resampling should report the rate at
which it accepts application samples, normally the requested rate. A direct
hardware host may instead report a nearby negotiated rate. The engine's
rational 700 Hz sample clock remains exact at either rate.

Platform API v6 also has three optional native-OPL callbacks. A constrained
hardware host may initialize an OPL2-compatible device, accept register/value
writes, and shut it down without putting port I/O in the generic engine. The
`adlib` adapter is compiled only by an explicitly configured target. Its
`generate` step emits a silent FM bed while the normal renderer advances the
700 Hz IMF clock and mixes digitized/PC-speaker effects into host PCM. Hosts
that do not provide native OPL should set all three callbacks to `NULL`.

The optional `input_devices` callback returns the bitwise combination of
`WOLF3D_INPUT_DEVICE_MOUSE` and `WOLF3D_INPUT_DEVICE_JOYSTICK` detected after
host initialization. Return zero (or use `NULL`) when discovery is unavailable.
Command-line force-on and force-off options take precedence over this initial
snapshot; later joystick connection changes use `WOLF3D_EVENT_JOYSTICK`.
The optional `set_mouse_capture` callback confines and hides the native cursor
when its argument is nonzero, and releases and shows it otherwise. The engine
calls it after startup configuration, whenever Mouse Enabled changes, and
before platform shutdown.

## Build integration

Add one executable containing the host and an entry point, link it to the
`wolf3d::wolf3d` shared target, and apply C99 plus strict warnings. Initialize
every callback, set `api_version` to `WOLF3D_PLATFORM_API_VERSION`, set
`struct_size` to `sizeof(wolf3d_platform_api_t)`, and call
`wolf3d_SetPlatform`. Then call `wolf3d_Create`, followed by
`wolf3d_Run`, and always finish with `wolf3d_Shutdown` after
successful creation. The companion hosts demonstrate this exact dynamic
library boundary and build the library from their selected submodule checkout.

Game data is external. Pass its directory through `--data`; never compile or
package the commercial files into a host. A new platform should first reproduce
the headless title/menu hashes, then exercise interactive input and continuous
PCM under its native event loop.

## Lifecycle and runtime choices

Hosts include `WOLF3D.h`, fill a `wolf3d_platform_api_t`, then call:

1. `wolf3d_SetPlatform()`
2. `wolf3d_Create()`
3. `wolf3d_Run()`
4. `wolf3d_Shutdown()`

The library also exports read-only OPL-driver discovery functions plus the
indexed 320x200 `wolf3d_ScreenBuffer` and 256-color `wolf3d_Palette`. Default modern
builds include Nuked-OPL3, pure-C DBOPL, and timing-preserving silent drivers;
`--opl nuked|dbopl|silent` selects among the compiled choices at runtime.
Nuked-OPL3 remains a separate, replaceable LGPL shared library.

The platform table also exposes optional native-OPL lifecycle and
register-write callbacks. They support constrained hardware hosts without
adding raw I/O to the portable engine; ordinary Windows and Linux builds leave
them unset and do not compile the `adlib` adapter.

The in-tree `wolf3d::wolf3d` CMake target is the supported target for a
parent project. The public header is under `include/`; private `src/`
headers are not a host interface.

Runtime audio selection is intentionally host-independent:

```text
--opl nuked|dbopl|silent  Select an implementation compiled into the library
--sample-rate HZ          Request 8000--192000 Hz PCM (default: 48000)
--adlib / -nosb           Emulate an AdLib-only machine
--pc-speaker / -noal      Emulate no sound card; default to PC-speaker effects
--no-sound                Emulate no sound card and select no audio
```

The host may negotiate another application-facing PCM rate. The library then
constructs the selected emulator at that obtained rate while preserving the
exact rational 700 Hz IMF clock and 140 Hz effect clock.
Hardware profiles are independent of the selected OPL implementation and of
whether the host opens a physical audio device.
