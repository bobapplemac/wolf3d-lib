#include "WOLF3DGENERIC.h"
#include "../WG_HOST.h"
#include "../WG_TEXT_OUTPUT.h"
#include "WG_LINUX_CONSOLE.h"

#include <alsa/asoundlib.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/kd.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#define WG_PATH_CAPACITY 512U
#define WG_INPUT_CAPACITY 32U
#define WG_EVENT_QUEUE_CAPACITY 128U
#define WG_BITS_PER_LONG (sizeof(unsigned long) * 8U)
#define WG_BIT_WORD(bit) ((unsigned)(bit) / WG_BITS_PER_LONG)
#define WG_BIT_MASK(bit) (1UL << ((unsigned)(bit) % WG_BITS_PER_LONG))
#define WG_BIT_ARRAY_SIZE(maximum) (WG_BIT_WORD(maximum) + 1U)

typedef struct wg_input_device
{
    int descriptor;
    int grabbed;
    int keyboard;
    int mouse;
    int joystick;
    uint8_t joystick_index;
    int mouse_x;
    int mouse_y;
    int16_t joystick_x;
    int16_t joystick_y;
    uint32_t joystick_buttons;
    int joystick_dirty;
} wg_input_device_t;

typedef struct wg_drm_state
{
    int descriptor;
    int master;
    uint32_t connector_id;
    uint32_t crtc_id;
    drmModeModeInfo mode;
    drmModeCrtc *old_crtc;
    uint32_t framebuffer_id;
    uint32_t handle;
    uint32_t pitch;
    uint64_t size;
    uint32_t *pixels;
    int presentation_left;
    int presentation_top;
    int presentation_width;
    int presentation_height;
    uint16_t *source_x;
    uint16_t *source_y;
} wg_drm_state_t;

static char wg_drm_device[WG_PATH_CAPACITY];
static char wg_alsa_device[WG_PATH_CAPACITY] = "default";
static char wg_requested_inputs[WG_INPUT_CAPACITY][WG_PATH_CAPACITY];
static size_t wg_requested_input_count;
static int wg_audio_disabled;

static wg_drm_state_t wg_drm =
{
    .descriptor = -1,
    .pixels = MAP_FAILED
};
static wg_input_device_t wg_inputs[WG_INPUT_CAPACITY];
static size_t wg_input_count;
static int wg_console_descriptor = -1;
static int wg_console_mode = KD_TEXT;
static int wg_console_graphics;
static struct timespec wg_clock_start;
static volatile sig_atomic_t wg_quit_requested;
static int wg_quit_delivered;
static snd_pcm_t *wg_pcm;
static uint8_t wg_previous_pixels[WG_SCREEN_WIDTH * WG_SCREEN_HEIGHT];
static uint8_t wg_previous_palette[WG_PALETTE_COLORS * 3U];
static int wg_previous_frame_valid;
static uint8_t wg_text_screen[WG_TEXT_COLUMNS * WG_TEXT_ROWS
                              * WG_TEXT_CELL_BYTES];
static uint16_t wg_text_columns;
static uint16_t wg_text_rows;

static wg_event_t wg_event_queue[WG_EVENT_QUEUE_CAPACITY];
static size_t wg_event_read;
static size_t wg_event_write;

static void WG_CloseDRM(void);

static int WG_CopyOption(char *destination, const char *source)
{
    size_t length;

    if (source == NULL)
    {
        return 0;
    }
    length = strlen(source);
    if (length == 0U || length >= WG_PATH_CAPACITY)
    {
        return 0;
    }
    memcpy(destination, source, length + 1U);
    return 1;
}

int WG_LinuxConsoleSetDRMDevice(const char *path)
{
    return WG_CopyOption(wg_drm_device, path);
}

int WG_LinuxConsoleAddInputDevice(const char *path)
{
    if (wg_requested_input_count >= WG_INPUT_CAPACITY
        || !WG_CopyOption(wg_requested_inputs[wg_requested_input_count], path))
    {
        return 0;
    }
    ++wg_requested_input_count;
    return 1;
}

int WG_LinuxConsoleSetALSADevice(const char *name)
{
    return WG_CopyOption(wg_alsa_device, name);
}

void WG_LinuxConsoleDisableAudio(void)
{
    wg_audio_disabled = 1;
}

static int WG_TestBit(const unsigned long *bits, unsigned bit)
{
    return (bits[WG_BIT_WORD(bit)] & WG_BIT_MASK(bit)) != 0UL;
}

static void WG_QueueEvent(const wg_event_t *event)
{
    size_t next = (wg_event_write + 1U) % WG_EVENT_QUEUE_CAPACITY;

    if (next != wg_event_read)
    {
        wg_event_queue[wg_event_write] = *event;
        wg_event_write = next;
    }
}

