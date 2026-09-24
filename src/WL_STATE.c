/* Portable actor state and tile movement from the original WL_STATE.C. */
#include "WL_STATE.h"

#include <limits.h>
#include <string.h>

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
    return (actor->flags & WG_ACTOR_FLAG_SHOOTABLE) != 0U;
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
    if (level->areas[(size_t)destination_y * WG_LEVEL_SIZE
                     + (size_t)destination_x] != WG_NO_AREA)
    {
        actor->area_number = level->areas[(size_t)destination_y * WG_LEVEL_SIZE
                                          + (size_t)destination_x];
    }
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

int WL_UpdateAreaConnectivity(wg_level_t *level)
{
    uint8_t area_connect[WG_NUM_AREAS][WG_NUM_AREAS];
    uint8_t pending[WG_NUM_AREAS];
    size_t pending_count = 0U;
    size_t door_index;
    int player_x;
    int player_y;
    uint8_t player_area;

    if (level == NULL)
    {
        return 0;
    }
    memset(level->area_by_player, 0, sizeof(level->area_by_player));
    memset(area_connect, 0, sizeof(area_connect));
    player_x = level->player_x / WG_FIXED_ONE;
    player_y = level->player_y / WG_FIXED_ONE;
    if (player_x < 0 || player_x >= WG_LEVEL_SIZE
        || player_y < 0 || player_y >= WG_LEVEL_SIZE)
    {
        return 0;
    }
    player_area = level->areas[(size_t)player_y * WG_LEVEL_SIZE
                               + (size_t)player_x];
    if (player_area >= WG_NUM_AREAS)
    {
        return 0;
    }
    for (door_index = 0U; door_index < level->door_count; ++door_index)
    {
        const wg_door_t *door = &level->doors[door_index];
        int first_x = door->tile_x;
        int first_y = door->tile_y;
        int second_x = door->tile_x;
        int second_y = door->tile_y;
        uint8_t first_area;
        uint8_t second_area;

        if (door->position != 0xffffU)
        {
            continue;
        }
        if (door->vertical)
        {
            --first_x;
            ++second_x;
        }
        else
        {
            --first_y;
            ++second_y;
        }
        if (first_x < 0 || first_x >= WG_LEVEL_SIZE
            || first_y < 0 || first_y >= WG_LEVEL_SIZE
            || second_x < 0 || second_x >= WG_LEVEL_SIZE
            || second_y < 0 || second_y >= WG_LEVEL_SIZE)
        {
            continue;
        }
        first_area = level->areas[(size_t)first_y * WG_LEVEL_SIZE
                                  + (size_t)first_x];
        second_area = level->areas[(size_t)second_y * WG_LEVEL_SIZE
                                   + (size_t)second_x];
        if (first_area < WG_NUM_AREAS && second_area < WG_NUM_AREAS)
        {
            area_connect[first_area][second_area] = 1U;
            area_connect[second_area][first_area] = 1U;
        }
    }
    pending[pending_count++] = player_area;
    level->area_by_player[player_area] = 1U;
    while (pending_count > 0U)
    {
        uint8_t area = pending[--pending_count];
        size_t candidate;

        for (candidate = 0U; candidate < WG_NUM_AREAS; ++candidate)
        {
            if (area_connect[area][candidate]
                && !level->area_by_player[candidate])
            {
                level->area_by_player[candidate] = 1U;
                pending[pending_count++] = (uint8_t)candidate;
            }
        }
    }
    return 1;
}

static int WL_LineTileClear(const wg_level_t *level, int x, int y,
                            uint32_t intercept)
{
    uint8_t tile;
    size_t door;

    if (x < 0 || x >= WG_LEVEL_SIZE || y < 0 || y >= WG_LEVEL_SIZE)
    {
        return 0;
    }
    tile = level->tiles[(size_t)y * WG_LEVEL_SIZE + (size_t)x];
    if (tile == 0U)
    {
        return 1;
    }
    if ((tile & 0x80U) == 0U)
    {
        return 0;
    }
    door = tile & 0x3fU;
    return door < level->door_count
           && intercept <= level->doors[door].position;
}

