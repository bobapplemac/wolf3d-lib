#ifndef WL_GAME_H
#define WL_GAME_H

#include <stdint.h>

#include "ID_US_1.h"
#include "WG_MAPS.h"
#include "WL_ACT2.h"

#define WG_LEVEL_SIZE 64
#define WG_AMBUSH_TILE 106U
#define WG_AREA_TILE 107U
#define WG_NUM_AREAS 37
#define WG_NO_AREA 0xffU
#define WG_MAX_DOORS 64
#define WG_MAX_STATICS 400

typedef enum wg_difficulty
{
    WG_DIFFICULTY_BABY = 0,
    WG_DIFFICULTY_EASY,
    WG_DIFFICULTY_MEDIUM,
    WG_DIFFICULTY_HARD
} wg_difficulty_t;

typedef struct wg_static_object
{
    uint8_t tile_x;
    uint8_t tile_y;
    uint8_t blocking;
    uint16_t shape;
} wg_static_object_t;

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
    uint8_t areas[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    uint8_t area_by_player[WG_NUM_AREAS];
    uint8_t ambush_tiles[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    uint16_t info[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    int32_t player_x;
    int32_t player_y;
    uint16_t player_angle;
    uint8_t player_tile_x;
    uint8_t player_tile_y;
    wg_difficulty_t difficulty;
    int32_t player_thrust_speed;
    uint16_t player_health;
    uint16_t damage_count;
    uint8_t player_dead;
    uint8_t player_best_weapon;
    uint8_t made_noise;
    uint32_t score;
    uint16_t kill_count;
    uint16_t secret_count;
    uint16_t pushwall_state;
    uint8_t pushwall_position;
    uint8_t pushwall_x;
    uint8_t pushwall_y;
    uint8_t pushwall_direction;
    wg_door_t doors[WG_MAX_DOORS];
    uint8_t door_count;
    wg_static_object_t statics[WG_MAX_STATICS];
    uint16_t static_count;
    wg_actor_t actors[WG_MAX_ACTORS];
    uint16_t actor_count;
    wg_random_t random;
} wg_level_t;

int WG_LevelBuild(const wg_map_t *map, wg_level_t *level);
int WG_LevelBuildForDifficulty(const wg_map_t *map, wg_difficulty_t difficulty,
                               wg_level_t *level);

#endif