static uint16_t WG_LinuxKey(unsigned code)
{
    switch (code)
    {
        case KEY_ESC: return 0x01;
        case KEY_1: return 0x02; case KEY_2: return 0x03;
        case KEY_3: return 0x04; case KEY_4: return 0x05;
        case KEY_5: return 0x06; case KEY_6: return 0x07;
        case KEY_7: return 0x08; case KEY_8: return 0x09;
        case KEY_9: return 0x0a; case KEY_0: return 0x0b;
        case KEY_MINUS: return 0x0c; case KEY_EQUAL: return 0x0d;
        case KEY_BACKSPACE: return 0x0e; case KEY_TAB: return 0x0f;
        case KEY_Q: return 0x10; case KEY_W: return 0x11;
        case KEY_E: return 0x12; case KEY_R: return 0x13;
        case KEY_T: return 0x14; case KEY_Y: return 0x15;
        case KEY_U: return 0x16; case KEY_I: return 0x17;
        case KEY_O: return 0x18; case KEY_P: return 0x19;
        case KEY_LEFTBRACE: return 0x1a; case KEY_RIGHTBRACE: return 0x1b;
        case KEY_ENTER: case KEY_KPENTER: return 0x1c;
        case KEY_LEFTCTRL: case KEY_RIGHTCTRL: return 0x1d;
        case KEY_A: return 0x1e; case KEY_S: return 0x1f;
        case KEY_D: return 0x20; case KEY_F: return 0x21;
        case KEY_G: return 0x22; case KEY_H: return 0x23;
        case KEY_J: return 0x24; case KEY_K: return 0x25;
        case KEY_L: return 0x26; case KEY_SEMICOLON: return 0x27;
        case KEY_APOSTROPHE: return 0x28; case KEY_GRAVE: return 0x29;
        case KEY_LEFTSHIFT: return 0x2a; case KEY_BACKSLASH: return 0x2b;
        case KEY_Z: return 0x2c; case KEY_X: return 0x2d;
        case KEY_C: return 0x2e; case KEY_V: return 0x2f;
        case KEY_B: return 0x30; case KEY_N: return 0x31;
        case KEY_M: return 0x32; case KEY_COMMA: return 0x33;
        case KEY_DOT: return 0x34; case KEY_SLASH: return 0x35;
        case KEY_RIGHTSHIFT: return 0x36; case KEY_KPASTERISK: return 0x37;
        case KEY_LEFTALT: case KEY_RIGHTALT: return 0x38;
        case KEY_SPACE: return 0x39; case KEY_CAPSLOCK: return 0x3a;
        case KEY_F1: return 0x3b; case KEY_F2: return 0x3c;
        case KEY_F3: return 0x3d; case KEY_F4: return 0x3e;
        case KEY_F5: return 0x3f; case KEY_F6: return 0x40;
        case KEY_F7: return 0x41; case KEY_F8: return 0x42;
        case KEY_F9: return 0x43; case KEY_F10: return 0x44;
        case KEY_NUMLOCK: return 0x45; case KEY_SCROLLLOCK: return 0x46;
        case KEY_KP7: case KEY_HOME: return 0x47;
        case KEY_KP8: case KEY_UP: return 0x48;
        case KEY_KP9: case KEY_PAGEUP: return 0x49;
        case KEY_KPMINUS: return 0x4a;
        case KEY_KP4: case KEY_LEFT: return 0x4b;
        case KEY_KP5: return 0x4c;
        case KEY_KP6: case KEY_RIGHT: return 0x4d;
        case KEY_KPPLUS: return 0x4e;
        case KEY_KP1: case KEY_END: return 0x4f;
        case KEY_KP2: case KEY_DOWN: return 0x50;
        case KEY_KP3: case KEY_PAGEDOWN: return 0x51;
        case KEY_KP0: case KEY_INSERT: return 0x52;
        case KEY_KPDOT: case KEY_DELETE: return 0x53;
        case KEY_F11: return 0x57; case KEY_F12: return 0x58;
        case KEY_PAUSE: return WG_KEY_PAUSE;
        default: return 0;
    }
}

static int16_t WG_ScaleAbsolute(int value, const struct input_absinfo *info)
{
    int64_t range;
    int64_t scaled;

    if (info == NULL || info->maximum <= info->minimum)
    {
        return 0;
    }
    range = (int64_t)info->maximum - info->minimum;
    scaled = ((int64_t)(value - info->minimum) * 65535) / range - 32768;
    if (scaled < INT16_MIN)
    {
        scaled = INT16_MIN;
    }
    if (scaled > INT16_MAX)
    {
        scaled = INT16_MAX;
    }
    return (int16_t)scaled;
}

