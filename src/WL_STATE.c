/* Portable actor state and tile movement from the original WL_STATE.C. */
#include "WL_STATE.h"

#include <limits.h>

#include "WG_FIXED.h"

#define WG_NO_DIRECTION 8U

static int WL_IsPathState(wg_actor_state_t state)
{
    return state >= WG_STATE_PATH1 && state <= WG_STATE_PATH4;
}

static int32_t WL_StateDuration(wg_actor_state_t state)
{
    switch (state)
    {
    case WG_STATE_PATH1:
    case WG_STATE_PATH3:
        return 20;
    case WG_STATE_PATH1S:
    case WG_STATE_PATH3S:
        return 5;
    case WG_STATE_PATH2:
    case WG_STATE_PATH4:
        return 15;
    default:
        return 0;
    }
}

static wg_actor_state_t WL_NextPathState(wg_actor_state_t state)
{
    switch (state)
    {
    case WG_STATE_PATH1:
        return WG_STATE_PATH1S;
    case WG_STATE_PATH1S:
        return WG_STATE_PATH2;
    case WG_STATE_PATH2:
        return WG_STATE_PATH3;
    case WG_STATE_PATH3:
        return WG_STATE_PATH3S;
    case WG_STATE_PATH3S:
        return WG_STATE_PATH4;
    default:
        return WG_STATE_PATH1;
    }
}

static void WL_SetPathShape(wg_actor_t *actor)
{
    unsigned frame;

    switch (actor->state)
    {
    case WG_STATE_PATH2:
        frame = 1U;
        break;
    case WG_STATE_PATH3:
    case WG_STATE_PATH3S:
        frame = 2U;
        break;
    case WG_STATE_PATH4:
        frame = 3U;
        break;
    default:
        frame = 0U;
        break;
    }
    actor->shape = (uint16_t)(actor->base_shape + frame * 8U);
}

static int WL_IsBlockingActor(const wg_actor_t *actor)
{
    return actor->actor_class != WG_ACTOR_INERT
           && actor->actor_class != WG_ACTOR_GHOST;
}

static int WL_ActorBlocksSpot(const wg_level_t *level, size_t actor_index,
                              int tile_x, int tile_y)
{
    size_t index;

    for (index = 0; index < level->actor_count; ++index)
    {
        const wg_actor_t *other = &level->actors[index];

        if (index != actor_index && WL_IsBlockingActor(other)
            && other->tile_x == (uint8_t)tile_x
            && other->tile_y == (uint8_t)tile_y)
        {
            return 1;
        }
    }
    return 0;
}

static int WL_DiagonalSpotBlocked(const wg_level_t *level,
                                  size_t actor_index, int tile_x, int tile_y)
{
    return level->tiles[(size_t)tile_y * WG_LEVEL_SIZE + (size_t)tile_x] != 0U
           || WL_ActorBlocksSpot(level, actor_index, tile_x, tile_y);
}

static int WL_TryWalk(wg_level_t *level, size_t actor_index)
{
    static const int direction_x[] = { 1, 1, 0, -1, -1, -1, 0, 1 };
    static const int direction_y[] = { 0, -1, -1, -1, 0, 1, 1, 1 };
    wg_actor_t *actor = &level->actors[actor_index];
    int destination_x;
    int destination_y;
    uint8_t tile;
    size_t door;

    if (actor->direction >= WG_NO_DIRECTION)
    {
        return 0;
    }
    destination_x = (int)actor->tile_x + direction_x[actor->direction];
    destination_y = (int)actor->tile_y + direction_y[actor->direction];
    if (destination_x < 0 || destination_x >= WG_LEVEL_SIZE
        || destination_y < 0 || destination_y >= WG_LEVEL_SIZE)
    {
        return 0;
    }
    if ((actor->direction & 1U) != 0U)
    {
        if (WL_DiagonalSpotBlocked(level, actor_index,
                                   destination_x, destination_y)
            || WL_DiagonalSpotBlocked(level, actor_index,
                                      destination_x, actor->tile_y)
            || WL_DiagonalSpotBlocked(level, actor_index,
                                      actor->tile_x, destination_y))
        {
            return 0;
        }
    }
    else if (WL_ActorBlocksSpot(level, actor_index,
                                destination_x, destination_y))
    {
        return 0;
    }
    tile = level->tiles[(size_t)destination_y * WG_LEVEL_SIZE
                        + (size_t)destination_x];
    if (tile != 0U)
    {
        if ((tile & 0x80U) == 0U
            || actor->actor_class == WG_ACTOR_DOG
            || actor->actor_class == WG_ACTOR_FAKE)
        {
            return 0;
        }
        door = tile & 0x3fU;
        if (door >= level->door_count)
        {
            return 0;
        }
        actor->distance = level->doors[door].position == 0xffffU
                              ? WG_FIXED_ONE : -(int32_t)door - 1;
    }
    else
    {
        actor->distance = WG_FIXED_ONE;
    }
    actor->tile_x = (uint8_t)destination_x;
    actor->tile_y = (uint8_t)destination_y;
    return 1;
}

