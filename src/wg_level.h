#ifndef WG_LEVEL_H
#define WG_LEVEL_H

#include <stdint.h>

#include "wg_maps.h"

#define WG_LEVEL_SIZE 64
#define WG_AREA_TILE 107U
#define WG_MAX_DOORS 64

typedef enum wg_door_lock
{
    WG_DOOR_NORMAL = 0,
    WG_DOOR_LOCK_1,
    WG_DOOR_LOCK_2,
    WG_DOOR_LOCK_3,
    WG_DOOR_LOCK_4,
    WG_DOOR_ELEVATOR
} wg_door_lock_t;

typedef struct wg_door
{
    uint16_t position;
    uint8_t tile_x;
    uint8_t tile_y;
    uint8_t vertical;
    wg_door_lock_t lock;
} wg_door_t;

typedef struct wg_level
{
    uint8_t tiles[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    uint16_t info[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    int32_t player_x;
    int32_t player_y;
    uint16_t player_angle;
    uint8_t player_tile_x;
    uint8_t player_tile_y;
    wg_door_t doors[WG_MAX_DOORS];
    uint8_t door_count;
} wg_level_t;

int WG_LevelBuild(const wg_map_t *map, wg_level_t *level);

#endif
