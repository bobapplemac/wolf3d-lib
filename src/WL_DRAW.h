#ifndef WL_DRAW_H
#define WL_DRAW_H

#include <stdint.h>

#include "WL_GAME.h"
#include "WL_MAIN.h"
#include "ID_PM.h"
#include "ID_VL.h"

typedef enum wg_wall_side
{
    WG_WALL_HORIZONTAL = 0,
    WG_WALL_VERTICAL = 1,
    WG_WALL_DOOR = 2
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

int WG_RaycastWalls(const wg_level_t *level,
                    const wg_view_tables_t *tables,
                    int32_t player_x, int32_t player_y,
                    uint16_t player_angle, uint16_t door_wall_base,
                    wg_wall_hit_t hits[WG_MAX_VIEW_WIDTH]);

int WL_DrawPlayerWeapon(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    const wg_pages_t *pages, unsigned weapon, unsigned weapon_frame);

#endif
