#ifndef WL_AGENT_H
#define WL_AGENT_H

#include <stdint.h>

#include "ID_VL.h"
#include "WG_GRAPHICS.h"

typedef struct wl_status
{
    uint32_t score;
    uint16_t health;
    uint16_t ammo;
    uint8_t map;
    uint8_t lives;
    uint8_t weapon;
    uint8_t keys;
    uint8_t face_frame;
} wl_status_t;

struct wg_level;

void WL_StatusDefaults(wl_status_t *status);
void WL_TakeDamage(struct wg_level *level, unsigned points);
int WL_DrawStatusBar(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    const wg_graphics_t *graphics, const wl_status_t *status);

#endif