static void WG_QueueJoystick(wg_input_device_t *device)
{
    wg_event_t event = { 0 };

    event.type = WG_EVENT_JOYSTICK;
    event.joystick = device->joystick_index;
    event.connected = 1U;
    event.x = device->joystick_x;
    event.y = device->joystick_y;
    event.buttons = device->joystick_buttons;
    WG_QueueEvent(&event);
    device->joystick_dirty = 0;
}

static unsigned WG_JoystickButton(unsigned code)
{
    switch (code)
    {
        case BTN_SOUTH: return 0U;
        case BTN_EAST: return 1U;
        case BTN_WEST: return 2U;
        case BTN_NORTH: return 3U;
        default: return UINT_MAX;
    }
}

static void WG_ProcessInputEvent(wg_input_device_t *device,
                                 const struct input_event *source)
{
    if (source->type == EV_KEY && device->keyboard && source->value != 2)
    {
        uint16_t key = WG_LinuxKey(source->code);

        if (key != 0U)
        {
            wg_event_t event = { 0 };

            event.type = WG_EVENT_KEY;
            event.pressed = source->value != 0;
            event.key = key;
            WG_QueueEvent(&event);
        }
    }
    if (source->type == EV_KEY && device->mouse && source->value != 2)
    {
        uint8_t button = 0xffU;

        if (source->code == BTN_LEFT) button = 1U;
        else if (source->code == BTN_RIGHT) button = 2U;
        else if (source->code == BTN_MIDDLE) button = 3U;
        if (button != 0xffU)
        {
            wg_event_t event = { 0 };

            event.type = WG_EVENT_MOUSE_BUTTON;
            event.pressed = source->value != 0;
            event.button = button;
            WG_QueueEvent(&event);
        }
    }
    if (source->type == EV_REL && device->mouse)
    {
        if (source->code == REL_X) device->mouse_x += source->value;
        if (source->code == REL_Y) device->mouse_y += source->value;
    }
    if (source->type == EV_ABS && device->joystick
        && (source->code == ABS_X || source->code == ABS_Y
            || source->code == ABS_HAT0X || source->code == ABS_HAT0Y))
    {
        struct input_absinfo info;

        if (ioctl(device->descriptor, EVIOCGABS(source->code), &info) == 0)
        {
            int16_t value = WG_ScaleAbsolute(source->value, &info);

            if (source->code == ABS_X || source->code == ABS_HAT0X)
            {
                device->joystick_x = value;
            }
            else
            {
                device->joystick_y = value;
            }
            device->joystick_dirty = 1;
        }
    }
    if (source->type == EV_KEY && device->joystick && source->value != 2
        && source->code >= BTN_GAMEPAD && source->code <= BTN_THUMBR)
    {
        unsigned button = WG_JoystickButton(source->code);

        if (button < 32U)
        {
            if (source->value != 0)
            {
                device->joystick_buttons |= 1U << button;
            }
            else
            {
                device->joystick_buttons &= ~(1U << button);
            }
            device->joystick_dirty = 1;
        }
    }
    if (source->type == EV_SYN && source->code == SYN_REPORT)
    {
        if (device->mouse_x != 0 || device->mouse_y != 0)
        {
            wg_event_t event = { 0 };

            event.type = WG_EVENT_MOUSE_MOTION;
            event.x = (int16_t)(device->mouse_x < INT16_MIN ? INT16_MIN
                                : device->mouse_x > INT16_MAX ? INT16_MAX
                                : device->mouse_x);
            event.y = (int16_t)(device->mouse_y < INT16_MIN ? INT16_MIN
                                : device->mouse_y > INT16_MAX ? INT16_MAX
                                : device->mouse_y);
            WG_QueueEvent(&event);
            device->mouse_x = 0;
            device->mouse_y = 0;
        }
        if (device->joystick_dirty)
        {
            WG_QueueJoystick(device);
        }
    }
}

