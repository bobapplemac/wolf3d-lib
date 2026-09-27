/* Portable AsmRefresh and wall/door hit paths from WL_DR_A.ASM/WL_DRAW.C. */
#include "WL_DRAW.h"

#include <stddef.h>
#include <string.h>

#include "WG_FIXED.h"
#include "WG_ASSETS.h"
#include "WL_SCALE.h"

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

static int32_t WG_HalfFloor(int32_t value)
{
    if (value >= 0)
    {
        return value / 2;
    }
    return (value - 1) / 2;
}

static int32_t WG_PushWallStepOffset(int32_t step, uint8_t position)
{
    int64_t product = (int64_t)step * position;

    if (product >= 0)
    {
        return (int32_t)(product / 64);
    }
    return -(int32_t)((-product + 63) / 64);
}

static uint16_t WG_DoorPage(uint16_t base, wg_door_lock_t lock,
                            int vertical_hit)
{
    unsigned offset;

    if (lock == WG_DOOR_ELEVATOR)
    {
        offset = 4U;
    }
    else if (lock != WG_DOOR_NORMAL)
    {
        offset = 6U;
    }
    else
    {
        offset = 0U;
    }
    return (uint16_t)(base + offset + (vertical_hit ? 1U : 0U));
}

static void WG_RecordDoorHit(const wg_view_tables_t *tables,
                             wg_wall_hit_t *hit, uint8_t tile,
                             int map_x, int map_y, uint16_t wall_page,
                             unsigned texture, int32_t x_intercept,
                             int32_t y_intercept, int32_t view_x,
                             int32_t view_y, int32_t view_cosine,
                             int32_t view_sine)
{
    int32_t distance = WG_FixedMul(x_intercept - view_x, view_cosine)
                       - WG_FixedMul(y_intercept - view_y, view_sine);

    if (distance < WG_MIN_DISTANCE)
    {
        distance = WG_MIN_DISTANCE;
    }
    hit->x = x_intercept;
    hit->y = y_intercept;
    hit->height = tables->height_numerator / (distance / 256);
    hit->wall_page = wall_page;
    hit->texture_column = (uint8_t)texture;
    hit->tile = tile;
    hit->map_x = (uint8_t)map_x;
    hit->map_y = (uint8_t)map_y;
    hit->side = WG_WALL_DOOR;
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

static void WG_RecordPushWallHit(const wg_level_t *level,
                                 const wg_view_tables_t *tables,
                                 wg_wall_hit_t *hit, uint8_t tile,
                                 wg_wall_side_t side, int map_x, int map_y,
                                 int x_tile_step, int y_tile_step,
                                 int32_t x_intercept, int32_t y_intercept,
                                 int32_t view_x, int32_t view_y,
                                 int32_t view_cosine, int32_t view_sine)
{
    unsigned texture;
    int32_t offset = (int32_t)level->pushwall_position * 1024;
    int32_t distance;

    if (side == WG_WALL_VERTICAL)
    {
        texture = ((uint32_t)y_intercept >> 10) & 63U;
        if (x_tile_step == -1)
        {
            x_intercept += WG_FIXED_ONE - offset;
            texture = 63U - texture;
        }
        else
        {
            x_intercept += offset;
        }
        hit->wall_page = (uint16_t)(((tile & 63U) - 1U) * 2U + 1U);
    }
    else
    {
        texture = ((uint32_t)x_intercept >> 10) & 63U;
        if (y_tile_step == -1)
        {
            y_intercept += WG_FIXED_ONE - offset;
        }
        else
        {
            y_intercept += offset;
            texture = 63U - texture;
        }
        hit->wall_page = (uint16_t)(((tile & 63U) - 1U) * 2U);
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

static int WG_RaycastWallsInternal(
    const wg_level_t *level, const wg_view_tables_t *tables,
    int32_t player_x, int32_t player_y, uint16_t player_angle,
    uint16_t door_wall_base, wg_wall_hit_t hits[WG_MAX_VIEW_WIDTH],
    uint8_t visible_tiles[WG_LEVEL_SIZE * WG_LEVEL_SIZE])
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
    if (visible_tiles != NULL)
    {
        int player_tile_x = WG_FixedTile(player_x);
        int player_tile_y = WG_FixedTile(player_y);

        memset(visible_tiles, 0, WG_LEVEL_SIZE * WG_LEVEL_SIZE);
        if (player_tile_x >= 0 && player_tile_x < WG_LEVEL_SIZE
            && player_tile_y >= 0 && player_tile_y < WG_LEVEL_SIZE)
        {
            visible_tiles[(size_t)player_tile_y * WG_LEVEL_SIZE
                          + (size_t)player_tile_x] = 1U;
        }
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
                    if ((tile & 0xc0U) == 0xc0U)
                    {
                        int32_t adjusted = y_intercept
                            + WG_PushWallStepOffset(
                                y_step, level->pushwall_position);

                        /* WL_DR_A.ASM advances the ray intersection by the
                           same fraction that the pushwall has moved. If that
                           leaves this map tile, the ray struck the stationary
                           tile edge before reaching the translated wall and
                           must continue tracing. */
                        if (WG_FixedTile(adjusted) != map_y)
                        {
                            if (visible_tiles != NULL)
                            {
                                visible_tiles[(size_t)map_y * WG_LEVEL_SIZE
                                              + (size_t)map_x] = 1U;
                            }
                            x_tile += x_tile_step;
                            y_intercept += y_step;
                            continue;
                        }
                        y_intercept = adjusted;
                        x_intercept = x_tile * WG_FIXED_ONE;
                        WG_RecordPushWallHit(
                            level, tables, &hits[pixel], tile,
                            WG_WALL_VERTICAL, map_x, map_y,
                            x_tile_step, y_tile_step,
                            x_intercept, y_intercept,
                            view_x, view_y, view_cosine, view_sine);
                        break;
                    }
                    if ((tile & 0x80U) != 0U)
                    {
                        unsigned door_index = tile & 0x7fU;
                        const wg_door_t *door;
                        int32_t adjusted;

                        if (door_index >= level->door_count)
                        {
                            return 0;
                        }
                        door = &level->doors[door_index];
                        adjusted = y_intercept + WG_HalfFloor(y_step);
                        if (WG_FixedTile(adjusted)
                            != WG_FixedTile(y_intercept)
                            || (uint16_t)adjusted < door->position)
                        {
                            if (visible_tiles != NULL)
                            {
                                visible_tiles[(size_t)map_y * WG_LEVEL_SIZE
                                              + (size_t)map_x] = 1U;
                            }
                            x_tile += x_tile_step;
                            y_intercept += y_step;
                            continue;
                        }
                        y_intercept = adjusted;
                        x_intercept = x_tile * WG_FIXED_ONE
                                      + WG_FIXED_ONE / 2;
                        WG_RecordDoorHit(
                            tables, &hits[pixel], tile, map_x, map_y,
                            WG_DoorPage(door_wall_base, door->lock, 1),
                            ((uint32_t)(y_intercept - door->position) >> 10)
                            & 63U,
                            x_intercept, y_intercept, view_x, view_y,
                            view_cosine, view_sine);
                        break;
                    }
                    if (tile == 64U)
                    {
                        return 0;
                    }
                    x_intercept = x_tile * WG_FIXED_ONE;
                    WG_RecordHit(tables, &hits[pixel], tile & 0x3fU,
                                 WG_WALL_VERTICAL, map_x, map_y,
                                 x_tile_step, y_tile_step,
                                 x_intercept, y_intercept,
                                 view_x, view_y, view_cosine, view_sine);
                    hits[pixel].tile = tile;
                    if ((tile & 0x40U) != 0U
                        && (WG_LevelTile(level, map_x - x_tile_step, map_y)
                            & 0x80U) != 0U)
                    {
                        hits[pixel].wall_page = (uint16_t)(door_wall_base + 3U);
                    }
                    break;
                }
                if (visible_tiles != NULL)
                {
                    visible_tiles[(size_t)map_y * WG_LEVEL_SIZE
                                  + (size_t)map_x] = 1U;
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
                    if ((tile & 0xc0U) == 0xc0U)
                    {
                        int32_t adjusted = x_intercept
                            + WG_PushWallStepOffset(
                                x_step, level->pushwall_position);

                        if (WG_FixedTile(adjusted) != map_x)
                        {
                            if (visible_tiles != NULL)
                            {
                                visible_tiles[(size_t)map_y * WG_LEVEL_SIZE
                                              + (size_t)map_x] = 1U;
                            }
                            y_tile += y_tile_step;
                            x_intercept += x_step;
                            continue;
                        }
                        x_intercept = adjusted;
                        y_intercept = y_tile * WG_FIXED_ONE;
                        WG_RecordPushWallHit(
                            level, tables, &hits[pixel], tile,
                            WG_WALL_HORIZONTAL, map_x, map_y,
                            x_tile_step, y_tile_step,
                            x_intercept, y_intercept,
                            view_x, view_y, view_cosine, view_sine);
                        break;
                    }
                    if ((tile & 0x80U) != 0U)
                    {
                        unsigned door_index = tile & 0x7fU;
                        const wg_door_t *door;
                        int32_t adjusted;

                        if (door_index >= level->door_count)
                        {
                            return 0;
                        }
                        door = &level->doors[door_index];
                        adjusted = x_intercept + WG_HalfFloor(x_step);
                        if (WG_FixedTile(adjusted)
                            != WG_FixedTile(x_intercept)
                            || (uint16_t)adjusted < door->position)
                        {
                            if (visible_tiles != NULL)
                            {
                                visible_tiles[(size_t)map_y * WG_LEVEL_SIZE
                                              + (size_t)map_x] = 1U;
                            }
                            y_tile += y_tile_step;
                            x_intercept += x_step;
                            continue;
                        }
                        x_intercept = adjusted;
                        y_intercept = y_tile * WG_FIXED_ONE
                                      + WG_FIXED_ONE / 2;
                        WG_RecordDoorHit(
                            tables, &hits[pixel], tile, map_x, map_y,
                            WG_DoorPage(door_wall_base, door->lock, 0),
                            ((uint32_t)(x_intercept - door->position) >> 10)
                            & 63U,
                            x_intercept, y_intercept, view_x, view_y,
                            view_cosine, view_sine);
                        break;
                    }
                    if (tile == 64U)
                    {
                        return 0;
                    }
                    y_intercept = y_tile * WG_FIXED_ONE;
                    WG_RecordHit(tables, &hits[pixel], tile & 0x3fU,
                                 WG_WALL_HORIZONTAL, map_x, map_y,
                                 x_tile_step, y_tile_step,
                                 x_intercept, y_intercept,
                                 view_x, view_y, view_cosine, view_sine);
                    hits[pixel].tile = tile;
                    if ((tile & 0x40U) != 0U
                        && (WG_LevelTile(level, map_x, map_y - y_tile_step)
                            & 0x80U) != 0U)
                    {
                        hits[pixel].wall_page = (uint16_t)(door_wall_base + 2U);
                    }
                    break;
                }
                if (visible_tiles != NULL)
                {
                    visible_tiles[(size_t)map_y * WG_LEVEL_SIZE
                                  + (size_t)map_x] = 1U;
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

int WG_RaycastWalls(const wg_level_t *level,
                    const wg_view_tables_t *tables,
                    int32_t player_x, int32_t player_y,
                    uint16_t player_angle, uint16_t door_wall_base,
                    wg_wall_hit_t hits[WG_MAX_VIEW_WIDTH])
{
    return WG_RaycastWallsInternal(level, tables, player_x, player_y,
                                   player_angle, door_wall_base, hits, NULL);
}

int WG_RaycastWallsVisible(
    const wg_level_t *level, const wg_view_tables_t *tables,
    int32_t player_x, int32_t player_y, uint16_t player_angle,
    uint16_t door_wall_base, wg_wall_hit_t hits[WG_MAX_VIEW_WIDTH],
    uint8_t visible_tiles[WG_LEVEL_SIZE * WG_LEVEL_SIZE])
{
    if (visible_tiles == NULL)
    {
        return 0;
    }
    return WG_RaycastWallsInternal(level, tables, player_x, player_y,
                                   player_angle, door_wall_base, hits,
                                   visible_tiles);
}

int WG_RaycastStaticWalls(const wg_level_t *level,
                          const wg_view_tables_t *tables,
                          int32_t player_x, int32_t player_y,
                          uint16_t player_angle,
                          wg_wall_hit_t hits[WG_MAX_VIEW_WIDTH])
{
    return WG_RaycastWalls(level, tables, player_x, player_y, player_angle,
                           0, hits);
}

typedef struct wg_visible_object
{
    int view_x;
    int32_t view_height;
    int32_t trans_x;
    uint16_t shape;
} wg_visible_object_t;

static int WG_TransformTile(const wg_view_tables_t *tables,
                            uint8_t tile_x, uint8_t tile_y,
                            int32_t view_x, int32_t view_y,
                            int32_t view_cosine, int32_t view_sine,
                            wg_visible_object_t *visible)
{
    int32_t gx = (int32_t)tile_x * WG_FIXED_ONE + WG_FIXED_ONE / 2 - view_x;
    int32_t gy = (int32_t)tile_y * WG_FIXED_ONE + WG_FIXED_ONE / 2 - view_y;
    int32_t gxt = WG_FixedMul(gx, view_cosine);
    int32_t gyt = WG_FixedMul(gy, view_sine);
    int32_t nx = gxt - gyt - 0x2000;
    int32_t ny;

    if (nx < WG_MIN_DISTANCE)
    {
        return 0;
    }
    gxt = WG_FixedMul(gx, view_sine);
    gyt = WG_FixedMul(gy, view_cosine);
    ny = gyt + gxt;
    visible->view_x = tables->view_width / 2
                      + (int)((int64_t)ny * tables->scale / nx);
    visible->view_height = tables->height_numerator / (nx / 256);
    visible->trans_x = nx;
    return visible->view_height > 0;
}

static int WG_TransformActor(const wg_view_tables_t *tables,
                             const wg_actor_t *actor,
                             int32_t view_x, int32_t view_y,
                             int32_t view_cosine, int32_t view_sine,
                             wg_visible_object_t *visible)
{
    int32_t gx = actor->x - view_x;
    int32_t gy = actor->y - view_y;
    int32_t gxt = WG_FixedMul(gx, view_cosine);
    int32_t gyt = WG_FixedMul(gy, view_sine);
    int32_t nx = gxt - gyt - 0x4000;
    int32_t ny;

    if (nx < WG_MIN_DISTANCE)
    {
        return 0;
    }
    gxt = WG_FixedMul(gx, view_sine);
    gyt = WG_FixedMul(gy, view_cosine);
    ny = gyt + gxt;
    visible->view_x = tables->view_width / 2
                      + (int)((int64_t)ny * tables->scale / nx);
    visible->view_height = tables->height_numerator / (nx / 256);
    visible->trans_x = nx;
    return visible->view_height > 0;
}

static int WG_ActorTileIsVisible(
    const wg_level_t *level,
    const uint8_t visible_tiles[WG_LEVEL_SIZE * WG_LEVEL_SIZE],
    int tile_x, int tile_y)
{
    int offset_x;
    int offset_y;

    if (visible_tiles[(size_t)tile_y * WG_LEVEL_SIZE + (size_t)tile_x] != 0U)
    {
        return 1;
    }
    for (offset_y = -1; offset_y <= 1; ++offset_y)
    {
        for (offset_x = -1; offset_x <= 1; ++offset_x)
        {
            int x = tile_x + offset_x;
            int y = tile_y + offset_y;
            size_t tile_index;

            if ((offset_x == 0 && offset_y == 0)
                || x < 0 || x >= WG_LEVEL_SIZE
                || y < 0 || y >= WG_LEVEL_SIZE)
            {
                continue;
            }
            tile_index = (size_t)y * WG_LEVEL_SIZE + (size_t)x;
            if (visible_tiles[tile_index] != 0U
                && level->tiles[tile_index] == 0U)
            {
                return 1;
            }
        }
    }
    return 0;
}

static uint16_t WG_ActorShape(const wg_actor_t *actor, int projected_x,
                              uint16_t player_angle, int center_x)
{
    int view_angle = (int)player_angle + (center_x - projected_x) / 8;
    int actor_angle = actor->actor_class == WG_ACTOR_ROCKET
                          ? actor->angle
                          : actor->direction * (WG_ANGLES / 8);
    int angle = (view_angle - 180) - actor_angle;

    if (actor->rotate == 0U)
    {
        return actor->shape;
    }
    angle += WG_ANGLES / 16;
    while (angle >= WG_ANGLES)
    {
        angle -= WG_ANGLES;
    }
    while (angle < 0)
    {
        angle += WG_ANGLES;
    }
    if (actor->rotate == 2U)
    {
        return (uint16_t)(actor->shape
                          + 4 * (angle / (WG_ANGLES / 2)));
    }
    return (uint16_t)(actor->shape + angle / (WG_ANGLES / 8));
}

int WL_DrawScaleds(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    const wg_pages_t *pages, wg_level_t *level,
    const wg_view_tables_t *tables,
    const wg_wall_hit_t hits[WG_MAX_VIEW_WIDTH],
    const uint8_t visible_tiles[WG_LEVEL_SIZE * WG_LEVEL_SIZE],
    int32_t player_x, int32_t player_y, uint16_t player_angle)
{
    wg_visible_object_t visible[50];
    int32_t wall_height[WG_MAX_VIEW_WIDTH];
    const int32_t *cosine;
    int32_t view_cosine;
    int32_t view_sine;
    int32_t view_x;
    int32_t view_y;
    int screen_x;
    int screen_y;
    int view_height;
    size_t visible_count = 0;
    size_t index;

    if (framebuffer == NULL || pages == NULL || level == NULL
        || tables == NULL || hits == NULL || visible_tiles == NULL
        || tables->view_width < 64U
        || tables->view_width > WG_VIDEO_WIDTH
        || (tables->view_width & 15U) != 0U
        || player_angle >= WG_ANGLES)
    {
        return 0;
    }
    cosine = WG_ViewCosineTable(tables);
    view_cosine = cosine[player_angle];
    view_sine = tables->sine[player_angle];
    view_x = player_x - WG_FixedMul(tables->focal_length, view_cosine);
    view_y = player_y + WG_FixedMul(tables->focal_length, view_sine);
    view_height = tables->view_width / 2;
    screen_x = (WG_VIDEO_WIDTH - tables->view_width) / 2;
    screen_y = (160 - view_height) / 2;
    level->view_width = tables->view_width;
    for (index = 0; index < tables->view_width; ++index)
    {
        wall_height[index] = hits[index].height;
    }
    for (index = 0; index < level->static_count; ++index)
    {
        wg_visible_object_t candidate;
        size_t tile_index;

        if (level->statics[index].removed != 0U)
        {
            continue;
        }
        tile_index = (size_t)level->statics[index].tile_y * WG_LEVEL_SIZE
                     + level->statics[index].tile_x;

        if (visible_tiles[tile_index] != 0U
            && WG_TransformTile(tables, level->statics[index].tile_x,
                                level->statics[index].tile_y, view_x, view_y,
                                view_cosine, view_sine, &candidate))
        {
            candidate.shape = level->statics[index].shape;
            if (visible_count < sizeof(visible) / sizeof(visible[0]))
            {
                visible[visible_count++] = candidate;
            }
        }
    }
    for (index = 0; index < level->actor_count; ++index)
    {
        wg_actor_t *actor = &level->actors[index];
        wg_visible_object_t candidate;

        if ((actor->flags & WG_ACTOR_FLAG_REMOVED) != 0U)
        {
            continue;
        }
        actor->flags = (uint16_t)(actor->flags & 0xfff7U);
        if (WG_ActorTileIsVisible(level, visible_tiles,
                                  actor->tile_x, actor->tile_y)
            && WG_TransformActor(tables, actor, view_x, view_y,
                                 view_cosine, view_sine, &candidate))
        {
            actor->flags |= WG_ACTOR_FLAG_VISIBLE;
            actor->view_x = candidate.view_x;
            actor->trans_x = candidate.trans_x;
            candidate.shape = WG_ActorShape(
                actor, candidate.view_x, player_angle,
                tables->view_width / 2);
            if (visible_count < sizeof(visible) / sizeof(visible[0]))
            {
                visible[visible_count++] = candidate;
            }
        }
    }

    for (index = 0; index < visible_count; ++index)
    {
        size_t search;
        size_t farthest = index;
        wg_sprite_image_t sprite;

        for (search = index + 1; search < visible_count; ++search)
        {
            if (visible[search].view_height
                < visible[farthest].view_height)
            {
                farthest = search;
            }
        }
        if (farthest != index)
        {
            wg_visible_object_t temporary = visible[index];

            visible[index] = visible[farthest];
            visible[farthest] = temporary;
        }
        if (!WG_DecodeSprite(pages, visible[index].shape, &sprite)
            || !WG_ScaleSpriteClipped(
                framebuffer, screen_x, screen_y, tables->view_width,
                view_height,
                visible[index].view_x, &sprite,
                (unsigned)visible[index].view_height, wall_height))
        {
            return 0;
        }
    }
    return 1;
}

int WL_DrawPlayerWeapon(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    const wg_pages_t *pages, unsigned weapon, unsigned weapon_frame,
    unsigned view_width)
{
    wg_sprite_image_t sprite;
    size_t sprite_count;
    size_t shape;

    if (framebuffer == NULL || pages == NULL || pages->data_set == NULL
        || weapon > 3U || weapon_frame > 4U
        || view_width < 64U || view_width > WG_VIDEO_WIDTH
        || (view_width & 15U) != 0U)
    {
        return 0;
    }
    sprite_count = (size_t)(pages->data_set->sound_start
                            - pages->data_set->sprite_start);
    if (sprite_count < 20U)
    {
        return 0;
    }
    shape = sprite_count - 20U + weapon * 5U + weapon_frame;
    return WG_DecodeSprite(pages, shape, &sprite)
           && WG_ScaleSprite(framebuffer,
                             (WG_VIDEO_WIDTH - (int)view_width) / 2,
                             (160 - (int)view_width / 2) / 2,
                             (int)view_width, (int)view_width / 2,
                             (int)view_width / 2, &sprite,
                             view_width / 2U + 1U);
}
