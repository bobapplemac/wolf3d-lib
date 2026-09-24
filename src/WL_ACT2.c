/* Portable initial SpawnStand construction from the original WL_ACT2.C. */
#include "WL_ACT2.h"

#include "WG_FIXED.h"
#include "WL_GAME.h"

#define WG_SPR_GRD_S_1 50U
#define WG_SPR_GRD_W1_1 58U

static int WL_SpawnGuard(struct wg_level *level, uint8_t tile_x,
                         uint8_t tile_y, uint8_t map_direction,
                         uint16_t shape, int patrol)
{
    wg_actor_t *actor;
    int destination_x = tile_x;
    int destination_y = tile_y;

    if (level == NULL || tile_x >= WG_LEVEL_SIZE || tile_y >= WG_LEVEL_SIZE
        || map_direction > 3U || level->actor_count >= WG_MAX_ACTORS)
    {
        return 0;
    }
    if (patrol)
    {
        switch (map_direction)
        {
        case 0:
            ++destination_x;
            break;
        case 1:
            --destination_y;
            break;
        case 2:
            --destination_x;
            break;
        default:
            ++destination_y;
            break;
        }
    }
    if (destination_x < 0 || destination_x >= WG_LEVEL_SIZE
        || destination_y < 0 || destination_y >= WG_LEVEL_SIZE)
    {
        return 0;
    }
    actor = &level->actors[level->actor_count++];
    actor->x = (int32_t)tile_x * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    actor->y = (int32_t)tile_y * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    actor->tile_x = (uint8_t)destination_x;
    actor->tile_y = (uint8_t)destination_y;
    actor->direction = (uint8_t)(map_direction * 2U);
    actor->shape = shape;
    actor->rotate = 1U;
    actor->actor_class = WG_ACTOR_GUARD;
    return 1;
}

int WL_SpawnStand(struct wg_level *level, wg_actor_class_t actor_class,
                  uint8_t tile_x, uint8_t tile_y, uint8_t map_direction)
{
    if (level == NULL || actor_class != WG_ACTOR_GUARD
        || !WL_SpawnGuard(level, tile_x, tile_y, map_direction,
                          WG_SPR_GRD_S_1, 0))
    {
        return 0;
    }
    return 1;
}

int WL_SpawnPatrol(struct wg_level *level, wg_actor_class_t actor_class,
                   uint8_t tile_x, uint8_t tile_y, uint8_t map_direction)
{
    if (level == NULL || actor_class != WG_ACTOR_GUARD
        || !WL_SpawnGuard(level, tile_x, tile_y, map_direction,
                          WG_SPR_GRD_W1_1, 1))
    {
        return 0;
    }
    return 1;
}
