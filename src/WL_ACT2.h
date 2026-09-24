#ifndef WL_ACT2_H
#define WL_ACT2_H

#include <stdint.h>

#define WG_MAX_ACTORS 150

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
    WG_ACTOR_FAT
} wg_actor_class_t;

typedef enum wg_ghost_kind
{
    WG_GHOST_BLINKY = 0,
    WG_GHOST_CLYDE,
    WG_GHOST_PINKY,
    WG_GHOST_INKY
} wg_ghost_kind_t;

typedef struct wg_actor
{
    int32_t x;
    int32_t y;
    uint16_t shape;
    uint8_t tile_x;
    uint8_t tile_y;
    uint8_t direction;
    uint8_t rotate;
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
