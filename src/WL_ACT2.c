/* Portable initial SpawnStand construction from the original WL_ACT2.C. */
#include "WL_ACT2.h"

#include "WG_FIXED.h"
#include "WL_GAME.h"

#define WG_SPR_GRD_S_1 50U
#define WG_SPR_GRD_W1_1 58U
#define WG_SPR_GRD_DEAD 95U
#define WG_SPR_DOG_W1_1 99U
#define WG_SPR_SS_S_1 138U
#define WG_SPR_SS_W1_1 146U
#define WG_SPR_MUT_S_1 187U
#define WG_SPR_MUT_W1_1 195U
#define WG_SPR_OFC_S_1 238U
#define WG_SPR_OFC_W1_1 246U

static uint16_t WL_StandingShape(wg_actor_class_t actor_class)
{
    switch (actor_class)
    {
    case WG_ACTOR_GUARD:
        return WG_SPR_GRD_S_1;
    case WG_ACTOR_OFFICER:
        return WG_SPR_OFC_S_1;
    case WG_ACTOR_SS:
        return WG_SPR_SS_S_1;
    case WG_ACTOR_DOG:
        /* The map format has standing-dog codes although DOS SpawnStand
         * omits its dog case. Use the dog's path pose without advancing. */
        return WG_SPR_DOG_W1_1;
    case WG_ACTOR_MUTANT:
        return WG_SPR_MUT_S_1;
    default:
        return UINT16_MAX;
    }
}

static uint16_t WL_PatrolShape(wg_actor_class_t actor_class)
{
    switch (actor_class)
    {
    case WG_ACTOR_GUARD:
        return WG_SPR_GRD_W1_1;
    case WG_ACTOR_OFFICER:
        return WG_SPR_OFC_W1_1;
    case WG_ACTOR_SS:
        return WG_SPR_SS_W1_1;
    case WG_ACTOR_DOG:
        return WG_SPR_DOG_W1_1;
    case WG_ACTOR_MUTANT:
        return WG_SPR_MUT_W1_1;
    default:
        return UINT16_MAX;
    }
}

static int WL_SpawnActor(struct wg_level *level, uint8_t tile_x,
                         uint8_t tile_y, uint8_t map_direction,
                         wg_actor_class_t actor_class, uint16_t shape,
                         int patrol)
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
    actor->actor_class = actor_class;
    return 1;
}

int WL_SpawnStand(struct wg_level *level, wg_actor_class_t actor_class,
                  uint8_t tile_x, uint8_t tile_y, uint8_t map_direction)
{
    uint16_t shape = WL_StandingShape(actor_class);

    if (level == NULL || shape == UINT16_MAX
        || !WL_SpawnActor(level, tile_x, tile_y, map_direction,
                          actor_class, shape, 0))
    {
        return 0;
    }
    return 1;
}

int WL_SpawnPatrol(struct wg_level *level, wg_actor_class_t actor_class,
                   uint8_t tile_x, uint8_t tile_y, uint8_t map_direction)
{
    uint16_t shape = WL_PatrolShape(actor_class);

    if (level == NULL || shape == UINT16_MAX
        || !WL_SpawnActor(level, tile_x, tile_y, map_direction,
                          actor_class, shape, 1))
    {
        return 0;
    }
    return 1;
}

int WL_SpawnDeadGuard(struct wg_level *level, uint8_t tile_x, uint8_t tile_y)
{
    wg_actor_t *actor;

    if (level == NULL || tile_x >= WG_LEVEL_SIZE || tile_y >= WG_LEVEL_SIZE
        || level->actor_count >= WG_MAX_ACTORS)
    {
        return 0;
    }
    actor = &level->actors[level->actor_count++];
    actor->x = (int32_t)tile_x * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    actor->y = (int32_t)tile_y * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    actor->tile_x = tile_x;
    actor->tile_y = tile_y;
    actor->direction = 0U;
    actor->shape = WG_SPR_GRD_DEAD;
    actor->rotate = 0U;
    actor->actor_class = WG_ACTOR_INERT;
    return 1;
}
