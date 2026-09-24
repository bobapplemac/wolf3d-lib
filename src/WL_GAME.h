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

typedef enum wg_item_type
{
    WG_ITEM_NONE = 0,
    WG_ITEM_ALPO,
    WG_ITEM_KEY1,
    WG_ITEM_KEY2,
    WG_ITEM_FOOD,
    WG_ITEM_FIRSTAID,
    WG_ITEM_CLIP,
    WG_ITEM_MACHINEGUN,
    WG_ITEM_CHAINGUN,
    WG_ITEM_CROSS,
    WG_ITEM_CHALICE,
    WG_ITEM_BIBLE,
    WG_ITEM_CROWN,
    WG_ITEM_FULLHEAL,
    WG_ITEM_GIBS,
    WG_ITEM_CLIP2
} wg_item_type_t;

typedef struct wg_static_object
{
    uint8_t tile_x;
    uint8_t tile_y;
    uint8_t blocking;
    uint8_t removed;
    uint16_t shape;
    wg_item_type_t item;
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

typedef enum wg_door_action
{
    WG_DOOR_CLOSED = 0,
    WG_DOOR_OPENING,
    WG_DOOR_OPEN,
    WG_DOOR_CLOSING
} wg_door_action_t;

typedef struct wg_door
{
    uint16_t position;
    uint16_t tic_count;
    uint8_t tile_x;
    uint8_t tile_y;
    uint8_t vertical;
    wg_door_lock_t lock;
    wg_door_action_t action;
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
    int32_t player_angle_fraction;
    uint16_t player_angle;
    uint8_t player_tile_x;
    uint8_t player_tile_y;
    wg_difficulty_t difficulty;
    int32_t player_thrust_speed;
    uint16_t player_health;
    uint16_t player_ammo;
    uint8_t player_keys;
    uint8_t player_lives;
    uint16_t damage_count;
    uint16_t bonus_count;
    uint8_t player_dead;
    uint8_t player_weapon;
    uint8_t player_chosen_weapon;
    uint8_t player_best_weapon;
    uint8_t weapon_frame;
    uint8_t attack_frame;
    uint8_t attack_active;
    int32_t attack_count;
    uint8_t made_noise;
    uint8_t victory_flag;
    uint8_t level_completed;
    uint8_t secret_level;
    uint32_t score;
    uint32_t next_extra;
    uint16_t kill_count;
    uint16_t treasure_count;
    int32_t kill_x;
    int32_t kill_y;
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
