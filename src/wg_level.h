#ifndef WG_LEVEL_H
#define WG_LEVEL_H

#include <stdint.h>

#include "wg_maps.h"

#define WG_LEVEL_SIZE 64
#define WG_AREA_TILE 107U

typedef struct wg_level
{
    uint8_t tiles[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    uint16_t info[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    int32_t player_x;
    int32_t player_y;
    uint16_t player_angle;
    uint8_t player_tile_x;
    uint8_t player_tile_y;
} wg_level_t;

int WG_LevelBuild(const wg_map_t *map, wg_level_t *level);

#endif