int WL_CheckLine(const wg_level_t *level, const wg_actor_t *actor)
{
    int32_t x1;
    int32_t y1;
    int32_t x2;
    int32_t y2;
    int xt1;
    int yt1;
    int xt2;
    int yt2;
    int x;
    int y;
    int step;
    int partial;
    int32_t fraction;
    int32_t fraction_step;
    int32_t delta_fraction;
    int32_t delta;
    int64_t long_step;

    if (level == NULL || actor == NULL)
    {
        return 0;
    }
    x1 = actor->x >> 8;
    y1 = actor->y >> 8;
    x2 = level->player_x >> 8;
    y2 = level->player_y >> 8;
    xt1 = x1 >> 8;
    yt1 = y1 >> 8;
    xt2 = x2 >> 8;
    yt2 = y2 >> 8;

    if (xt2 != xt1)
    {
        if (xt2 > xt1)
        {
            partial = 256 - (x1 & 0xff);
            step = 1;
        }
        else
        {
            partial = x1 & 0xff;
            step = -1;
        }
        delta_fraction = x2 > x1 ? x2 - x1 : x1 - x2;
        delta = y2 - y1;
        long_step = ((int64_t)delta * 256) / delta_fraction;
        if (long_step > 0x7fff)
        {
            fraction_step = 0x7fff;
        }
        else if (long_step < -0x7fff)
        {
            fraction_step = -0x7fff;
        }
        else
        {
            fraction_step = (int32_t)long_step;
        }
        fraction = y1 + fraction_step * partial / 256;
        x = xt1 + step;
        xt2 += step;
        do
        {
            uint32_t intercept;

            y = fraction >> 8;
            fraction += fraction_step;
            intercept = (uint32_t)(fraction - fraction_step / 2);
            if (!WL_LineTileClear(level, x, y, intercept))
            {
                return 0;
            }
            x += step;
        } while (x != xt2);
    }

    if (yt2 != yt1)
    {
        if (yt2 > yt1)
        {
            partial = 256 - (y1 & 0xff);
            step = 1;
        }
        else
        {
            partial = y1 & 0xff;
            step = -1;
        }
        delta_fraction = y2 > y1 ? y2 - y1 : y1 - y2;
        delta = x2 - x1;
        long_step = ((int64_t)delta * 256) / delta_fraction;
        if (long_step > 0x7fff)
        {
            fraction_step = 0x7fff;
        }
        else if (long_step < -0x7fff)
        {
            fraction_step = -0x7fff;
        }
        else
        {
            fraction_step = (int32_t)long_step;
        }
        fraction = x1 + fraction_step * partial / 256;
        y = yt1 + step;
        yt2 += step;
        do
        {
            uint32_t intercept;

            x = fraction >> 8;
            fraction += fraction_step;
            intercept = (uint32_t)(fraction - fraction_step / 2);
            if (!WL_LineTileClear(level, x, y, intercept))
            {
                return 0;
            }
            y += step;
        } while (y != yt2);
    }
    return 1;
}

int WL_CheckSight(const wg_level_t *level, const wg_actor_t *actor)
{
    int32_t delta_x;
    int32_t delta_y;

    if (level == NULL || actor == NULL
        || actor->area_number >= WG_NUM_AREAS
        || !level->area_by_player[actor->area_number])
    {
        return 0;
    }
    delta_x = level->player_x - actor->x;
    delta_y = level->player_y - actor->y;
    if (delta_x > -0x18000L && delta_x < 0x18000L
        && delta_y > -0x18000L && delta_y < 0x18000L)
    {
        return 1;
    }
    switch (actor->direction)
    {
    case 0U:
        if (delta_x < 0)
        {
            return 0;
        }
        break;
    case 2U:
        if (delta_y > 0)
        {
            return 0;
        }
        break;
    case 4U:
        if (delta_x > 0)
        {
            return 0;
        }
        break;
    case 6U:
        if (delta_y < 0)
        {
            return 0;
        }
        break;
    default:
        break;
    }
    return WL_CheckLine(level, actor);
}

