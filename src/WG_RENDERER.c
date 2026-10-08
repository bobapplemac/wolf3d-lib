#include "WG_RENDERER.h"

#include <stdlib.h>
#include <string.h>

#include "WG_ASSETS.h"
#include "WL_DRAW.h"
#include "WL_SCALE.h"

static const uint8_t wg_wolf_ceiling_colors[60] =
{
    0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0xbf,
    0x4e,0x4e,0x4e,0x1d,0x8d,0x4e,0x1d,0x2d,0x1d,0x8d,
    0x1d,0x1d,0x1d,0x1d,0x1d,0x2d,0xdd,0x1d,0x1d,0x98,
    0x1d,0x9d,0x2d,0xdd,0xdd,0x9d,0x2d,0x4d,0x1d,0xdd,
    0x7d,0x1d,0x2d,0x2d,0xdd,0xd7,0x1d,0x1d,0x1d,0x2d,
    0x1d,0x1d,0x1d,0x1d,0xdd,0xdd,0x7d,0xdd,0xdd,0xdd
};

static const uint8_t wg_spear_ceiling_colors[21] =
{
    0x6f, 0x4f, 0x1d, 0xde, 0xdf, 0x2e, 0x7f, 0x9e, 0xae, 0x7f,
    0x1d, 0xde, 0xdf, 0xde, 0xdf, 0xde, 0xe1, 0xdc, 0x2e, 0x1d,
    0xdc
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
    int32_t player_x, int32_t player_y, uint16_t player_angle,
    wg_wall_hit_t hits[WG_MAX_VIEW_WIDTH],
    uint8_t visible_tiles[WG_LEVEL_SIZE * WG_LEVEL_SIZE])
{
    const uint8_t *ceiling_colors;
    size_t ceiling_color_count;
    size_t color_index;
    int view_height;
    int view_x;
    int view_y;
    int pixel;

    if (WG_DataVariantFamily(level != NULL ? level->variant : WG_GAME_UNKNOWN)
        == WG_GAME_FAMILY_SPEAR)
    {
        ceiling_colors = wg_spear_ceiling_colors;
        ceiling_color_count = sizeof(wg_spear_ceiling_colors);
        color_index = map;
    }
    else
    {
        ceiling_colors = wg_wolf_ceiling_colors;
        ceiling_color_count = sizeof(wg_wolf_ceiling_colors);
        color_index = (size_t)episode * 10U + map;
    }

    if (framebuffer == NULL || level == NULL || tables == NULL
        || walls == NULL || walls->pixels == NULL || hits == NULL
        || visible_tiles == NULL
        || tables->view_width < 64U
        || tables->view_width > WG_VIDEO_WIDTH
        || (tables->view_width & 15U) != 0U
        || color_index >= ceiling_color_count
        || walls->count < 8U
        || !WG_RaycastWallsVisible(
            level, tables, player_x, player_y, player_angle,
            (uint16_t)(walls->count - 8U), hits, visible_tiles))
    {
        return 0;
    }
    view_height = tables->view_width / 2;
    view_x = (WG_VIDEO_WIDTH - tables->view_width) / 2;
    view_y = (WG_PLAY_VIEW_HEIGHT - view_height) / 2;
    memset(framebuffer, 0, WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT);
    if (!WL_DrawPlayBorder(framebuffer, tables->view_width))
    {
        return 0;
    }
    for (pixel = 0; pixel < view_height / 2; ++pixel)
    {
        memset(framebuffer + (view_y + pixel) * WG_VIDEO_WIDTH + view_x,
               ceiling_colors[color_index], tables->view_width);
    }
    for (; pixel < view_height; ++pixel)
    {
        memset(framebuffer + (view_y + pixel) * WG_VIDEO_WIDTH + view_x,
               0x19, tables->view_width);
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
        if (!WG_ScaleWallPost(framebuffer, view_x, view_y,
                              tables->view_width, view_height,
                              pixel, 1, texture,
                              hits[pixel].texture_column,
                              hits[pixel].height))
        {
            return 0;
        }
    }
    return 1;
}