static int WG_OpenInput(const char *path)
{
    unsigned long event_bits[WG_BIT_ARRAY_SIZE(EV_MAX)] = { 0 };
    unsigned long key_bits[WG_BIT_ARRAY_SIZE(KEY_MAX)] = { 0 };
    unsigned long relative_bits[WG_BIT_ARRAY_SIZE(REL_MAX)] = { 0 };
    unsigned long absolute_bits[WG_BIT_ARRAY_SIZE(ABS_MAX)] = { 0 };
    wg_input_device_t device;
    int descriptor;

    if (wg_input_count >= WG_INPUT_CAPACITY)
    {
        return 0;
    }
    descriptor = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (descriptor < 0
        || ioctl(descriptor, EVIOCGBIT(0, sizeof(event_bits)), event_bits) < 0)
    {
        if (descriptor >= 0) close(descriptor);
        return 0;
    }
    if (WG_TestBit(event_bits, EV_KEY))
    {
        (void)ioctl(descriptor, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits);
    }
    if (WG_TestBit(event_bits, EV_REL))
    {
        (void)ioctl(descriptor, EVIOCGBIT(EV_REL, sizeof(relative_bits)),
                    relative_bits);
    }
    if (WG_TestBit(event_bits, EV_ABS))
    {
        (void)ioctl(descriptor, EVIOCGBIT(EV_ABS, sizeof(absolute_bits)),
                    absolute_bits);
    }

    memset(&device, 0, sizeof(device));
    device.descriptor = descriptor;
    device.keyboard = WG_TestBit(key_bits, KEY_ESC)
                   && WG_TestBit(key_bits, KEY_ENTER);
    device.mouse = WG_TestBit(relative_bits, REL_X)
                && WG_TestBit(relative_bits, REL_Y)
                && WG_TestBit(key_bits, BTN_LEFT);
    device.joystick = WG_TestBit(absolute_bits, ABS_X)
                   && WG_TestBit(absolute_bits, ABS_Y)
                   && WG_TestBit(key_bits, BTN_GAMEPAD);
    if (!device.keyboard && !device.mouse && !device.joystick)
    {
        close(descriptor);
        return 0;
    }
    if (device.joystick)
    {
        size_t index;
        uint8_t joystick_index = 0U;

        for (index = 0U; index < wg_input_count; ++index)
        {
            if (wg_inputs[index].joystick) ++joystick_index;
        }
        if (joystick_index >= WG_MAX_JOYSTICKS)
        {
            device.joystick = 0;
        }
        else
        {
            struct input_absinfo info;

            device.joystick_index = joystick_index;
            if (ioctl(descriptor, EVIOCGABS(ABS_X), &info) == 0)
            {
                device.joystick_x = WG_ScaleAbsolute(info.value, &info);
            }
            if (ioctl(descriptor, EVIOCGABS(ABS_Y), &info) == 0)
            {
                device.joystick_y = WG_ScaleAbsolute(info.value, &info);
            }
        }
    }
    device.grabbed = ioctl(descriptor, EVIOCGRAB, 1) == 0;
    wg_inputs[wg_input_count++] = device;
    if (device.joystick)
    {
        WG_QueueJoystick(&wg_inputs[wg_input_count - 1U]);
    }
    return device.keyboard;
}

static int WG_OpenInputs(void)
{
    size_t index;
    int keyboards = 0;

    if (wg_requested_input_count != 0U)
    {
        for (index = 0U; index < wg_requested_input_count; ++index)
        {
            keyboards += WG_OpenInput(wg_requested_inputs[index]);
        }
    }
    else
    {
        unsigned number;

        for (number = 0U; number < 64U; ++number)
        {
            char path[64];

            (void)snprintf(path, sizeof(path), "/dev/input/event%u", number);
            keyboards += WG_OpenInput(path);
        }
    }
    if (keyboards == 0)
    {
        fprintf(stderr, "wolf3dgeneric: no usable evdev keyboard found.\n");
        return 0;
    }
    return 1;
}

static void WG_CloseInputs(void)
{
    size_t index;

    for (index = 0U; index < wg_input_count; ++index)
    {
        if (wg_inputs[index].joystick)
        {
            wg_event_t event = { 0 };

            event.type = WG_EVENT_JOYSTICK;
            event.joystick = wg_inputs[index].joystick_index;
            WG_QueueEvent(&event);
        }
        if (wg_inputs[index].grabbed)
        {
            (void)ioctl(wg_inputs[index].descriptor, EVIOCGRAB, 0);
        }
        close(wg_inputs[index].descriptor);
    }
    wg_input_count = 0U;
}

static int WG_SelectCRTC(int descriptor, drmModeRes *resources,
                         drmModeConnector *connector, uint32_t *crtc_id)
{
    drmModeEncoder *encoder = NULL;
    int encoder_index;

    if (connector->encoder_id != 0U)
    {
        encoder = drmModeGetEncoder(descriptor, connector->encoder_id);
        if (encoder != NULL && encoder->crtc_id != 0U)
        {
            *crtc_id = encoder->crtc_id;
            drmModeFreeEncoder(encoder);
            return 1;
        }
        drmModeFreeEncoder(encoder);
    }
    for (encoder_index = 0; encoder_index < connector->count_encoders;
         ++encoder_index)
    {
        int crtc_index;

        encoder = drmModeGetEncoder(descriptor,
                                    connector->encoders[encoder_index]);
        if (encoder == NULL) continue;
        for (crtc_index = 0; crtc_index < resources->count_crtcs;
             ++crtc_index)
        {
            if ((encoder->possible_crtcs & (1U << crtc_index)) != 0U)
            {
                *crtc_id = resources->crtcs[crtc_index];
                drmModeFreeEncoder(encoder);
                return 1;
            }
        }
        drmModeFreeEncoder(encoder);
    }
    return 0;
}