static void WL_FirstSighting(wg_actor_t *actor)
{
    switch (actor->actor_class)
    {
    case WG_ACTOR_GUARD:
    case WG_ACTOR_MUTANT:
        actor->speed *= 3;
        break;
    case WG_ACTOR_OFFICER:
    case WG_ACTOR_REAL_HITLER:
        actor->speed *= 5;
        break;
    case WG_ACTOR_SS:
        actor->speed *= 4;
        break;
    case WG_ACTOR_DOG:
    case WG_ACTOR_GHOST:
        actor->speed *= 2;
        break;
    case WG_ACTOR_BOSS:
        actor->speed = 512 * 3;
        break;
    case WG_ACTOR_SCHABBS:
    case WG_ACTOR_FAKE:
    case WG_ACTOR_MECHA_HITLER:
    case WG_ACTOR_GRETEL:
    case WG_ACTOR_GIFT:
    case WG_ACTOR_FAT:
        actor->speed *= 3;
        break;
    default:
        return;
    }
    actor->state = WG_STATE_CHASE1;
    actor->shape = actor->base_shape;
    actor->tic_count = actor->actor_class == WG_ACTOR_REAL_HITLER ? 6 : 10;
    actor->reaction_time = 0;
    if (actor->distance < 0)
    {
        actor->distance = 0;
    }
    actor->flags |= WG_ACTOR_FLAG_ATTACK_MODE | WG_ACTOR_FLAG_FIRST_ATTACK;
}

static int32_t WL_ReactionTime(wg_level_t *level,
                               wg_actor_class_t actor_class)
{
    switch (actor_class)
    {
    case WG_ACTOR_GUARD:
        return 1 + WG_RandomNext(&level->random) / 4;
    case WG_ACTOR_OFFICER:
        return 2;
    case WG_ACTOR_MUTANT:
    case WG_ACTOR_SS:
        return 1 + WG_RandomNext(&level->random) / 6;
    case WG_ACTOR_DOG:
        return 1 + WG_RandomNext(&level->random) / 8;
    default:
        return 1;
    }
}

int WL_TickAwareness(wg_level_t *level, unsigned tics, int made_noise)
{
    size_t index;

    if (level == NULL || tics > (unsigned)INT32_MAX
        || !WL_UpdateAreaConnectivity(level))
    {
        return 0;
    }
    for (index = 0U; index < level->actor_count; ++index)
    {
        wg_actor_t *actor = &level->actors[index];
        int can_notice = actor->state == WG_STATE_STAND
                         || WL_IsPathState(actor->state);

        if (!can_notice || (actor->flags & WG_ACTOR_FLAG_ATTACK_MODE) != 0U
            || actor->area_number >= WG_NUM_AREAS
            || !level->area_by_player[actor->area_number])
        {
            continue;
        }
        if (actor->reaction_time > 0)
        {
            actor->reaction_time -= (int32_t)tics;
            if (actor->reaction_time > 0)
            {
                continue;
            }
            WL_FirstSighting(actor);
            continue;
        }
        if ((actor->flags & WG_ACTOR_FLAG_AMBUSH) != 0U)
        {
            if (!WL_CheckSight(level, actor))
            {
                continue;
            }
            actor->flags = (uint16_t)(actor->flags & 0xffbfU);
        }
        else if (!made_noise && !WL_CheckSight(level, actor))
        {
            continue;
        }
        actor->reaction_time = WL_ReactionTime(level, actor->actor_class);
    }
    return 1;
}
