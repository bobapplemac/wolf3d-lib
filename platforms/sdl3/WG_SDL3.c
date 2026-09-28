#include "WOLF3DGENERIC.h"
#include "../WG_HOST.h"
#include "../WG_TEXT_OUTPUT.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define WG_SDL_LOGICAL_WIDTH 320
#define WG_SDL_LOGICAL_HEIGHT 240
#define WG_SDL_AUDIO_TARGET_FRAMES 2048U

static SDL_Window *wg_window;
static SDL_Renderer *wg_renderer;
static SDL_Texture *wg_texture;
static SDL_AudioStream *wg_audio;
static uint16_t wg_audio_channels;
static uint32_t wg_rgba[WG_SCREEN_WIDTH * WG_SCREEN_HEIGHT];
static SDL_Gamepad *wg_gamepads[WG_MAX_JOYSTICKS];
static int16_t wg_joystick_x[WG_MAX_JOYSTICKS];
static int16_t wg_joystick_y[WG_MAX_JOYSTICKS];
static uint32_t wg_joystick_buttons[WG_MAX_JOYSTICKS];
static uint8_t wg_joystick_connected[WG_MAX_JOYSTICKS];
static uint8_t wg_joystick_dirty[WG_MAX_JOYSTICKS];
static uint8_t wg_text_screen[WG_TEXT_COLUMNS * WG_TEXT_ROWS
                              * WG_TEXT_CELL_BYTES];
static uint16_t wg_text_columns;
static uint16_t wg_text_rows;
static int wg_mouse_enabled;

static int16_t WG_SDLClampMotion(float value)
{
    if (value > (float)INT16_MAX)
    {
        return INT16_MAX;
    }
    if (value < (float)INT16_MIN)
    {
        return INT16_MIN;
    }
    return (int16_t)value;
}

static uint16_t WG_SDLScanCode(SDL_Scancode code)
{
    static const uint8_t letters[26] =
    {
        0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17,
        0x24, 0x25, 0x26, 0x32, 0x31, 0x18, 0x19, 0x10, 0x13,
        0x1f, 0x14, 0x16, 0x2f, 0x11, 0x2d, 0x15, 0x2c
    };

    if (code >= SDL_SCANCODE_A && code <= SDL_SCANCODE_Z)
    {
        return letters[code - SDL_SCANCODE_A];
    }
    if (code >= SDL_SCANCODE_1 && code <= SDL_SCANCODE_9)
    {
        return (uint16_t)(0x02 + code - SDL_SCANCODE_1);
    }
    if (code >= SDL_SCANCODE_F1 && code <= SDL_SCANCODE_F10)
    {
        return (uint16_t)(0x3b + code - SDL_SCANCODE_F1);
    }
    switch (code)
    {
        case SDL_SCANCODE_0: return 0x0b;
        case SDL_SCANCODE_ESCAPE: return 0x01;
        case SDL_SCANCODE_MINUS: return 0x0c;
        case SDL_SCANCODE_EQUALS: return 0x0d;
        case SDL_SCANCODE_BACKSPACE: return 0x0e;
        case SDL_SCANCODE_TAB: return 0x0f;
        case SDL_SCANCODE_LEFTBRACKET: return 0x1a;
        case SDL_SCANCODE_RIGHTBRACKET: return 0x1b;
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_KP_ENTER: return 0x1c;
        case SDL_SCANCODE_LCTRL:
        case SDL_SCANCODE_RCTRL: return 0x1d;
        case SDL_SCANCODE_SEMICOLON: return 0x27;
        case SDL_SCANCODE_APOSTROPHE: return 0x28;
        case SDL_SCANCODE_GRAVE: return 0x29;
        case SDL_SCANCODE_LSHIFT: return 0x2a;
        case SDL_SCANCODE_BACKSLASH:
        case SDL_SCANCODE_NONUSBACKSLASH: return 0x2b;
        case SDL_SCANCODE_COMMA: return 0x33;
        case SDL_SCANCODE_PERIOD: return 0x34;
        case SDL_SCANCODE_SLASH: return 0x35;
        case SDL_SCANCODE_RSHIFT: return 0x36;
        case SDL_SCANCODE_KP_MULTIPLY: return 0x37;
        case SDL_SCANCODE_LALT:
        case SDL_SCANCODE_RALT: return 0x38;
        case SDL_SCANCODE_SPACE: return 0x39;
        case SDL_SCANCODE_CAPSLOCK: return 0x3a;
        case SDL_SCANCODE_NUMLOCKCLEAR: return 0x45;
        case SDL_SCANCODE_SCROLLLOCK: return 0x46;
        case SDL_SCANCODE_HOME:
        case SDL_SCANCODE_KP_7: return 0x47;
        case SDL_SCANCODE_UP:
        case SDL_SCANCODE_KP_8: return 0x48;
        case SDL_SCANCODE_PAGEUP:
        case SDL_SCANCODE_KP_9: return 0x49;
        case SDL_SCANCODE_KP_MINUS: return 0x4a;
        case SDL_SCANCODE_LEFT:
        case SDL_SCANCODE_KP_4: return 0x4b;
        case SDL_SCANCODE_KP_5: return 0x4c;
        case SDL_SCANCODE_RIGHT:
        case SDL_SCANCODE_KP_6: return 0x4d;
        case SDL_SCANCODE_KP_PLUS: return 0x4e;
        case SDL_SCANCODE_END:
        case SDL_SCANCODE_KP_1: return 0x4f;
        case SDL_SCANCODE_DOWN:
        case SDL_SCANCODE_KP_2: return 0x50;
        case SDL_SCANCODE_PAGEDOWN:
        case SDL_SCANCODE_KP_3: return 0x51;
        case SDL_SCANCODE_INSERT:
        case SDL_SCANCODE_KP_0: return 0x52;
        case SDL_SCANCODE_DELETE:
        case SDL_SCANCODE_KP_PERIOD: return 0x53;
        case SDL_SCANCODE_F11: return 0x57;
        case SDL_SCANCODE_F12: return 0x58;
        case SDL_SCANCODE_PAUSE: return WG_KEY_PAUSE;
        default: return 0;
    }
}