static void WL_SelectPathDir(wg_level_t *level, size_t actor_index)
{
    wg_actor_t *actor = &level->actors[actor_index];
    uint16_t info = level->info[(size_t)actor->tile_y * WG_LEVEL_SIZE
                                + actor->tile_x];

    if (info >= 90U && info < 98U)
    {
        actor->direction = (uint8_t)(info - 90U);
    }
    actor->distance = WG_FIXED_ONE;
    if (!WL_TryWalk(level, actor_index))
    {
        actor->direction = WG_NO_DIRECTION;
    }
}

static void WL_MoveObj(wg_actor_t *actor, int32_t move)
{
    static const int direction_x[] = { 1, 1, 0, -1, -1, -1, 0, 1 };
    static const int direction_y[] = { 0, -1, -1, -1, 0, 1, 1, 1 };

    actor->x += direction_x[actor->direction] * move;
    actor->y += direction_y[actor->direction] * move;
    actor->distance -= move;
}

static void WL_T_Path(wg_level_t *level, size_t actor_index, int32_t tics)
{
    wg_actor_t *actor = &level->actors[actor_index];
    int32_t move;

    if (actor->direction == WG_NO_DIRECTION)
    {
        WL_SelectPathDir(level, actor_index);
        if (actor->direction == WG_NO_DIRECTION)
        {
            return;
        }
    }
    move = actor->speed * tics;
    while (move > 0)
    {
        if (actor->distance < 0)
        {
            size_t door = (size_t)(-actor->distance - 1);

            if (door >= level->door_count
                || level->doors[door].position != 0xffffU)
            {
                return;
            }
            actor->distance = WG_FIXED_ONE;
        }
        if (move < actor->distance)
        {
            WL_MoveObj(actor, move);
            return;
        }
        move -= actor->distance;
        actor->x = (int32_t)actor->tile_x * WG_FIXED_ONE + WG_FIXED_ONE / 2;
        actor->y = (int32_t)actor->tile_y * WG_FIXED_ONE + WG_FIXED_ONE / 2;
        WL_SelectPathDir(level, actor_index);
        if (actor->direction == WG_NO_DIRECTION)
        {
            return;
        }
    }
}

int WL_TickActors(wg_level_t *level, unsigned tics)
{
    size_t index;

    if (level == NULL || tics > (unsigned)(INT32_MAX / 1500))
    {
        return 0;
    }
    for (index = 0; index < level->actor_count; ++index)
    {
        wg_actor_t *actor = &level->actors[index];

        if (!WL_IsPathState(actor->state))
        {
            continue;
        }
        if (actor->tic_count != 0)
        {
            actor->tic_count -= (int32_t)tics;
            while (actor->tic_count <= 0)
            {
                actor->state = WL_NextPathState(actor->state);
                actor->tic_count += WL_StateDuration(actor->state);
                WL_SetPathShape(actor);
            }
        }
        WL_T_Path(level, index, (int32_t)tics);
    }
    return 1;
}
