#ifndef WOLF3DGENERIC_H
#define WOLF3DGENERIC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WOLF3DGENERIC_STATIC)
#define WG_API
#elif defined(_WIN32)
#if defined(WOLF3DGENERIC_BUILD)
#define WG_API __declspec(dllexport)
#else
#define WG_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define WG_API __attribute__((visibility("default")))
#else
#define WG_API
#endif

#define WG_SCREEN_WIDTH 320
#define WG_SCREEN_HEIGHT 200
#define WG_PALETTE_COLORS 256
#define WG_TEXT_COLUMNS 80
#define WG_TEXT_ROWS 25
#define WG_TEXT_CELL_BYTES 2
#define WG_MAX_JOYSTICKS 2

typedef enum wg_result
{
    WG_RESULT_OK = 0,
    WG_RESULT_QUIT = 1,
    WG_RESULT_NOT_IMPLEMENTED = 2,
    WG_RESULT_INVALID_ARGUMENT = 3,
    WG_RESULT_PLATFORM_ERROR = 4
} wg_result_t;

typedef enum wg_event_type
{
    WG_EVENT_NONE = 0,
    WG_EVENT_QUIT,
    WG_EVENT_KEY,
    WG_EVENT_MOUSE_MOTION,
    WG_EVENT_MOUSE_BUTTON,
    WG_EVENT_JOYSTICK
} wg_event_type_t;

/* IBM PC set-1 scan codes, matching the identifiers used by Wolf3D. */
typedef enum wg_key
{
    WG_KEY_ESCAPE = 0x01,
    WG_KEY_1 = 0x02,
    WG_KEY_2 = 0x03,
    WG_KEY_3 = 0x04,
    WG_KEY_4 = 0x05,
    WG_KEY_E = 0x12,
    WG_KEY_G = 0x22,
    WG_KEY_H = 0x23,
    WG_KEY_I = 0x17,
    WG_KEY_Q = 0x10,
    WG_KEY_Y = 0x15,
    WG_KEY_BACKSPACE = 0x0e,
    WG_KEY_TAB = 0x0f,
    WG_KEY_ENTER = 0x1c,
    WG_KEY_CONTROL = 0x1d,
    WG_KEY_LEFT_SHIFT = 0x2a,
    WG_KEY_N = 0x31,
    WG_KEY_RIGHT_SHIFT = 0x36,
    WG_KEY_ALT = 0x38,
    WG_KEY_SPACE = 0x39,
    WG_KEY_CAPS_LOCK = 0x3a,
    WG_KEY_F1 = 0x3b,
    WG_KEY_F2 = 0x3c,
    WG_KEY_F3 = 0x3d,
    WG_KEY_F4 = 0x3e,
    WG_KEY_F5 = 0x3f,
    WG_KEY_F6 = 0x40,
    WG_KEY_F7 = 0x41,
    WG_KEY_F8 = 0x42,
    WG_KEY_F9 = 0x43,
    WG_KEY_F10 = 0x44,
    WG_KEY_HOME = 0x47,
    WG_KEY_UP = 0x48,
    WG_KEY_LEFT = 0x4b,
    WG_KEY_RIGHT = 0x4d,
    WG_KEY_END = 0x4f,
    WG_KEY_DOWN = 0x50,
    WG_KEY_DELETE = 0x53,
    WG_KEY_PAUSE = 0xe1
} wg_key_t;

typedef struct wg_event
{
    wg_event_type_t type;
    int pressed;
    uint16_t key;
    int16_t x;
    int16_t y;
    uint8_t button;
    uint8_t joystick;
    uint8_t connected;
    uint32_t buttons;
} wg_event_t;

#define WG_PLATFORM_API_VERSION 2U

typedef struct wg_platform_api
{
    uint32_t api_version;
    size_t struct_size;
    int (*init)(void);
    void (*shutdown)(void);
    void (*present)(const uint8_t *pixels, const uint8_t *palette);
    uint32_t (*get_ticks_ms)(void);
    void (*sleep_ms)(uint32_t milliseconds);
    int (*poll_event)(wg_event_t *event);
    int (*is_interactive)(void);
    void (*set_window_title)(const char *title);
    void (*print_message)(const char *message);
    void (*report_error)(const char *message);
    void (*present_text)(const uint8_t *cells, uint16_t columns,
                         uint16_t rows);
    int (*pcm_init)(uint32_t sample_rate, uint16_t channels);
    void (*pcm_shutdown)(void);
    size_t (*pcm_writable_frames)(void);
    int (*pcm_submit)(const int16_t *samples, size_t frame_count);
} wg_platform_api_t;

WG_API extern uint8_t *WG_ScreenBuffer;
WG_API extern uint8_t WG_Palette[WG_PALETTE_COLORS * 3];

WG_API wg_result_t wolf3dgeneric_SetPlatform(
    const wg_platform_api_t *platform);
WG_API wg_result_t wolf3dgeneric_Create(int argc, char **argv);
WG_API wg_result_t wolf3dgeneric_Run(void);
WG_API void wolf3dgeneric_Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