static void WG_SDLCloseGamepads(void)
{
    unsigned int slot;
    for (slot = 0U; slot < WG_MAX_JOYSTICKS; ++slot)
    {
        if (wg_gamepads[slot] != NULL)
        {
            SDL_CloseGamepad(wg_gamepads[slot]);
            wg_gamepads[slot] = NULL;
        }
    }
}

static void WG_SDLRefreshGamepads(void)
{
    SDL_JoystickID *identifiers;
    int count = 0;
    unsigned int slot;

    WG_SDLCloseGamepads();
    identifiers = SDL_GetGamepads(&count);
    for (slot = 0U; slot < WG_MAX_JOYSTICKS; ++slot)
    {
        uint8_t connected = 0U;
        if (identifiers != NULL && (int)slot < count)
        {
            wg_gamepads[slot] = SDL_OpenGamepad(identifiers[slot]);
            connected = wg_gamepads[slot] != NULL;
        }
        if (connected != wg_joystick_connected[slot])
        {
            wg_joystick_connected[slot] = connected;
            wg_joystick_dirty[slot] = 1U;
        }
    }
    SDL_free(identifiers);
}

static void WG_SDLPollGamepads(void)
{
    unsigned int slot;
    for (slot = 0U; slot < WG_MAX_JOYSTICKS; ++slot)
    {
        int16_t x;
        int16_t y;
        uint32_t buttons;
        SDL_Gamepad *gamepad = wg_gamepads[slot];
        if (gamepad == NULL)
        {
            continue;
        }
        x = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX);
        y = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);
        buttons = (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH) ? 1U : 0U)
                | (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_EAST) ? 2U : 0U)
                | (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_WEST) ? 4U : 0U)
                | (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_NORTH) ? 8U : 0U);
        if (x != wg_joystick_x[slot] || y != wg_joystick_y[slot]
            || buttons != wg_joystick_buttons[slot])
        {
            wg_joystick_x[slot] = x;
            wg_joystick_y[slot] = y;
            wg_joystick_buttons[slot] = buttons;
            wg_joystick_dirty[slot] = 1U;
        }
    }
}

static int WG_SDLEmitGamepad(wg_event_t *event)
{
    unsigned int slot;
    WG_SDLPollGamepads();
    for (slot = 0U; slot < WG_MAX_JOYSTICKS; ++slot)
    {
        if (wg_joystick_dirty[slot])
        {
            memset(event, 0, sizeof(*event));
            event->type = WG_EVENT_JOYSTICK;
            event->joystick = (uint8_t)slot;
            event->connected = wg_joystick_connected[slot];
            event->x = wg_joystick_x[slot];
            event->y = wg_joystick_y[slot];
            event->buttons = wg_joystick_buttons[slot];
            wg_joystick_dirty[slot] = 0U;
            return 1;
        }
    }
    return 0;
}

