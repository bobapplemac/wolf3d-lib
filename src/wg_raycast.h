#ifndef WG_RAYCAST_H
#define WG_RAYCAST_H

#include <stdint.h>

#include "wg_level.h"
#include "wg_view.h"

typedef enum wg_wall_side
{
    WG_WALL_HORIZONTAL = 0,
    WG_WALL_VERTICAL = 1
} wg_wall_side_t;

typedef struct wg_wall_hit
{
    int32_t x;
    int32_t y;
    int32_t height;
    uint16_t wall_page;
    uint8_t texture_column;
    uint8_t tile;
    uint8_t map_x;
    uint8_t map_y;
    wg_wall_side_t side;
} wg_wall_hit_t;

int WG_RaycastStaticWalls(const wg_level_t *level,
                          const wg_view_tables_t *tables,
                          int32_t player_x, int32_t player_y,
                          uint16_t player_angle,
                          wg_wall_hit_t hits[WG_MAX_VIEW_WIDTH]);

#endif
