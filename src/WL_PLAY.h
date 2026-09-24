#ifndef WL_PLAY_H
#define WL_PLAY_H

#include <stddef.h>
#include <stdint.h>

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
    int16_t mouse_x;
    int16_t mouse_y;
} wl_input_t;

typedef struct wl_play_state
{
    uint8_t attack_held;
    uint8_t use_held;
} wl_play_state_t;

void WL_PlayStateReset(wl_play_state_t *state);
size_t WL_MusicChunkForMap(unsigned map_number);
int WL_DrawPaused(uint8_t *framebuffer, const struct wg_graphics *graphics);
int WL_PlayTick(struct wg_level *level,
                const struct wg_view_tables *tables,
                wl_play_state_t *state, const wl_input_t *input);

#endif