static int WG_SDLInit(void)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD))
    {
        fprintf(stderr, "wolf3dgeneric: SDL initialization failed: %s\n",
                SDL_GetError());
        return 0;
    }
    wg_window = SDL_CreateWindow("wolf3dgeneric", 960, 720,
                                 SDL_WINDOW_RESIZABLE);
    if (wg_window == NULL)
    {
        fprintf(stderr, "wolf3dgeneric: SDL window creation failed: %s\n",
                SDL_GetError());
        SDL_Quit();
        return 0;
    }
    wg_renderer = SDL_CreateRenderer(wg_window, NULL);
    if (wg_renderer == NULL)
    {
        fprintf(stderr, "wolf3dgeneric: SDL renderer creation failed: %s\n",
                SDL_GetError());
        SDL_DestroyWindow(wg_window);
        wg_window = NULL;
        SDL_Quit();
        return 0;
    }
    wg_texture = SDL_CreateTexture(wg_renderer, SDL_PIXELFORMAT_RGBA8888,
                                   SDL_TEXTUREACCESS_STREAMING,
                                   WG_SCREEN_WIDTH, WG_SCREEN_HEIGHT);
    if (wg_texture == NULL
        || !SDL_SetTextureScaleMode(wg_texture, SDL_SCALEMODE_NEAREST)
        || !SDL_SetRenderLogicalPresentation(wg_renderer,
                                             WG_SDL_LOGICAL_WIDTH,
                                             WG_SDL_LOGICAL_HEIGHT,
                                             SDL_LOGICAL_PRESENTATION_LETTERBOX))
    {
        fprintf(stderr, "wolf3dgeneric: SDL renderer setup failed: %s\n",
                SDL_GetError());
        SDL_DestroyTexture(wg_texture);
        SDL_DestroyRenderer(wg_renderer);
        SDL_DestroyWindow(wg_window);
        wg_texture = NULL;
        wg_renderer = NULL;
        wg_window = NULL;
        SDL_Quit();
        return 0;
    }
    if (wg_mouse_enabled
        && !SDL_SetWindowRelativeMouseMode(wg_window, true))
    {
        fprintf(stderr, "wolf3dgeneric: SDL relative mouse mode failed: %s\n",
                SDL_GetError());
        SDL_DestroyTexture(wg_texture);
        SDL_DestroyRenderer(wg_renderer);
        SDL_DestroyWindow(wg_window);
        wg_texture = NULL;
        wg_renderer = NULL;
        wg_window = NULL;
        SDL_Quit();
        return 0;
    }
    WG_SDLRefreshGamepads();
    return 1;
}

#ifdef _WIN32
static void WG_SDLWriteWindowsText(void)
{
    HANDLE output;
    CHAR_INFO characters[WG_TEXT_COLUMNS * WG_TEXT_ROWS];
    COORD size;
    COORD origin = { 0, 0 };
    SMALL_RECT rectangle;
    size_t index;

    output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output == NULL || output == INVALID_HANDLE_VALUE)
    {
        (void)AttachConsole(ATTACH_PARENT_PROCESS);
        output = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                             OPEN_EXISTING, 0, NULL);
    }
    if (output == NULL || output == INVALID_HANDLE_VALUE)
    {
        return;
    }
    for (index = 0U; index < (size_t)wg_text_columns * wg_text_rows; ++index)
    {
        characters[index].Char.AsciiChar = (CHAR)wg_text_screen[index * 2U];
        characters[index].Attributes = wg_text_screen[index * 2U + 1U];
    }
    size.X = (SHORT)wg_text_columns;
    size.Y = (SHORT)wg_text_rows;
    rectangle.Left = 0;
    rectangle.Top = 0;
    rectangle.Right = (SHORT)(wg_text_columns - 1U);
    rectangle.Bottom = (SHORT)(wg_text_rows - 1U);
    (void)SetConsoleOutputCP(437U);
    (void)WriteConsoleOutputA(output, characters, size, origin, &rectangle);
}
#endif

static void WG_SDLShutdown(void)
{
    if (wg_audio != NULL)
    {
        SDL_DestroyAudioStream(wg_audio);
        wg_audio = NULL;
    }
    WG_SDLCloseGamepads();
    SDL_DestroyTexture(wg_texture);
    SDL_DestroyRenderer(wg_renderer);
    SDL_DestroyWindow(wg_window);
    wg_texture = NULL;
    wg_renderer = NULL;
    wg_window = NULL;
    SDL_Quit();
    if (wg_text_columns != 0U && wg_text_rows != 0U)
    {
#ifdef _WIN32
        WG_SDLWriteWindowsText();
#else
        WG_WriteTextScreen(stdout, wg_text_screen, wg_text_columns,
                           wg_text_rows, WG_TextOutputSupportsColor(stdout));
#endif
    }
}

