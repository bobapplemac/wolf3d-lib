#include "wg_raycast.h"

#include <stddef.h>

#include "wg_fixed.h"

static int WG_FixedTile(int32_t value)
{
    if (value >= 0)
    {
        return value / WG_FIXED_ONE;
    }
    return -(int)((-(int64_t)value + WG_FIXED_ONE - 1) / WG_FIXED_ONE);
}

static uint8_t WG_LevelTile(const wg_level_t *level, int x, int y)
{
    return level->tiles[(size_t)y * WG_LEVEL_SIZE + (size_t)x];
}

static void WG_RecordHit(const wg_view_tables_t *tables,
                         wg_wall_hit_t *hit, uint8_t tile,
                         wg_wall_side_t side, int map_x, int map_y,
                         int x_tile_step, int y_tile_step,
                         int32_t x_intercept, int32_t y_intercept,
                         int32_t view_x, int32_t view_y,
                         int32_t view_cosine, int32_t view_sine)
{
    unsigned texture;
    int32_t distance;

    if (side == WG_WALL_VERTICAL)
    {
        texture = ((uint32_t)y_intercept >> 10) & 63U;
        if (x_tile_step == -1)
        {
            texture = 63U - texture;
            x_intercept += WG_FIXED_ONE;
        }
        hit->wall_page = (uint16_t)((tile - 1U) * 2U + 1U);
    }
    else
    {
        texture = ((uint32_t)x_intercept >> 10) & 63U;
        if (y_tile_step == -1)
        {
            y_intercept += WG_FIXED_ONE;
        }
        else
        {
            texture = 63U - texture;
        }
        hit->wall_page = (uint16_t)((tile - 1U) * 2U);
    }

    distance = WG_FixedMul(x_intercept - view_x, view_cosine)
               - WG_FixedMul(y_intercept - view_y, view_sine);
    if (distance < WG_MIN_DISTANCE)
    {
        distance = WG_MIN_DISTANCE;
    }
    hit->x = x_intercept;
    hit->y = y_intercept;
    hit->height = tables->height_numerator / (distance / 256);
    hit->texture_column = (uint8_t)texture;
    hit->tile = tile;
    hit->map_x = (uint8_t)map_x;
    hit->map_y = (uint8_t)map_y;
    hit->side = side;
}

