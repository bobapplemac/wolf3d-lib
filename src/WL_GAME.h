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
#define WG_MAX_SOUND_EVENTS 16

struct wg_view_tables;

typedef enum wg_sound
{
    WG_SOUND_NOWAY = 6,
    WG_SOUND_SCHABBS_THROW = 8,
    WG_SOUND_PLAYER_DEATH = 9,
    WG_SOUND_DOG_DEATH = 10,
    WG_SOUND_ATTACK_GATLING = 11,
    WG_SOUND_GET_KEY = 12,
    WG_SOUND_OPEN_DOOR = 18,
    WG_SOUND_CLOSE_DOOR = 19,
    WG_SOUND_GUARD_SIGHT = 21,
    WG_SOUND_DEATH_SCREAM_2 = 22,
    WG_SOUND_ATTACK_KNIFE = 23,
    WG_SOUND_ATTACK_PISTOL = 24,
    WG_SOUND_DEATH_SCREAM_3 = 25,
    WG_SOUND_ATTACK_MACHINEGUN = 26,
    WG_SOUND_SHOOT_DOOR = 28,
    WG_SOUND_DEATH_SCREAM_1 = 29,
    WG_SOUND_GET_MACHINEGUN = 30,
    WG_SOUND_GET_AMMO = 31,
    WG_SOUND_HEALTH_1 = 33,
    WG_SOUND_HEALTH_2 = 34,
    WG_SOUND_BONUS_1 = 35,
    WG_SOUND_BONUS_2 = 36,
    WG_SOUND_BONUS_3 = 37,
    WG_SOUND_GET_GATLING = 38,
    WG_SOUND_DOG_BARK = 41,
    WG_SOUND_BONUS_EXTRA_LIFE = 44,
    WG_SOUND_BONUS_4 = 45,
    WG_SOUND_PUSHWALL = 46,
    WG_SOUND_MUTTI = 50,
    WG_SOUND_SS_SIGHT = 51,
    WG_SOUND_AHHHG = 52,
    WG_SOUND_DIE = 53,
    WG_SOUND_EVA = 54,
    WG_SOUND_GUTENTAG = 55,
    WG_SOUND_LEBEN = 56,
    WG_SOUND_SCHEIST = 57,
    WG_SOUND_NAZI_FIRE = 58,
    WG_SOUND_BOSS_FIRE = 59,
    WG_SOUND_SS_FIRE = 60,
    WG_SOUND_SLURPIE = 61,
    WG_SOUND_TOT_HUND = 62,
    WG_SOUND_MEIN_GOTT = 63,
    WG_SOUND_SCHABBS_HA = 64,
    WG_SOUND_HITLER_HA = 65,
    WG_SOUND_OFFICER_SIGHT = 66,
    WG_SOUND_NEIN_SOWAS = 67,
    WG_SOUND_DOG_ATTACK = 68,
    WG_SOUND_FLAMETHROWER = 69,
    WG_SOUND_DEATH_SCREAM_4 = 73,
    WG_SOUND_DEATH_SCREAM_5 = 74,
    WG_SOUND_DEATH_SCREAM_6 = 75,
    WG_SOUND_DEATH_SCREAM_7 = 76,
    WG_SOUND_DEATH_SCREAM_8 = 77,
    WG_SOUND_DEATH_SCREAM_9 = 78,
    WG_SOUND_DONNER = 79,
    WG_SOUND_EINE = 80,
    WG_SOUND_ERLAUBEN = 81,
    WG_SOUND_KEIN = 82,
    WG_SOUND_MEIN = 83,
    WG_SOUND_ROSE = 84,
    WG_SOUND_MISSILE_FIRE = 85,
    WG_SOUND_MISSILE_HIT = 86
} wg_sound_t;

typedef struct wg_sound_event
{
    int32_t x;
    int32_t y;
    uint8_t sound;
    uint8_t positioned;
} wg_sound_event_t;

typedef struct wg_campaign_state
{
    uint32_t score;
    uint32_t next_extra;
    uint16_t health;
    uint16_t ammo;
    uint8_t lives;
    uint8_t weapon;
    uint8_t chosen_weapon;
    uint8_t best_weapon;
} wg_campaign_state_t;

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
    int32_t killer_x;
    int32_t killer_y;
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
    uint8_t shareware;
    uint8_t map_number;
    uint32_t score;
    uint32_t next_extra;
    uint32_t time_count;
    uint16_t kill_count;
    uint16_t kill_total;
    uint16_t treasure_count;
    uint16_t treasure_total;
    int32_t kill_x;
    int32_t kill_y;
    uint16_t secret_count;
    uint16_t secret_total;
    wg_sound_event_t sound_events[WG_MAX_SOUND_EVENTS];
    uint8_t sound_event_count;
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
int WG_QueueSound(wg_level_t *level, wg_sound_t sound);
unsigned WG_NextMapNumber(unsigned map_number, int secret_level);
void WG_CampaignCapture(wg_campaign_state_t *state,
                        const wg_level_t *level);
int WG_CampaignApply(wg_level_t *level,
                     const wg_campaign_state_t *state,
                     uint32_t level_start_score, int died);
int WG_QueueSoundAt(wg_level_t *level, wg_sound_t sound,
                    int32_t x, int32_t y);
int WG_SoundPosition(const wg_level_t *level,
                     const struct wg_view_tables *tables,
                     int32_t x, int32_t y,
                     uint8_t *left, uint8_t *right);
void WG_ClearSoundEvents(wg_level_t *level);
uint16_t WL_DeathTargetAngle(const wg_level_t *level);
int WL_DeathRotateStep(wg_level_t *level, uint16_t target_angle,
                       unsigned degrees);

#endif