static int WG_OpenDRMCard(const char *path)
{
    struct drm_mode_create_dumb create_request;
    struct drm_mode_map_dumb map_request;
    drmModeRes *resources;
    drmModeConnector *connector = NULL;
    int connector_index;
    int mode_index;

    wg_drm.descriptor = open(path, O_RDWR | O_CLOEXEC);
    if (wg_drm.descriptor < 0)
    {
        return 0;
    }
    wg_drm.master = drmSetMaster(wg_drm.descriptor) == 0;
    resources = drmModeGetResources(wg_drm.descriptor);
    if (resources == NULL)
    {
        WG_CloseDRM();
        return 0;
    }
    for (connector_index = 0; connector_index < resources->count_connectors;
         ++connector_index)
    {
        connector = drmModeGetConnector(
            wg_drm.descriptor, resources->connectors[connector_index]);
        if (connector != NULL && connector->connection == DRM_MODE_CONNECTED
            && connector->count_modes > 0
            && WG_SelectCRTC(wg_drm.descriptor, resources, connector,
                             &wg_drm.crtc_id))
        {
            break;
        }
        drmModeFreeConnector(connector);
        connector = NULL;
    }
    drmModeFreeResources(resources);
    if (connector == NULL)
    {
        WG_CloseDRM();
        return 0;
    }
    wg_drm.connector_id = connector->connector_id;
    wg_drm.mode = connector->modes[0];
    for (mode_index = 0; mode_index < connector->count_modes; ++mode_index)
    {
        if ((connector->modes[mode_index].type & DRM_MODE_TYPE_PREFERRED) != 0)
        {
            wg_drm.mode = connector->modes[mode_index];
            break;
        }
    }
    drmModeFreeConnector(connector);
    wg_drm.old_crtc = drmModeGetCrtc(wg_drm.descriptor, wg_drm.crtc_id);

    memset(&create_request, 0, sizeof(create_request));
    create_request.width = (uint32_t)wg_drm.mode.hdisplay;
    create_request.height = (uint32_t)wg_drm.mode.vdisplay;
    create_request.bpp = 32U;
    if (ioctl(wg_drm.descriptor, DRM_IOCTL_MODE_CREATE_DUMB,
              &create_request) < 0)
    {
        WG_CloseDRM();
        return 0;
    }
    wg_drm.handle = create_request.handle;
    wg_drm.pitch = create_request.pitch;
    wg_drm.size = create_request.size;
    if (drmModeAddFB(wg_drm.descriptor, create_request.width,
                     create_request.height, 24U, 32U, wg_drm.pitch,
                     wg_drm.handle, &wg_drm.framebuffer_id) != 0)
    {
        WG_CloseDRM();
        return 0;
    }
    memset(&map_request, 0, sizeof(map_request));
    map_request.handle = wg_drm.handle;
    if (ioctl(wg_drm.descriptor, DRM_IOCTL_MODE_MAP_DUMB, &map_request) < 0)
    {
        WG_CloseDRM();
        return 0;
    }
    wg_drm.pixels = (uint32_t *)mmap(NULL, (size_t)wg_drm.size,
                                    PROT_READ | PROT_WRITE, MAP_SHARED,
                                    wg_drm.descriptor, map_request.offset);
    if (wg_drm.pixels == MAP_FAILED)
    {
        WG_CloseDRM();
        return 0;
    }
    memset(wg_drm.pixels, 0, (size_t)wg_drm.size);
    if ((int64_t)create_request.width * 3
        > (int64_t)create_request.height * 4)
    {
        wg_drm.presentation_height = (int)create_request.height;
        wg_drm.presentation_width = wg_drm.presentation_height * 4 / 3;
    }
    else
    {
        wg_drm.presentation_width = (int)create_request.width;
        wg_drm.presentation_height = wg_drm.presentation_width * 3 / 4;
    }
    wg_drm.presentation_left = ((int)create_request.width
                                - wg_drm.presentation_width) / 2;
    wg_drm.presentation_top = ((int)create_request.height
                               - wg_drm.presentation_height) / 2;
    wg_drm.source_x = (uint16_t *)malloc(
        (size_t)wg_drm.presentation_width * sizeof(*wg_drm.source_x));
    wg_drm.source_y = (uint16_t *)malloc(
        (size_t)wg_drm.presentation_height * sizeof(*wg_drm.source_y));
    if (wg_drm.source_x == NULL || wg_drm.source_y == NULL)
    {
        WG_CloseDRM();
        return 0;
    }
    for (mode_index = 0; mode_index < wg_drm.presentation_width; ++mode_index)
    {
        wg_drm.source_x[mode_index] = (uint16_t)(
            mode_index * WG_SCREEN_WIDTH / wg_drm.presentation_width);
    }
    for (mode_index = 0; mode_index < wg_drm.presentation_height; ++mode_index)
    {
        wg_drm.source_y[mode_index] = (uint16_t)(
            mode_index * WG_SCREEN_HEIGHT / wg_drm.presentation_height);
    }
    if (drmModeSetCrtc(wg_drm.descriptor, wg_drm.crtc_id,
                       wg_drm.framebuffer_id, 0, 0, &wg_drm.connector_id, 1,
                       &wg_drm.mode) != 0)
    {
        WG_CloseDRM();
        return 0;
    }
    return 1;
}

