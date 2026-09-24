#ifndef WL_ACT2_H
#define WL_ACT2_H

#include <stdint.h>

#define WG_MAX_ACTORS 150

#define WG_ACTOR_FLAG_SHOOTABLE 0x0001U
#define WG_ACTOR_FLAG_VISIBLE 0x0008U
#define WG_ACTOR_FLAG_ATTACK_MODE 0x0010U
#define WG_ACTOR_FLAG_FIRST_ATTACK 0x0020U
#define WG_ACTOR_FLAG_AMBUSH 0x0040U
#define WG_ACTOR_FLAG_NONMARK 0x0080U
#define WG_ACTOR_FLAG_ATTACK_PENDING 0x0100U
#define WG_ACTOR_FLAG_REMOVED 0x0200U

typedef enum wg_actor_class
{
    WG_ACTOR_INERT = 0,
    WG_ACTOR_GUARD,
    WG_ACTOR_OFFICER,
    WG_ACTOR_SS,
    WG_ACTOR_DOG,
    WG_ACTOR_BOSS,
    WG_ACTOR_SCHABBS,
    WG_ACTOR_FAKE,
    WG_ACTOR_MECHA_HITLER,
    WG_ACTOR_MUTANT,
    WG_ACTOR_GHOST,
    WG_ACTOR_REAL_HITLER,
    WG_ACTOR_GRETEL,
    WG_ACTOR_GIFT,
    WG_ACTOR_FAT,
    WG_ACTOR_NEEDLE,
    WG_ACTOR_ROCKET,
    WG_ACTOR_SMOKE,
    WG_ACTOR_EXPLOSION,
    WG_ACTOR_FIRE
} wg_actor_class_t;

typedef enum wg_ghost_kind
{
    WG_GHOST_BLINKY = 0,
    WG_GHOST_CLYDE,
    WG_GHOST_PINKY,
    WG_GHOST_INKY
} wg_ghost_kind_t;

typedef enum wg_actor_state
{
    WG_STATE_NONE = 0,
    WG_STATE_STAND,
    WG_STATE_PATH1,
    WG_STATE_PATH1S,
    WG_STATE_PATH2,
    WG_STATE_PATH3,
    WG_STATE_PATH3S,
    WG_STATE_PATH4,
    WG_STATE_GHOST1,
    WG_STATE_CHASE1,
    WG_STATE_CHASE1S,
    WG_STATE_CHASE2,
    WG_STATE_CHASE3,
    WG_STATE_CHASE3S,
    WG_STATE_CHASE4,
    WG_STATE_SHOOT1,
    WG_STATE_SHOOT2,
    WG_STATE_SHOOT3,
    WG_STATE_SHOOT4,
    WG_STATE_SHOOT5,
    WG_STATE_SHOOT6,
    WG_STATE_SHOOT7,
    WG_STATE_SHOOT8,
    WG_STATE_SHOOT9,
    WG_STATE_DOG_JUMP1,
    WG_STATE_DOG_JUMP2,
    WG_STATE_DOG_JUMP3,
    WG_STATE_DOG_JUMP4,
    WG_STATE_DOG_JUMP5,
    WG_STATE_PAIN1,
    WG_STATE_PAIN2,
    WG_STATE_DIE1,
    WG_STATE_DIE2,
    WG_STATE_DIE3,
    WG_STATE_DIE4,
    WG_STATE_DEAD,
    WG_STATE_NEEDLE1,
    WG_STATE_NEEDLE2,
    WG_STATE_NEEDLE3,
    WG_STATE_NEEDLE4,
    WG_STATE_ROCKET,
    WG_STATE_SMOKE1,
    WG_STATE_SMOKE2,
    WG_STATE_SMOKE3,
    WG_STATE_SMOKE4,
    WG_STATE_BOOM1,
    WG_STATE_BOOM2,
    WG_STATE_BOOM3,
    WG_STATE_FIRE1,
    WG_STATE_FIRE2,
    WG_STATE_ATTACK_PENDING
} wg_actor_state_t;

typedef struct wg_actor
{
    int32_t x;
    int32_t y;
    uint16_t shape;
    uint8_t tile_x;
    uint8_t tile_y;
    uint8_t direction;
    uint8_t rotate;
    uint8_t area_number;
    uint16_t angle;
    uint16_t base_shape;
    uint16_t attack_shape;
    uint16_t flags;
    int32_t tic_count;
    int32_t reaction_time;
    int32_t speed;
    int32_t distance;
    int32_t hit_points;
    wg_actor_state_t state;
    wg_actor_class_t actor_class;
} wg_actor_t;

struct wg_level;

int WL_SpawnStand(struct wg_level *level, wg_actor_class_t actor_class,
                  uint8_t tile_x, uint8_t tile_y, uint8_t map_direction);
int WL_SpawnPatrol(struct wg_level *level, wg_actor_class_t actor_class,
                   uint8_t tile_x, uint8_t tile_y, uint8_t map_direction);
int WL_SpawnDeadGuard(struct wg_level *level, uint8_t tile_x, uint8_t tile_y);
int WL_SpawnBoss(struct wg_level *level, wg_actor_class_t actor_class,
                 uint8_t tile_x, uint8_t tile_y);
int WL_SpawnGhost(struct wg_level *level, wg_ghost_kind_t ghost_kind,
                  uint8_t tile_x, uint8_t tile_y);

#endif
