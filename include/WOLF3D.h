#ifndef WOLF3D_H
#define WOLF3D_H

#include <stddef.h>
#if defined(_MSC_VER) && _MSC_VER < 1600
#include "WOLF3D_STDINT.h"
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WOLF3D_STATIC)
#define WOLF3D_API
#elif defined(_WIN32)
#if defined(WOLF3D_BUILD)
#define WOLF3D_API __declspec(dllexport)
#else
#define WOLF3D_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define WOLF3D_API __attribute__((visibility("default")))
#else
#define WOLF3D_API
#endif

#define WOLF3D_SCREEN_WIDTH 320
#define WOLF3D_SCREEN_HEIGHT 200
#define WOLF3D_PALETTE_COLORS 256
#define WOLF3D_TEXT_COLUMNS 80
#define WOLF3D_TEXT_ROWS 25
#define WOLF3D_TEXT_CELL_BYTES 2
#define WOLF3D_MAX_JOYSTICKS 2

typedef enum wolf3d_result
{
    WOLF3D_RESULT_OK = 0,
    WOLF3D_RESULT_QUIT = 1,
    WOLF3D_RESULT_NOT_IMPLEMENTED = 2,
    WOLF3D_RESULT_INVALID_ARGUMENT = 3,
    WOLF3D_RESULT_PLATFORM_ERROR = 4
} wolf3d_result_t;

typedef enum wolf3d_event_type
{
    WOLF3D_EVENT_NONE = 0,
    WOLF3D_EVENT_QUIT,
    WOLF3D_EVENT_KEY,
    WOLF3D_EVENT_MOUSE_MOTION,
    WOLF3D_EVENT_MOUSE_BUTTON,
    WOLF3D_EVENT_JOYSTICK
} wolf3d_event_type_t;

/* IBM PC set-1 scan codes, matching the identifiers used by Wolf3D. */
typedef enum wolf3d_key
{
    WOLF3D_KEY_ESCAPE = 0x01,
    WOLF3D_KEY_1 = 0x02,
    WOLF3D_KEY_2 = 0x03,
    WOLF3D_KEY_3 = 0x04,
    WOLF3D_KEY_4 = 0x05,
    WOLF3D_KEY_E = 0x12,
    WOLF3D_KEY_G = 0x22,
    WOLF3D_KEY_H = 0x23,
    WOLF3D_KEY_I = 0x17,
    WOLF3D_KEY_L = 0x26,
    WOLF3D_KEY_M = 0x32,
    WOLF3D_KEY_Q = 0x10,
    WOLF3D_KEY_Y = 0x15,
    WOLF3D_KEY_BACKSPACE = 0x0e,
    WOLF3D_KEY_TAB = 0x0f,
    WOLF3D_KEY_ENTER = 0x1c,
    WOLF3D_KEY_CONTROL = 0x1d,
    WOLF3D_KEY_LEFT_SHIFT = 0x2a,
    WOLF3D_KEY_N = 0x31,
    WOLF3D_KEY_RIGHT_SHIFT = 0x36,
    WOLF3D_KEY_ALT = 0x38,
    WOLF3D_KEY_SPACE = 0x39,
    WOLF3D_KEY_CAPS_LOCK = 0x3a,
    WOLF3D_KEY_F1 = 0x3b,
    WOLF3D_KEY_F2 = 0x3c,
    WOLF3D_KEY_F3 = 0x3d,
    WOLF3D_KEY_F4 = 0x3e,
    WOLF3D_KEY_F5 = 0x3f,
    WOLF3D_KEY_F6 = 0x40,
    WOLF3D_KEY_F7 = 0x41,
    WOLF3D_KEY_F8 = 0x42,
    WOLF3D_KEY_F9 = 0x43,
    WOLF3D_KEY_F10 = 0x44,
    WOLF3D_KEY_HOME = 0x47,
    WOLF3D_KEY_UP = 0x48,
    WOLF3D_KEY_LEFT = 0x4b,
    WOLF3D_KEY_RIGHT = 0x4d,
    WOLF3D_KEY_END = 0x4f,
    WOLF3D_KEY_DOWN = 0x50,
    WOLF3D_KEY_DELETE = 0x53,
    WOLF3D_KEY_PAUSE = 0xe1
} wolf3d_key_t;

typedef struct wolf3d_event
{
    wolf3d_event_type_t type;
    int pressed;
    uint16_t key;
    int16_t x;
    int16_t y;
    uint8_t button;
    uint8_t joystick;
    uint8_t connected;
    uint32_t buttons;
} wolf3d_event_t;

#define WOLF3D_PLATFORM_API_VERSION 6U

#define WOLF3D_INPUT_DEVICE_MOUSE    0x01U
#define WOLF3D_INPUT_DEVICE_JOYSTICK 0x02U

typedef struct wolf3d_pcm_format
{
    uint32_t sample_rate;
    uint16_t channels;
    uint16_t bits_per_sample;
} wolf3d_pcm_format_t;

#define WOLF3D_OPL_CAP_PCM_RENDER 0x01U
#define WOLF3D_OPL_CAP_SILENT 0x02U
#define WOLF3D_OPL_CAP_HARDWARE 0x04U

typedef struct wolf3d_platform_api
{
    uint32_t api_version;
    size_t struct_size;
    int (*init)(void);
    void (*shutdown)(void);
    void (*present)(const uint8_t *pixels, const uint8_t *palette);
    uint32_t (*get_ticks_ms)(void);
    void (*sleep_ms)(uint32_t milliseconds);
    int (*poll_event)(wolf3d_event_t *event);
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
    int (*pcm_init_ex)(const wolf3d_pcm_format_t *requested,
                       wolf3d_pcm_format_t *obtained);
    int (*opl_hardware_init)(void);
    void (*opl_hardware_shutdown)(void);
    void (*opl_hardware_write)(uint16_t register_number, uint8_t value);
    uint32_t (*input_devices)(void);
    void (*set_mouse_capture)(int captured);
} wolf3d_platform_api_t;

WOLF3D_API extern uint8_t *wolf3d_ScreenBuffer;
WOLF3D_API extern uint8_t wolf3d_Palette[WOLF3D_PALETTE_COLORS * 3];

WOLF3D_API wolf3d_result_t wolf3d_SetPlatform(
    const wolf3d_platform_api_t *platform);
WOLF3D_API wolf3d_result_t wolf3d_Create(int argc, char **argv);
WOLF3D_API wolf3d_result_t wolf3d_Run(void);
WOLF3D_API void wolf3d_Shutdown(void);
WOLF3D_API size_t wolf3d_GetOPLDriverCount(void);
WOLF3D_API const char *wolf3d_GetOPLDriverName(size_t index);
WOLF3D_API uint32_t wolf3d_GetOPLDriverCapabilities(size_t index);
WOLF3D_API const char *wolf3d_GetSelectedOPLDriver(void);

#ifdef __cplusplus
}
#endif

#endif