static int WG_OpenDRM(void)
{
    unsigned card;

    if (wg_drm_device[0] != '\0')
    {
        if (WG_OpenDRMCard(wg_drm_device)) return 1;
        fprintf(stderr, "wolf3dgeneric: unable to initialize DRM device %s: %s\n",
                wg_drm_device, strerror(errno));
        return 0;
    }
    for (card = 0U; card < 16U; ++card)
    {
        char path[64];

        (void)snprintf(path, sizeof(path), "/dev/dri/card%u", card);
        if (WG_OpenDRMCard(path)) return 1;
    }
    fprintf(stderr, "wolf3dgeneric: no connected DRM/KMS display found.\n");
    return 0;
}

static void WG_CloseDRM(void)
{
    if (wg_drm.descriptor < 0) return;
    if (wg_drm.old_crtc != NULL)
    {
        (void)drmModeSetCrtc(wg_drm.descriptor, wg_drm.old_crtc->crtc_id,
                             wg_drm.old_crtc->buffer_id,
                             wg_drm.old_crtc->x, wg_drm.old_crtc->y,
                             &wg_drm.connector_id, 1,
                             &wg_drm.old_crtc->mode);
        drmModeFreeCrtc(wg_drm.old_crtc);
        wg_drm.old_crtc = NULL;
    }
    if (wg_drm.pixels != MAP_FAILED)
    {
        (void)munmap(wg_drm.pixels, (size_t)wg_drm.size);
        wg_drm.pixels = MAP_FAILED;
    }
    free(wg_drm.source_x);
    free(wg_drm.source_y);
    wg_drm.source_x = NULL;
    wg_drm.source_y = NULL;
    if (wg_drm.framebuffer_id != 0U)
    {
        (void)drmModeRmFB(wg_drm.descriptor, wg_drm.framebuffer_id);
        wg_drm.framebuffer_id = 0U;
    }
    if (wg_drm.handle != 0U)
    {
        struct drm_mode_destroy_dumb destroy_request;

        memset(&destroy_request, 0, sizeof(destroy_request));
        destroy_request.handle = wg_drm.handle;
        (void)ioctl(wg_drm.descriptor, DRM_IOCTL_MODE_DESTROY_DUMB,
                    &destroy_request);
        wg_drm.handle = 0U;
    }
    if (wg_drm.master) (void)drmDropMaster(wg_drm.descriptor);
    close(wg_drm.descriptor);
    memset(&wg_drm, 0, sizeof(wg_drm));
    wg_drm.descriptor = -1;
    wg_drm.pixels = MAP_FAILED;
}

static void WG_SignalHandler(int signal_number)
{
    (void)signal_number;
    wg_quit_requested = 1;
}

static int WG_LinuxConsoleInit(void)
{
    struct sigaction action;

    memset(&action, 0, sizeof(action));
    action.sa_handler = WG_SignalHandler;
    sigemptyset(&action.sa_mask);
    (void)sigaction(SIGINT, &action, NULL);
    (void)sigaction(SIGTERM, &action, NULL);
    if (clock_gettime(CLOCK_MONOTONIC, &wg_clock_start) != 0
        || !WG_OpenDRM() || !WG_OpenInputs())
    {
        WG_CloseInputs();
        WG_CloseDRM();
        return 0;
    }
    wg_previous_frame_valid = 0;
    wg_console_descriptor = open("/dev/tty", O_RDWR | O_CLOEXEC);
    if (wg_console_descriptor < 0
        || ioctl(wg_console_descriptor, KDGETMODE, &wg_console_mode) < 0)
    {
        if (wg_console_descriptor >= 0) close(wg_console_descriptor);
        wg_console_descriptor = open("/dev/tty0", O_RDWR | O_CLOEXEC);
    }
    if (wg_console_descriptor >= 0
        && ioctl(wg_console_descriptor, KDGETMODE, &wg_console_mode) == 0
        && ioctl(wg_console_descriptor, KDSETMODE, KD_GRAPHICS) == 0)
    {
        wg_console_graphics = 1;
    }
    return 1;
}

