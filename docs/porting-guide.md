# Porting wolf3dgeneric to a new host

The engine is a shared C99 library (`wolf3d.dll` on Windows and
`libwolf3d.so` on Linux). A host fills the public
`wolf3d_platform_api_t` callback table from `WOLF3D.h` and passes it to
`wolf3d_SetPlatform` before creating the engine. The library therefore
has no unresolved dependency on symbols supplied by its executable.
`platforms/headless/WG_HEADLESS.c` is the smallest implementation. The SDL3
host is the cross-platform desktop reference, the native Win32 host shows a
dependency-free platform implementation, and `platforms/linux-console/`
demonstrates a complete interactive host without a window system: DRM/KMS
dumb-buffer video, raw evdev input, monotonic POSIX timing, and ALSA PCM.

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
| `pcm_init`, `pcm_shutdown` | Open/close signed 16-bit stereo PCM at the requested sample rate. |
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

## Build integration

Add one executable containing the host and an entry point, link it to the
`wolf3d::wolf3d` shared target, and apply C99 plus strict warnings. Initialize
every callback, set `api_version` to `WOLF3D_PLATFORM_API_VERSION`, set
`struct_size` to `sizeof(wolf3d_platform_api_t)`, and call
`wolf3d_SetPlatform`. Then call `wolf3d_Create`, followed by
`wolf3d_Run`, and always finish with `wolf3d_Shutdown` after
successful creation. The existing Win32 host demonstrates this exact dynamic
library boundary.

Game data is external. Pass its directory through `--data`; never compile or
package the commercial files into a host. A new platform should first reproduce
the headless title/menu hashes, then exercise interactive input and continuous
PCM under its native event loop.