int WG_RaycastStaticWalls(const wg_level_t *level,
                          const wg_view_tables_t *tables,
                          int32_t player_x, int32_t player_y,
                          uint16_t player_angle,
                          wg_wall_hit_t hits[WG_MAX_VIEW_WIDTH])
{
    const int32_t *cosine;
    int32_t view_sine;
    int32_t view_cosine;
    int32_t view_x;
    int32_t view_y;
    int32_t x_partial_down;
    int32_t x_partial_up;
    int32_t y_partial_down;
    int32_t y_partial_up;
    int focal_x;
    int focal_y;
    int pixel;

    if (level == NULL || tables == NULL || hits == NULL
        || tables->view_width < 2U
        || tables->view_width > WG_MAX_VIEW_WIDTH
        || player_angle >= WG_ANGLES)
    {
        return 0;
    }
    cosine = WG_ViewCosineTable(tables);
    view_sine = tables->sine[player_angle];
    view_cosine = cosine[player_angle];
    view_x = player_x - WG_FixedMul(tables->focal_length, view_cosine);
    view_y = player_y + WG_FixedMul(tables->focal_length, view_sine);
    focal_x = WG_FixedTile(view_x);
    focal_y = WG_FixedTile(view_y);
    if (focal_x < 0 || focal_x >= WG_LEVEL_SIZE
        || focal_y < 0 || focal_y >= WG_LEVEL_SIZE)
    {
        return 0;
    }
    x_partial_down = (int32_t)((uint32_t)view_x & 0xffffU);
    x_partial_up = WG_FIXED_ONE - x_partial_down;
    y_partial_down = (int32_t)((uint32_t)view_y & 0xffffU);
    y_partial_up = WG_FIXED_ONE - y_partial_down;

    for (pixel = 0; pixel < tables->view_width; ++pixel)
    {
        int angle = (int)player_angle * (WG_FINE_ANGLES / WG_ANGLES)
                    + tables->pixel_angle[pixel];
        int x_tile_step;
        int y_tile_step;
        int32_t x_step;
        int32_t y_step;
        int32_t x_partial;
        int32_t y_partial;
        int32_t x_intercept;
        int32_t y_intercept;
        int x_tile;
        int y_tile;
        int vertical_entry = 1;
        int iterations;

        if (angle < 0)
        {
            angle += WG_FINE_ANGLES;
        }
        if (angle >= WG_FINE_ANGLES)
        {
            angle -= WG_FINE_ANGLES;
        }
        if (angle < 900)
        {
            x_tile_step = 1;
            y_tile_step = -1;
            x_step = tables->fine_tangent[899 - angle];
            y_step = -tables->fine_tangent[angle];
            x_partial = x_partial_up;
            y_partial = y_partial_down;
        }
        else if (angle < 1800)
        {
            x_tile_step = -1;
            y_tile_step = -1;
            x_step = -tables->fine_tangent[angle - 900];
            y_step = -tables->fine_tangent[1799 - angle];
            x_partial = x_partial_down;
            y_partial = y_partial_down;
        }
        else if (angle < 2700)
        {
            x_tile_step = -1;
            y_tile_step = 1;
            x_step = -tables->fine_tangent[2699 - angle];
            y_step = tables->fine_tangent[angle - 1800];
            x_partial = x_partial_down;
            y_partial = y_partial_up;
        }
        else
        {
            x_tile_step = 1;
            y_tile_step = 1;
            x_step = tables->fine_tangent[angle - 2700];
            y_step = tables->fine_tangent[3599 - angle];
            x_partial = x_partial_up;
            y_partial = y_partial_up;
        }

        y_intercept = WG_FixedMul(y_step, x_partial) + view_y;
        x_tile = focal_x + x_tile_step;
        x_intercept = WG_FixedMul(x_step, y_partial) + view_x;
        y_tile = focal_y + y_tile_step;

        for (iterations = 0; iterations < WG_LEVEL_SIZE * 2; ++iterations)
        {
            int map_x;
            int map_y;
            uint8_t tile;

            if (vertical_entry)
            {
                if ((y_tile_step == -1 && WG_FixedTile(y_intercept) <= y_tile)
                    || (y_tile_step == 1
                        && WG_FixedTile(y_intercept) >= y_tile))
                {
                    vertical_entry = 0;
                    --iterations;
                    continue;
                }
                map_x = x_tile;
                map_y = WG_FixedTile(y_intercept);
                if (map_x < 0 || map_x >= WG_LEVEL_SIZE
                    || map_y < 0 || map_y >= WG_LEVEL_SIZE)
                {
                    return 0;
                }
                tile = WG_LevelTile(level, map_x, map_y);
                if (tile != 0U)
                {
                    if (tile >= 64U)
                    {
                        return 0;
                    }
                    x_intercept = x_tile * WG_FIXED_ONE;
                    WG_RecordHit(tables, &hits[pixel], tile,
                                 WG_WALL_VERTICAL, map_x, map_y,
                                 x_tile_step, y_tile_step,
                                 x_intercept, y_intercept,
                                 view_x, view_y, view_cosine, view_sine);
                    break;
                }
                x_tile += x_tile_step;
                y_intercept += y_step;
            }
            else
            {
                if ((x_tile_step == -1 && WG_FixedTile(x_intercept) <= x_tile)
                    || (x_tile_step == 1
                        && WG_FixedTile(x_intercept) >= x_tile))
                {
                    vertical_entry = 1;
                    --iterations;
                    continue;
                }
                map_x = WG_FixedTile(x_intercept);
                map_y = y_tile;
                if (map_x < 0 || map_x >= WG_LEVEL_SIZE
                    || map_y < 0 || map_y >= WG_LEVEL_SIZE)
                {
                    return 0;
                }
                tile = WG_LevelTile(level, map_x, map_y);
                if (tile != 0U)
                {
                    if (tile >= 64U)
                    {
                        return 0;
                    }
                    y_intercept = y_tile * WG_FIXED_ONE;
                    WG_RecordHit(tables, &hits[pixel], tile,
                                 WG_WALL_HORIZONTAL, map_x, map_y,
                                 x_tile_step, y_tile_step,
                                 x_intercept, y_intercept,
                                 view_x, view_y, view_cosine, view_sine);
                    break;
                }
                y_tile += y_tile_step;
                x_intercept += x_step;
            }
        }
        if (iterations == WG_LEVEL_SIZE * 2)
        {
            return 0;
        }
    }
    return 1;
}