static void WG_LinuxConsolePCMShutdown(void);

static void WG_LinuxConsoleShutdown(void)
{
    WG_LinuxConsolePCMShutdown();
    WG_CloseInputs();
    WG_CloseDRM();
    if (wg_console_descriptor >= 0)
    {
        if (wg_console_graphics)
        {
            (void)ioctl(wg_console_descriptor, KDSETMODE, wg_console_mode);
        }
        close(wg_console_descriptor);
        wg_console_descriptor = -1;
        wg_console_graphics = 0;
    }
    if (wg_text_columns != 0U && wg_text_rows != 0U)
    {
        WG_WriteTextScreen(stdout, wg_text_screen, wg_text_columns,
                           wg_text_rows,
                           WG_TextOutputSupportsColor(stdout));
        wg_text_columns = 0U;
        wg_text_rows = 0U;
    }
}

static void WG_LinuxConsolePresent(const uint8_t *pixels,
                                   const uint8_t *palette)
{
    uint32_t colors[WG_PALETTE_COLORS];
    unsigned color;
    int y;

    if (wg_drm.pixels == MAP_FAILED || pixels == NULL || palette == NULL)
    {
        return;
    }
    if (wg_previous_frame_valid
        && memcmp(wg_previous_pixels, pixels,
                  sizeof(wg_previous_pixels)) == 0
        && memcmp(wg_previous_palette, palette,
                  sizeof(wg_previous_palette)) == 0)
    {
        return;
    }
    memcpy(wg_previous_pixels, pixels, sizeof(wg_previous_pixels));
    memcpy(wg_previous_palette, palette, sizeof(wg_previous_palette));
    wg_previous_frame_valid = 1;
    for (color = 0U; color < WG_PALETTE_COLORS; ++color)
    {
        const uint8_t *rgb = palette + (size_t)color * 3U;

        colors[color] = ((uint32_t)rgb[0] << 16)
                      | ((uint32_t)rgb[1] << 8) | rgb[2];
    }
    for (y = 0; y < wg_drm.presentation_height; ++y)
    {
        uint32_t *row = (uint32_t *)((uint8_t *)wg_drm.pixels
            + (size_t)(wg_drm.presentation_top + y) * wg_drm.pitch);
        const uint8_t *source = pixels
            + (size_t)wg_drm.source_y[y] * WG_SCREEN_WIDTH;
        int x;

        for (x = 0; x < wg_drm.presentation_width; ++x)
        {
            row[wg_drm.presentation_left + x]
                = colors[source[wg_drm.source_x[x]]];
        }
    }
}

static uint32_t WG_LinuxConsoleGetTicksMs(void)
{
    struct timespec now;
    uint64_t milliseconds;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) return 0U;
    milliseconds = (uint64_t)(now.tv_sec - wg_clock_start.tv_sec) * 1000U;
    if (now.tv_nsec >= wg_clock_start.tv_nsec)
    {
        milliseconds += (uint64_t)(now.tv_nsec - wg_clock_start.tv_nsec)
                      / 1000000U;
    }
    else
    {
        milliseconds -= 1000U;
        milliseconds += (uint64_t)(1000000000L + now.tv_nsec
                                    - wg_clock_start.tv_nsec) / 1000000U;
    }
    return (uint32_t)milliseconds;
}

static void WG_LinuxConsoleSleepMs(uint32_t milliseconds)
{
    struct timespec requested;

    requested.tv_sec = (time_t)(milliseconds / 1000U);
    requested.tv_nsec = (long)(milliseconds % 1000U) * 1000000L;
    while (nanosleep(&requested, &requested) != 0 && errno == EINTR)
    {
    }
}

static int WG_LinuxConsolePollEvent(wg_event_t *event)
{
    size_t index;

    if (event == NULL) return 0;
    if (wg_event_read != wg_event_write)
    {
        *event = wg_event_queue[wg_event_read];
        wg_event_read = (wg_event_read + 1U) % WG_EVENT_QUEUE_CAPACITY;
        return 1;
    }
    if (wg_quit_requested && !wg_quit_delivered)
    {
        memset(event, 0, sizeof(*event));
        event->type = WG_EVENT_QUIT;
        wg_quit_delivered = 1;
        return 1;
    }
    for (index = 0U; index < wg_input_count; ++index)
    {
        struct input_event source[32];
        ssize_t bytes;

        while ((bytes = read(wg_inputs[index].descriptor, source,
                             sizeof(source))) > 0)
        {
            size_t count = (size_t)bytes / sizeof(source[0]);
            size_t source_index;

            for (source_index = 0U; source_index < count; ++source_index)
            {
                WG_ProcessInputEvent(&wg_inputs[index],
                                     &source[source_index]);
            }
        }
    }
    if (wg_event_read == wg_event_write) return 0;
    *event = wg_event_queue[wg_event_read];
    wg_event_read = (wg_event_read + 1U) % WG_EVENT_QUEUE_CAPACITY;
    return 1;
}

