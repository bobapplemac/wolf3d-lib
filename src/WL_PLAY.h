#ifndef WL_PLAY_H
#define WL_PLAY_H

#include <stddef.h>
#include <stdint.h>

#define WL_MAX_TICS 10U

#include "WG_DATA.h"

struct wg_level;
struct wg_view_tables;
struct wg_graphics;

typedef struct wl_input
{
    uint8_t up;
    uint8_t down;
    uint8_t left;
    uint8_t right;
    uint8_t attack;
    uint8_t use;
    uint8_t strafe;
    uint8_t run;
    uint8_t weapon;
    uint8_t mouse_adjustment;
    int16_t mouse_x;
    int16_t mouse_y;
    int8_t joystick_x;
    int8_t joystick_y;
    uint8_t joystick_enabled;
} wl_input_t;

typedef struct wl_play_state
{
    uint8_t attack_held;
    uint8_t use_held;
} wl_play_state_t;

#define WL_DEMO_TICS 4U

typedef struct wl_demo
{
    const uint8_t *commands;
    size_t command_count;
    size_t position;
    uint8_t map_number;
} wl_demo_t;

typedef struct wl_demo_command
{
    uint8_t buttons;
    int8_t control_x;
    int8_t control_y;
} wl_demo_command_t;

void WL_PlayStateReset(wl_play_state_t *state);
int WL_DemoOpen(wl_demo_t *demo, const uint8_t *data, size_t size);
int WL_DemoNext(wl_demo_t *demo, wl_demo_command_t *command);
int WL_PlayDemoCommand(struct wg_level *level,
                       const struct wg_view_tables *tables,
                       wl_play_state_t *state,
                       const wl_demo_command_t *command);
size_t WL_MusicChunkForMap(unsigned map_number);
size_t WL_MusicChunkForVariant(wg_game_variant_t variant,
                               unsigned map_number);
int WL_DrawPaused(uint8_t *framebuffer, const struct wg_graphics *graphics);
void WL_UpdatePaletteShifts(struct wg_level *level,
                            uint8_t palette[256 * 3]);
void WL_UpdatePaletteShiftsForTics(struct wg_level *level,
                                   uint8_t palette[256 * 3],
                                   unsigned tics);
int WL_PlayTick(struct wg_level *level,
                const struct wg_view_tables *tables,
                wl_play_state_t *state, const wl_input_t *input);
int WL_PlayTicks(struct wg_level *level,
                 const struct wg_view_tables *tables,
                 wl_play_state_t *state, const wl_input_t *input,
                 unsigned tics);

#endif
