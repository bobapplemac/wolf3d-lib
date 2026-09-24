#include "WG_RENDERER.h"

#include <stdlib.h>
#include <string.h>

#include "WG_ASSETS.h"
#include "WL_DRAW.h"
#include "WL_SCALE.h"

static const uint8_t wg_ceiling_colors[60] =
{
    0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0xbf,
    0x4e,0x4e,0x4e,0x1d,0x8d,0x4e,0x1d,0x2d,0x1d,0x8d,
    0x1d,0x1d,0x1d,0x1d,0x1d,0x2d,0xdd,0x1d,0x1d,0x98,
    0x1d,0x9d,0x2d,0xdd,0xdd,0x9d,0x2d,0x4d,0x1d,0xdd,
    0x7d,0x1d,0x2d,0x2d,0xdd,0xd7,0x1d,0x1d,0x1d,0x2d,
    0x1d,0x1d,0x1d,0x1d,0xdd,0xdd,0x7d,0xdd,0xdd,0xdd
};

int WG_WallCacheLoad(wg_wall_cache_t *cache, const wg_pages_t *pages)
{
    size_t wall;

    if (cache == NULL || pages == NULL || pages->data_set == NULL)
    {
        return 0;
    }
    memset(cache, 0, sizeof(*cache));
    cache->count = pages->data_set->sprite_start;
    if (cache->count == 0U
        || cache->count > SIZE_MAX / (WG_TEXTURE_SIZE * WG_TEXTURE_SIZE))
    {
        cache->count = 0;
        return 0;
    }
    cache->pixels = (uint8_t *)calloc(
        cache->count, WG_TEXTURE_SIZE * WG_TEXTURE_SIZE);
    if (cache->pixels == NULL)
    {
        cache->count = 0;
        return 0;
    }
    for (wall = 0; wall < cache->count; ++wall)
    {
        if (pages->data_set->pages[wall].offset == 0U
            || pages->data_set->pages[wall].length == 0U)
        {
            continue;
        }
        if (!WG_DecodeWall(pages, wall,
                           cache->pixels
                           + wall * WG_TEXTURE_SIZE * WG_TEXTURE_SIZE))
        {
            WG_WallCacheFree(cache);
            return 0;
        }
    }
    return 1;
}

void WG_WallCacheFree(wg_wall_cache_t *cache)
{
    if (cache == NULL)
    {
        return;
    }
    free(cache->pixels);
    cache->pixels = NULL;
    cache->count = 0;
}

int WG_RenderStaticView(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    const wg_level_t *level, const wg_view_tables_t *tables,
    const wg_wall_cache_t *walls, unsigned episode, unsigned map,
    int32_t player_x, int32_t player_y, uint16_t player_angle)
{
    wg_wall_hit_t hits[WG_MAX_VIEW_WIDTH];
    size_t color_index = (size_t)episode * 10U + map;
    int pixel;

    if (framebuffer == NULL || level == NULL || tables == NULL
        || walls == NULL || walls->pixels == NULL
        || tables->view_width != WG_VIDEO_WIDTH
        || color_index >= sizeof(wg_ceiling_colors)
        || walls->count < 8U
        || !WG_RaycastWalls(level, tables, player_x, player_y,
                            player_angle, (uint16_t)(walls->count - 8U), hits))
    {
        return 0;
    }
    memset(framebuffer, 0, WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT);
    for (pixel = 0; pixel < WG_PLAY_VIEW_HEIGHT / 2; ++pixel)
    {
        memset(framebuffer + pixel * WG_VIDEO_WIDTH,
               wg_ceiling_colors[color_index], WG_VIDEO_WIDTH);
    }
    for (; pixel < WG_PLAY_VIEW_HEIGHT; ++pixel)
    {
        memset(framebuffer + pixel * WG_VIDEO_WIDTH, 0x19, WG_VIDEO_WIDTH);
    }

    for (pixel = 0; pixel < tables->view_width; ++pixel)
    {
        const uint8_t *texture;

        if (hits[pixel].wall_page >= walls->count)
        {
            return 0;
        }
        texture = walls->pixels
                  + hits[pixel].wall_page
                    * WG_TEXTURE_SIZE * WG_TEXTURE_SIZE;
        if (!WG_ScaleWallPost(framebuffer, 0, 0, WG_VIDEO_WIDTH,
                              WG_PLAY_VIEW_HEIGHT, pixel, 1, texture,
                              hits[pixel].texture_column,
                              hits[pixel].height))
        {
            return 0;
        }
    }
    return 1;
}