static int WG_LinuxConsoleIsInteractive(void)
{
    return 1;
}

static void WG_LinuxConsoleSetWindowTitle(const char *title)
{
    (void)title;
}

static void WG_LinuxConsoleReportError(const char *message)
{
    fprintf(stderr, "wolf3dgeneric: %s\n", message);
}

static void WG_LinuxConsolePrintMessage(const char *message)
{
    fprintf(stdout, "%s\n", message);
    fflush(stdout);
}

static void WG_LinuxConsolePresentText(const uint8_t *cells,
                                       uint16_t columns, uint16_t rows)
{
    size_t size = (size_t)columns * rows * WG_TEXT_CELL_BYTES;

    if (cells == NULL || columns > WG_TEXT_COLUMNS || rows > WG_TEXT_ROWS
        || size > sizeof(wg_text_screen))
    {
        return;
    }
    memcpy(wg_text_screen, cells, size);
    wg_text_columns = columns;
    wg_text_rows = rows;
}

static int WG_LinuxConsolePCMInit(uint32_t sample_rate, uint16_t channels)
{
    int error;

    WG_LinuxConsolePCMShutdown();
    if (wg_audio_disabled) return 0;
    error = snd_pcm_open(&wg_pcm, wg_alsa_device, SND_PCM_STREAM_PLAYBACK,
                         SND_PCM_NONBLOCK);
    if (error < 0
        || snd_pcm_set_params(wg_pcm, SND_PCM_FORMAT_S16_LE,
                              SND_PCM_ACCESS_RW_INTERLEAVED, channels,
                              sample_rate, 1, 40000U) < 0)
    {
        fprintf(stderr, "wolf3dgeneric: ALSA device '%s' unavailable; "
                        "continuing without audio.\n", wg_alsa_device);
        WG_LinuxConsolePCMShutdown();
        return 0;
    }
    return 1;
}

static void WG_LinuxConsolePCMShutdown(void)
{
    if (wg_pcm != NULL)
    {
        (void)snd_pcm_drop(wg_pcm);
        snd_pcm_close(wg_pcm);
        wg_pcm = NULL;
    }
}

static size_t WG_LinuxConsolePCMWritableFrames(void)
{
    snd_pcm_sframes_t available;

    if (wg_pcm == NULL) return 0U;
    available = snd_pcm_avail_update(wg_pcm);
    if (available < 0)
    {
        if (snd_pcm_recover(wg_pcm, (int)available, 1) < 0) return 0U;
        available = snd_pcm_avail_update(wg_pcm);
    }
    return available > 0 ? (size_t)available : 0U;
}

static int WG_LinuxConsolePCMSubmit(const int16_t *samples,
                                    size_t frame_count)
{
    size_t submitted = 0U;

    if (wg_pcm == NULL || samples == NULL || frame_count == 0U) return 0;
    while (submitted < frame_count)
    {
        snd_pcm_sframes_t result = snd_pcm_writei(
            wg_pcm, samples + submitted * 2U, frame_count - submitted);

        if (result == -EAGAIN)
        {
            if (snd_pcm_wait(wg_pcm, 20) < 0) return 0;
            continue;
        }
        if (result < 0)
        {
            if (snd_pcm_recover(wg_pcm, (int)result, 1) < 0) return 0;
            continue;
        }
        if (result == 0) return 0;
        submitted += (size_t)result;
    }
    return 1;
}

int WG_InstallPlatform(void)
{
    static const wg_platform_api_t platform =
    {
        WG_PLATFORM_API_VERSION,
        sizeof(wg_platform_api_t),
        WG_LinuxConsoleInit,
        WG_LinuxConsoleShutdown,
        WG_LinuxConsolePresent,
        WG_LinuxConsoleGetTicksMs,
        WG_LinuxConsoleSleepMs,
        WG_LinuxConsolePollEvent,
        WG_LinuxConsoleIsInteractive,
        WG_LinuxConsoleSetWindowTitle,
        WG_LinuxConsolePrintMessage,
        WG_LinuxConsoleReportError,
        WG_LinuxConsolePresentText,
        WG_LinuxConsolePCMInit,
        WG_LinuxConsolePCMShutdown,
        WG_LinuxConsolePCMWritableFrames,
        WG_LinuxConsolePCMSubmit
    };

    return wolf3dgeneric_SetPlatform(&platform) == WG_RESULT_OK;
}