static void WG_SDLPresent(const uint8_t *pixels, const uint8_t *palette)
{
    SDL_FRect destination = { 0.0f, 0.0f, 320.0f, 240.0f };
    size_t index;
    if (pixels == NULL || palette == NULL || wg_texture == NULL)
    {
        return;
    }
    for (index = 0U; index < WG_SCREEN_WIDTH * WG_SCREEN_HEIGHT; ++index)
    {
        const uint8_t *color = palette + (size_t)pixels[index] * 3U;
        wg_rgba[index] = ((uint32_t)color[0] << 24)
                       | ((uint32_t)color[1] << 16)
                       | ((uint32_t)color[2] << 8) | 0xffU;
    }
    (void)SDL_UpdateTexture(wg_texture, NULL, wg_rgba,
                            WG_SCREEN_WIDTH * (int)sizeof(wg_rgba[0]));
    (void)SDL_SetRenderDrawColor(wg_renderer, 0, 0, 0, 255);
    (void)SDL_RenderClear(wg_renderer);
    (void)SDL_RenderTexture(wg_renderer, wg_texture, NULL, &destination);
    (void)SDL_RenderPresent(wg_renderer);
}

static uint32_t WG_SDLGetTicksMs(void)
{
    return (uint32_t)SDL_GetTicks();
}

static void WG_SDLSleepMs(uint32_t milliseconds)
{
    SDL_Delay(milliseconds);
}

static int WG_SDLPollEvent(wg_event_t *event)
{
    SDL_Event sdl_event;
    if (event == NULL)
    {
        return 0;
    }
    if (WG_SDLEmitGamepad(event))
    {
        return 1;
    }
    while (SDL_PollEvent(&sdl_event))
    {
        memset(event, 0, sizeof(*event));
        switch (sdl_event.type)
        {
            case SDL_EVENT_QUIT:
                event->type = WG_EVENT_QUIT;
                return 1;
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
                event->key = WG_SDLScanCode(sdl_event.key.scancode);
                if (event->key != 0U && !sdl_event.key.repeat)
                {
                    event->type = WG_EVENT_KEY;
                    event->pressed = sdl_event.key.down;
                    return 1;
                }
                break;
            case SDL_EVENT_MOUSE_MOTION:
                if (wg_mouse_enabled)
                {
                    event->type = WG_EVENT_MOUSE_MOTION;
                    event->x = WG_SDLClampMotion(sdl_event.motion.xrel);
                    event->y = WG_SDLClampMotion(sdl_event.motion.yrel);
                    return 1;
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (wg_mouse_enabled && sdl_event.button.button <= 3U)
                {
                    static const uint8_t buttons[4] = { 0U, 1U, 3U, 2U };
                    event->type = WG_EVENT_MOUSE_BUTTON;
                    event->button = buttons[sdl_event.button.button];
                    event->pressed = sdl_event.button.down;
                    return 1;
                }
                break;
            case SDL_EVENT_GAMEPAD_ADDED:
            case SDL_EVENT_GAMEPAD_REMOVED:
            case SDL_EVENT_GAMEPAD_REMAPPED:
                WG_SDLRefreshGamepads();
                if (WG_SDLEmitGamepad(event))
                {
                    return 1;
                }
                break;
            default:
                break;
        }
    }
    return WG_SDLEmitGamepad(event);
}

static int WG_SDLIsInteractive(void)
{
    return 1;
}

static void WG_SDLSetWindowTitle(const char *title)
{
    if (wg_window != NULL && title != NULL)
    {
        (void)SDL_SetWindowTitle(wg_window, title);
    }
}

static void WG_SDLPrintMessage(const char *message)
{
    if (message != NULL)
    {
        fputs(message, stdout);
        fflush(stdout);
    }
}

static void WG_SDLReportError(const char *message)
{
    if (message != NULL)
    {
        fprintf(stderr, "%s\n", message);
        if (wg_window != NULL)
        {
            (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                           "wolf3dgeneric", message,
                                           wg_window);
        }
    }
}

static void WG_SDLPresentText(const uint8_t *cells, uint16_t columns,
                              uint16_t rows)
{
    size_t bytes;
    if (cells == NULL || columns == 0U || rows == 0U
        || columns > WG_TEXT_COLUMNS || rows > WG_TEXT_ROWS)
    {
        return;
    }
    bytes = (size_t)columns * rows * WG_TEXT_CELL_BYTES;
    memcpy(wg_text_screen, cells, bytes);
    wg_text_columns = columns;
    wg_text_rows = rows;
}

static int WG_SDLPCMInit(uint32_t sample_rate, uint16_t channels)
{
    SDL_AudioSpec specification;
    if (channels == 0U || channels > 2U)
    {
        return 0;
    }
    specification.format = SDL_AUDIO_S16;
    specification.channels = (int)channels;
    specification.freq = (int)sample_rate;
    wg_audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                         &specification, NULL, NULL);
    if (wg_audio == NULL || !SDL_ResumeAudioStreamDevice(wg_audio))
    {
        fprintf(stderr, "wolf3dgeneric: SDL audio unavailable: %s\n",
                SDL_GetError());
        if (wg_audio != NULL)
        {
            SDL_DestroyAudioStream(wg_audio);
            wg_audio = NULL;
        }
        return 0;
    }
    wg_audio_channels = channels;
    return 1;
}

