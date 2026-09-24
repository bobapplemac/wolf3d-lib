#ifndef WG_RENDERER_H
#define WG_RENDERER_H

#include <stddef.h>
#include <stdint.h>

#include "WL_GAME.h"
#include "WL_DRAW.h"
#include "ID_PM.h"
#include "ID_VL.h"
#include "WL_MAIN.h"

#define WG_PLAY_VIEW_HEIGHT 160

typedef struct wg_wall_cache
{
    uint8_t *pixels;
    size_t count;
} wg_wall_cache_t;

int WG_WallCacheLoad(wg_wall_cache_t *cache, const wg_pages_t *pages);
void WG_WallCacheFree(wg_wall_cache_t *cache);

int WG_RenderStaticView(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    const wg_level_t *level, const wg_view_tables_t *tables,
    const wg_wall_cache_t *walls, unsigned episode, unsigned map,
    int32_t player_x, int32_t player_y, uint16_t player_angle,
    wg_wall_hit_t hits[WG_MAX_VIEW_WIDTH],
    uint8_t visible_tiles[WG_LEVEL_SIZE * WG_LEVEL_SIZE]);

#endif
