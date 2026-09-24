#ifndef WOLF3DGENERIC_H
#define WOLF3DGENERIC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WG_SCREEN_WIDTH 320
#define WG_SCREEN_HEIGHT 200
#define WG_PALETTE_COLORS 256

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
    WG_EVENT_MOUSE_BUTTON
} wg_event_type_t;

/* IBM PC set-1 scan codes, matching the identifiers used by Wolf3D. */
typedef enum wg_key
{
    WG_KEY_ESCAPE = 0x01,
    WG_KEY_ENTER = 0x1c,
    WG_KEY_CONTROL = 0x1d,
    WG_KEY_SPACE = 0x39,
    WG_KEY_LEFT = 0x4b,
    WG_KEY_UP = 0x48,
    WG_KEY_RIGHT = 0x4d,
    WG_KEY_DOWN = 0x50
} wg_key_t;

typedef struct wg_event
{
    wg_event_type_t type;
    int pressed;
    uint16_t key;
    int16_t x;
    int16_t y;
    uint8_t button;
} wg_event_t;

extern uint8_t *WG_ScreenBuffer;
extern uint8_t WG_Palette[WG_PALETTE_COLORS * 3];

wg_result_t wolf3dgeneric_Create(int argc, char **argv);
wg_result_t wolf3dgeneric_Run(void);
void wolf3dgeneric_Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