static void WG_SDLPCMShutdown(void)
{
    if (wg_audio != NULL)
    {
        SDL_DestroyAudioStream(wg_audio);
        wg_audio = NULL;
    }
}

static size_t WG_SDLPCMWritableFrames(void)
{
    int queued;
    size_t frames;
    if (wg_audio == NULL || wg_audio_channels == 0U)
    {
        return 0U;
    }
    queued = SDL_GetAudioStreamQueued(wg_audio);
    if (queued < 0)
    {
        return 0U;
    }
    frames = (size_t)queued / ((size_t)wg_audio_channels * sizeof(int16_t));
    return frames < WG_SDL_AUDIO_TARGET_FRAMES
               ? WG_SDL_AUDIO_TARGET_FRAMES - frames : 0U;
}

static int WG_SDLPCMSubmit(const int16_t *samples, size_t frame_count)
{
    size_t bytes;
    if (wg_audio == NULL || samples == NULL || frame_count == 0U)
    {
        return 0;
    }
    bytes = frame_count * wg_audio_channels * sizeof(*samples);
    return bytes <= INT_MAX
        && SDL_PutAudioStreamData(wg_audio, samples, (int)bytes);
}

int WG_InstallPlatform(void)
{
    static const wg_platform_api_t platform =
    {
        WG_PLATFORM_API_VERSION,
        sizeof(wg_platform_api_t),
        WG_SDLInit,
        WG_SDLShutdown,
        WG_SDLPresent,
        WG_SDLGetTicksMs,
        WG_SDLSleepMs,
        WG_SDLPollEvent,
        WG_SDLIsInteractive,
        WG_SDLSetWindowTitle,
        WG_SDLPrintMessage,
        WG_SDLReportError,
        WG_SDLPresentText,
        WG_SDLPCMInit,
        WG_SDLPCMShutdown,
        WG_SDLPCMWritableFrames,
        WG_SDLPCMSubmit
    };
    return wolf3dgeneric_SetPlatform(&platform) == WG_RESULT_OK;
}

static void WG_SDLPrintHelp(const char *program)
{
    printf("Usage: %s [game options]\n\n", program != NULL ? program
           : "wolf3dgeneric-sdl3");
    printf("SDL3 host options:\n");
    printf("  --sdl3-help  Show this help and exit\n\n");
    printf("Pass --mouse to expose relative mouse input to the game.\n");
}

int main(int argc, char **argv)
{
    int index;
    wg_result_t result;

    for (index = 1; index < argc; ++index)
    {
        if (strcmp(argv[index], "--sdl3-help") == 0)
        {
            WG_SDLPrintHelp(argv[0]);
            return 0;
        }
        if (strcmp(argv[index], "--mouse") == 0)
        {
            wg_mouse_enabled = 1;
        }
    }
    if (!WG_InstallPlatform())
    {
        return 1;
    }
    result = wolf3dgeneric_Create(argc, argv);
    if (result == WG_RESULT_OK)
    {
        result = wolf3dgeneric_Run();
        wolf3dgeneric_Shutdown();
    }
    return result == WG_RESULT_QUIT || result == WG_RESULT_NOT_IMPLEMENTED
               ? 0 : 1;
}
