/* Portable actor state and tile movement from the original WL_STATE.C. */
#include "WL_STATE.h"

#include <limits.h>
#include <math.h>
#include <string.h>

#include "WG_FIXED.h"
#include "WL_ACT1.h"
#include "WL_AGENT.h"
#include "WL_MAIN.h"

#define WG_NO_DIRECTION 8U
#define WG_PROJECTILE_SIZE INT32_C(0xc000)
#define WG_PROJECTILE_COLLISION_SIZE INT32_C(0x2000)
#define WG_SPR_HYPO1 317U
#define WG_SPR_FIRE1 326U
#define WG_SPR_ROCKET1 370U
#define WG_SPR_SMOKE1 378U
#define WG_SPR_BOOM1 382U
#define WG_TWO_PI 6.283185314
#define WG_SPR_STAT_KEY1 22U
#define WG_SPR_STAT_CLIP2 28U
#define WG_SPR_STAT_MACHINEGUN 29U

static int WL_BeginAttack(wg_actor_t *actor);
static void WL_FirstSighting(wg_actor_t *actor);
static int32_t WL_ChaseStateDuration(const wg_actor_t *actor,
                                     wg_actor_state_t state);
static wg_actor_state_t WL_NextChaseState(wg_actor_state_t state);
static void WL_SetChaseShape(wg_actor_t *actor);
static wg_actor_t *WL_AllocateTransientActor(wg_level_t *level);

static int WL_IsPathState(wg_actor_state_t state)
{
    return state >= WG_STATE_PATH1 && state <= WG_STATE_PATH4;
}

static int WL_PathHasThink(wg_actor_state_t state)
{
    return state == WG_STATE_PATH1 || state == WG_STATE_PATH2
           || state == WG_STATE_PATH3 || state == WG_STATE_PATH4;
}

static int WL_IsChaseState(wg_actor_state_t state)
{
    return state >= WG_STATE_CHASE1 && state <= WG_STATE_CHASE4;
}

static int WL_ChaseHasThink(wg_actor_state_t state)
{
    return state == WG_STATE_CHASE1 || state == WG_STATE_CHASE2
           || state == WG_STATE_CHASE3 || state == WG_STATE_CHASE4;
}

static int WL_IsShootState(wg_actor_state_t state)
{
    return state >= WG_STATE_SHOOT1 && state <= WG_STATE_SHOOT9;
}

static int WL_IsDogJumpState(wg_actor_state_t state)
{
    return state >= WG_STATE_DOG_JUMP1 && state <= WG_STATE_DOG_JUMP5;
}

static int WL_IsPainState(wg_actor_state_t state)
{
    return state == WG_STATE_PAIN1 || state == WG_STATE_PAIN2;
}

static int WL_IsDeathState(wg_actor_state_t state)
{
    return state >= WG_STATE_DIE1 && state <= WG_STATE_DIE9;
}

static uint16_t WL_PainShape(wg_actor_class_t actor_class, int second)
{
    switch (actor_class)
    {
    case WG_ACTOR_GUARD:
        return (uint16_t)(second ? 94U : 90U);
    case WG_ACTOR_OFFICER:
        return (uint16_t)(second ? 282U : 278U);
    case WG_ACTOR_MUTANT:
        return (uint16_t)(second ? 231U : 227U);
    case WG_ACTOR_SS:
        return (uint16_t)(second ? 182U : 178U);
    default:
        return 0U;
    }
}

static unsigned WL_DeathTimedFrames(wg_actor_class_t actor_class)
{
    switch (actor_class)
    {
    case WG_ACTOR_OFFICER:
    case WG_ACTOR_MUTANT:
        return 4U;
    case WG_ACTOR_GUARD:
    case WG_ACTOR_SS:
    case WG_ACTOR_DOG:
    case WG_ACTOR_BOSS:
    case WG_ACTOR_GRETEL:
    case WG_ACTOR_MECHA_HITLER:
        return 3U;
    case WG_ACTOR_SCHABBS:
    case WG_ACTOR_FAKE:
    case WG_ACTOR_GIFT:
    case WG_ACTOR_FAT:
        return 5U;
    case WG_ACTOR_REAL_HITLER:
        return 9U;
    default:
        return 0U;
    }
}

static int32_t WL_DeathFrameDuration(wg_actor_class_t actor_class,
                                     unsigned frame)
{
    switch (actor_class)
    {
    case WG_ACTOR_OFFICER:
        return 11;
    case WG_ACTOR_MUTANT:
        return 7;
    case WG_ACTOR_FAKE:
    case WG_ACTOR_MECHA_HITLER:
        return 10;
    case WG_ACTOR_SCHABBS:
        return frame == 1U ? 5 : 10;
    case WG_ACTOR_GIFT:
    case WG_ACTOR_FAT:
    case WG_ACTOR_REAL_HITLER:
        return frame == 0U ? 1 : frame == 1U ? 5 : 10;
    default:
        return 15;
    }
}

static uint16_t WL_DeathShape(wg_actor_class_t actor_class, unsigned frame)
{
    static const uint16_t guard[] = {91U, 92U, 93U, 95U};
    static const uint16_t officer[] = {279U, 280U, 281U, 283U, 284U};
    static const uint16_t mutant[] = {228U, 229U, 230U, 232U, 233U};
    static const uint16_t ss[] = {179U, 180U, 181U, 183U};
    static const uint16_t dog[] = {131U, 132U, 133U, 134U};
    static const uint16_t boss[] = {304U, 305U, 306U, 303U};
    static const uint16_t schabbs[] = {307U, 307U, 313U, 314U, 315U, 316U};
    static const uint16_t fake[] = {328U, 329U, 330U, 331U, 332U, 333U};
    static const uint16_t mecha[] = {342U, 343U, 344U, 341U};
    static const uint16_t hitler[] =
        {345U, 345U, 353U, 354U, 355U, 356U, 357U, 358U, 359U, 352U};
    static const uint16_t gift[] = {360U, 360U, 366U, 367U, 368U, 369U};
    static const uint16_t gretel[] = {393U, 394U, 395U, 392U};
    static const uint16_t fat[] = {396U, 396U, 404U, 405U, 406U, 407U};

    switch (actor_class)
    {
    case WG_ACTOR_GUARD:
        return guard[frame];
    case WG_ACTOR_OFFICER:
        return officer[frame];
    case WG_ACTOR_MUTANT:
        return mutant[frame];
    case WG_ACTOR_SS:
        return ss[frame];
    case WG_ACTOR_DOG:
        return dog[frame];
    case WG_ACTOR_BOSS:
        return boss[frame];
    case WG_ACTOR_SCHABBS:
        return schabbs[frame];
    case WG_ACTOR_FAKE:
        return fake[frame];
    case WG_ACTOR_MECHA_HITLER:
        return mecha[frame];
    case WG_ACTOR_REAL_HITLER:
        return hitler[frame];
    case WG_ACTOR_GIFT:
        return gift[frame];
    case WG_ACTOR_GRETEL:
        return gretel[frame];
    case WG_ACTOR_FAT:
        return fat[frame];
    default:
        return 0U;
    }
}

static int32_t WL_DeathTerminalDuration(wg_actor_class_t actor_class)
{
    switch (actor_class)
    {
    case WG_ACTOR_DOG:
        return 15;
    case WG_ACTOR_SCHABBS:
    case WG_ACTOR_GIFT:
    case WG_ACTOR_FAT:
    case WG_ACTOR_REAL_HITLER:
        return 20;
    default:
        return 0;
    }
}

static int WL_UsesDeathCam(wg_actor_class_t actor_class)
{
    return actor_class == WG_ACTOR_SCHABBS
           || actor_class == WG_ACTOR_GIFT
           || actor_class == WG_ACTOR_FAT
           || actor_class == WG_ACTOR_REAL_HITLER;
}

static int WL_DropItem(wg_level_t *level, uint8_t tile_x, uint8_t tile_y,
                       uint16_t shape, wg_item_type_t item)
{
    wg_static_object_t *object;

    if (level->static_count >= WG_MAX_STATICS)
    {
        return 0;
    }
    object = &level->statics[level->static_count++];
    object->tile_x = tile_x;
    object->tile_y = tile_y;
    object->blocking = 0U;
    object->removed = 0U;
    object->shape = shape;
    object->item = item;
    return 1;
}

static int WL_SpawnRealHitler(wg_level_t *level, const wg_actor_t *mecha)
{
    static const int32_t hit_points[] = {500, 700, 800, 900};
    wg_actor_t *actor = WL_AllocateTransientActor(level);

    if (actor == NULL)
    {
        return 0;
    }
    actor->x = mecha->x;
    actor->y = mecha->y;
    actor->tile_x = mecha->tile_x;
    actor->tile_y = mecha->tile_y;
    actor->direction = mecha->direction;
    actor->shape = 345U;
    actor->base_shape = 345U;
    actor->attack_shape = 349U;
    actor->rotate = 0U;
    actor->area_number = mecha->area_number;
    actor->flags = mecha->flags | WG_ACTOR_FLAG_SHOOTABLE;
    actor->tic_count = WG_RandomNext(&level->random) % 6U;
    actor->speed = 512 * 5;
    actor->distance = mecha->distance;
    actor->hit_points = hit_points[level->difficulty];
    actor->state = WG_STATE_CHASE1;
    actor->actor_class = WG_ACTOR_REAL_HITLER;
    return 1;
}

int WL_KillActor(wg_level_t *level, size_t actor_index)
{
    wg_actor_t *actor;
    uint32_t points;
    int drop = 0;
    wg_item_type_t drop_item = WG_ITEM_NONE;

    if (level == NULL || actor_index >= level->actor_count)
    {
        return 0;
    }
    actor = &level->actors[actor_index];
    if ((actor->flags & WG_ACTOR_FLAG_SHOOTABLE) == 0U
        || WL_DeathTimedFrames(actor->actor_class) == 0U)
    {
        return 0;
    }
    switch (actor->actor_class)
    {
    case WG_ACTOR_GUARD:
        points = 100U;
        drop = WG_SPR_STAT_CLIP2;
        drop_item = WG_ITEM_CLIP2;
        break;
    case WG_ACTOR_OFFICER:
        points = 400U;
        drop = WG_SPR_STAT_CLIP2;
        drop_item = WG_ITEM_CLIP2;
        break;
    case WG_ACTOR_MUTANT:
        points = 700U;
        drop = WG_SPR_STAT_CLIP2;
        drop_item = WG_ITEM_CLIP2;
        break;
    case WG_ACTOR_SS:
        points = 500U;
        drop = level->player_best_weapon < 2U
                   ? WG_SPR_STAT_MACHINEGUN : WG_SPR_STAT_CLIP2;
        drop_item = level->player_best_weapon < 2U
                        ? WG_ITEM_MACHINEGUN : WG_ITEM_CLIP2;
        break;
    case WG_ACTOR_DOG:
        points = 200U;
        break;
    case WG_ACTOR_BOSS:
    case WG_ACTOR_GRETEL:
        points = 5000U;
        drop = WG_SPR_STAT_KEY1;
        drop_item = WG_ITEM_KEY1;
        break;
    case WG_ACTOR_FAKE:
        points = 2000U;
        break;
    case WG_ACTOR_SCHABBS:
    case WG_ACTOR_MECHA_HITLER:
    case WG_ACTOR_REAL_HITLER:
    case WG_ACTOR_GIFT:
    case WG_ACTOR_FAT:
        points = 5000U;
        break;
    default:
        return 0;
    }
    actor->tile_x = (uint8_t)(actor->x / WG_FIXED_ONE);
    actor->tile_y = (uint8_t)(actor->y / WG_FIXED_ONE);
    if (WL_UsesDeathCam(actor->actor_class))
    {
        level->kill_x = level->player_x;
        level->kill_y = level->player_y;
    }
    if (drop != 0)
    {
        (void)WL_DropItem(level, actor->tile_x, actor->tile_y,
                          (uint16_t)drop, drop_item);
    }
    actor->hit_points = 0;
    actor->state = WG_STATE_DIE1;
    actor->tic_count = WL_DeathFrameDuration(actor->actor_class, 0U);
    actor->shape = WL_DeathShape(actor->actor_class, 0U);
    actor->rotate = 0U;
    actor->flags = (uint16_t)(actor->flags & ~WG_ACTOR_FLAG_SHOOTABLE);
    actor->flags |= WG_ACTOR_FLAG_NONMARK;
    WL_GivePoints(level, points);
    ++level->kill_count;
    return 1;
}

static int WL_TickPainOrDeath(wg_level_t *level, wg_actor_t *actor,
                              unsigned tics)
{
    actor->tic_count -= (int32_t)tics;
    if (WL_IsPainState(actor->state))
    {
        if (actor->tic_count <= 0)
        {
            actor->state = WG_STATE_CHASE1;
            actor->tic_count += WL_ChaseStateDuration(actor,
                                                       actor->state);
            while (actor->tic_count <= 0)
            {
                actor->state = WL_NextChaseState(actor->state);
                actor->tic_count += WL_ChaseStateDuration(actor,
                                                           actor->state);
            }
            WL_SetChaseShape(actor);
            actor->rotate = 1U;
        }
        return 1;
    }
    while (actor->tic_count <= 0 && WL_IsDeathState(actor->state))
    {
        unsigned frame = (unsigned)(actor->state - WG_STATE_DIE1) + 1U;

        if (frame >= WL_DeathTimedFrames(actor->actor_class))
        {
            if (actor->actor_class == WG_ACTOR_MECHA_HITLER
                && !WL_SpawnRealHitler(level, actor))
            {
                return 0;
            }
            actor->state = WG_STATE_DEAD;
            actor->shape = WL_DeathShape(actor->actor_class, frame);
            actor->tic_count = WL_DeathTerminalDuration(actor->actor_class);
            return 1;
        }
        actor->state = (wg_actor_state_t)(actor->state + 1);
        actor->shape = WL_DeathShape(actor->actor_class, frame);
        actor->tic_count += WL_DeathFrameDuration(actor->actor_class, frame);
    }
    return 1;
}

static int WL_IsNeedleState(wg_actor_state_t state)
{
    return state >= WG_STATE_NEEDLE1 && state <= WG_STATE_NEEDLE4;
}

static int WL_IsSmokeState(wg_actor_state_t state)
{
    return state >= WG_STATE_SMOKE1 && state <= WG_STATE_SMOKE4;
}

static int WL_IsBoomState(wg_actor_state_t state)
{
    return state >= WG_STATE_BOOM1 && state <= WG_STATE_BOOM3;
}

static int WL_IsFireState(wg_actor_state_t state)
{
    return state == WG_STATE_FIRE1 || state == WG_STATE_FIRE2;
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

static int32_t WL_ChaseStateDuration(const wg_actor_t *actor,
                                     wg_actor_state_t state)
{
    if (actor->actor_class == WG_ACTOR_REAL_HITLER)
    {
        switch (state)
        {
        case WG_STATE_CHASE1:
        case WG_STATE_CHASE3:
            return 6;
        case WG_STATE_CHASE1S:
        case WG_STATE_CHASE3S:
            return 4;
        case WG_STATE_CHASE2:
        case WG_STATE_CHASE4:
            return 2;
        default:
            return 0;
        }
    }
    switch (state)
    {
    case WG_STATE_CHASE1:
    case WG_STATE_CHASE3:
        return 10;
    case WG_STATE_CHASE1S:
    case WG_STATE_CHASE3S:
        return actor->actor_class == WG_ACTOR_MECHA_HITLER ? 6 : 3;
    case WG_STATE_CHASE2:
    case WG_STATE_CHASE4:
        return 8;
    default:
        return 0;
    }
}

static wg_actor_state_t WL_NextChaseState(wg_actor_state_t state)
{
    switch (state)
    {
    case WG_STATE_CHASE1:
        return WG_STATE_CHASE1S;
    case WG_STATE_CHASE1S:
        return WG_STATE_CHASE2;
    case WG_STATE_CHASE2:
        return WG_STATE_CHASE3;
    case WG_STATE_CHASE3:
        return WG_STATE_CHASE3S;
    case WG_STATE_CHASE3S:
        return WG_STATE_CHASE4;
    default:
        return WG_STATE_CHASE1;
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

static void WL_SetChaseShape(wg_actor_t *actor)
{
    unsigned frame;
    unsigned spacing = actor->rotate ? 8U : 1U;

    switch (actor->state)
    {
    case WG_STATE_CHASE2:
        frame = 1U;
        break;
    case WG_STATE_CHASE3:
    case WG_STATE_CHASE3S:
        frame = 2U;
        break;
    case WG_STATE_CHASE4:
        frame = 3U;
        break;
    default:
        frame = 0U;
        break;
    }
    actor->shape = (uint16_t)(actor->base_shape + frame * spacing);
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

static uint8_t WL_OppositeDirection(uint8_t direction)
{
    return direction < WG_NO_DIRECTION
               ? (uint8_t)((direction + 4U) & 7U) : WG_NO_DIRECTION;
}

static uint8_t WL_DiagonalDirection(uint8_t first, uint8_t second)
{
    uint8_t horizontal = first == 0U || first == 4U ? first : second;
    uint8_t vertical = first == 2U || first == 6U ? first : second;

    if (horizontal == 0U)
    {
        return vertical == 2U ? 1U : 7U;
    }
    if (horizontal == 4U)
    {
        return vertical == 2U ? 3U : 5U;
    }
    return WG_NO_DIRECTION;
}

static int WL_PlayerTileX(const wg_level_t *level)
{
    return level->player_x / WG_FIXED_ONE;
}

static int WL_PlayerTileY(const wg_level_t *level)
{
    return level->player_y / WG_FIXED_ONE;
}

static void WL_SelectDodgeDir(wg_level_t *level, size_t actor_index)
{
    wg_actor_t *actor = &level->actors[actor_index];
    uint8_t directions[5];
    uint8_t turnaround;
    uint8_t temporary;
    int delta_x;
    int delta_y;
    unsigned absolute_x;
    unsigned absolute_y;
    size_t index;

    if ((actor->flags & WG_ACTOR_FLAG_FIRST_ATTACK) != 0U)
    {
        turnaround = WG_NO_DIRECTION;
        actor->flags = (uint16_t)(actor->flags & 0xffdfU);
    }
    else
    {
        turnaround = WL_OppositeDirection(actor->direction);
    }
    delta_x = WL_PlayerTileX(level) - actor->tile_x;
    delta_y = WL_PlayerTileY(level) - actor->tile_y;
    if (delta_x > 0)
    {
        directions[1] = 0U;
        directions[3] = 4U;
    }
    else
    {
        directions[1] = 4U;
        directions[3] = 0U;
    }
    if (delta_y > 0)
    {
        directions[2] = 6U;
        directions[4] = 2U;
    }
    else
    {
        directions[2] = 2U;
        directions[4] = 6U;
    }
    absolute_x = (unsigned)(delta_x < 0 ? -delta_x : delta_x);
    absolute_y = (unsigned)(delta_y < 0 ? -delta_y : delta_y);
    if (absolute_x > absolute_y)
    {
        temporary = directions[1];
        directions[1] = directions[2];
        directions[2] = temporary;
        temporary = directions[3];
        directions[3] = directions[4];
        directions[4] = temporary;
    }
    if (WG_RandomNext(&level->random) < 128U)
    {
        temporary = directions[1];
        directions[1] = directions[2];
        directions[2] = temporary;
        temporary = directions[3];
        directions[3] = directions[4];
        directions[4] = temporary;
    }
    directions[0] = WL_DiagonalDirection(directions[1], directions[2]);
    for (index = 0U; index < 5U; ++index)
    {
        if (directions[index] == WG_NO_DIRECTION
            || directions[index] == turnaround)
        {
            continue;
        }
        actor->direction = directions[index];
        if (WL_TryWalk(level, actor_index))
        {
            return;
        }
    }
    if (turnaround != WG_NO_DIRECTION)
    {
        actor->direction = turnaround;
        if (WL_TryWalk(level, actor_index))
        {
            return;
        }
    }
    actor->direction = WG_NO_DIRECTION;
}

static void WL_SelectChaseDir(wg_level_t *level, size_t actor_index)
{
    wg_actor_t *actor = &level->actors[actor_index];
    uint8_t directions[3] =
    {
        WG_NO_DIRECTION, WG_NO_DIRECTION, WG_NO_DIRECTION
    };
    uint8_t old_direction = actor->direction;
    uint8_t turnaround = WL_OppositeDirection(old_direction);
    uint8_t temporary;
    int delta_x = WL_PlayerTileX(level) - actor->tile_x;
    int delta_y = WL_PlayerTileY(level) - actor->tile_y;
    int direction;

    if (delta_x > 0)
    {
        directions[1] = 0U;
    }
    else if (delta_x < 0)
    {
        directions[1] = 4U;
    }
    if (delta_y > 0)
    {
        directions[2] = 6U;
    }
    else if (delta_y < 0)
    {
        directions[2] = 2U;
    }
    if ((delta_y < 0 ? -delta_y : delta_y)
        > (delta_x < 0 ? -delta_x : delta_x))
    {
        temporary = directions[1];
        directions[1] = directions[2];
        directions[2] = temporary;
    }
    if (directions[1] == turnaround)
    {
        directions[1] = WG_NO_DIRECTION;
    }
    if (directions[2] == turnaround)
    {
        directions[2] = WG_NO_DIRECTION;
    }
    if (directions[1] != WG_NO_DIRECTION)
    {
        actor->direction = directions[1];
        if (WL_TryWalk(level, actor_index))
        {
            return;
        }
    }
    if (directions[2] != WG_NO_DIRECTION)
    {
        actor->direction = directions[2];
        if (WL_TryWalk(level, actor_index))
        {
            return;
        }
    }
    if (old_direction != WG_NO_DIRECTION)
    {
        actor->direction = old_direction;
        if (WL_TryWalk(level, actor_index))
        {
            return;
        }
    }
    if (WG_RandomNext(&level->random) > 128U)
    {
        for (direction = 2; direction <= 4; ++direction)
        {
            if ((uint8_t)direction != turnaround)
            {
                actor->direction = (uint8_t)direction;
                if (WL_TryWalk(level, actor_index))
                {
                    return;
                }
            }
        }
    }
    else
    {
        for (direction = 4; direction >= 2; --direction)
        {
            if ((uint8_t)direction != turnaround)
            {
                actor->direction = (uint8_t)direction;
                if (WL_TryWalk(level, actor_index))
                {
                    return;
                }
            }
        }
    }
    if (turnaround != WG_NO_DIRECTION)
    {
        actor->direction = turnaround;
        if (WL_TryWalk(level, actor_index))
        {
            return;
        }
    }
    actor->direction = WG_NO_DIRECTION;
}

static void WL_SelectRunDir(wg_level_t *level, size_t actor_index)
{
    static const uint8_t forward_search[] = { 2U, 3U, 4U };
    static const uint8_t reverse_search[] = { 4U, 3U, 2U };
    wg_actor_t *actor = &level->actors[actor_index];
    uint8_t directions[2];
    uint8_t temporary;
    const uint8_t *search;
    int delta_x = WL_PlayerTileX(level) - actor->tile_x;
    int delta_y = WL_PlayerTileY(level) - actor->tile_y;
    size_t index;

    directions[0] = delta_x < 0 ? 0U : 4U;
    directions[1] = delta_y < 0 ? 6U : 2U;
    if ((delta_y < 0 ? -delta_y : delta_y)
        > (delta_x < 0 ? -delta_x : delta_x))
    {
        temporary = directions[0];
        directions[0] = directions[1];
        directions[1] = temporary;
    }
    for (index = 0U; index < 2U; ++index)
    {
        actor->direction = directions[index];
        if (WL_TryWalk(level, actor_index))
        {
            return;
        }
    }
    search = WG_RandomNext(&level->random) > 128U
                 ? forward_search : reverse_search;
    for (index = 0U; index < 3U; ++index)
    {
        actor->direction = search[index];
        if (WL_TryWalk(level, actor_index))
        {
            return;
        }
    }
    actor->direction = WG_NO_DIRECTION;
}

static void WL_MoveChase(wg_level_t *level, wg_actor_t *actor, int32_t move)
{
    int32_t old_x = actor->x;
    int32_t old_y = actor->y;
    int32_t delta_x;
    int32_t delta_y;

    WL_MoveObj(actor, move);
    if (actor->area_number >= WG_NUM_AREAS
        || !level->area_by_player[actor->area_number])
    {
        return;
    }
    delta_x = actor->x - level->player_x;
    delta_y = actor->y - level->player_y;
    if (delta_x >= -WG_FIXED_ONE && delta_x <= WG_FIXED_ONE
        && delta_y >= -WG_FIXED_ONE && delta_y <= WG_FIXED_ONE)
    {
        actor->x = old_x;
        actor->y = old_y;
        actor->distance += move;
    }
}

static void WL_MoveChasingActor(wg_level_t *level, size_t actor_index,
                                int32_t tics, int dodge)
{
    wg_actor_t *actor = &level->actors[actor_index];
    int32_t move;

    if (actor->direction == WG_NO_DIRECTION)
    {
        if (dodge)
        {
            WL_SelectDodgeDir(level, actor_index);
        }
        else
        {
            WL_SelectChaseDir(level, actor_index);
        }
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

            if (door >= level->door_count)
            {
                return;
            }
            (void)WL_OpenDoor(level, door);
            if (level->doors[door].position != 0xffffU)
            {
                return;
            }
            actor->distance = WG_FIXED_ONE;
        }
        if (move < actor->distance)
        {
            WL_MoveChase(level, actor, move);
            return;
        }
        move -= actor->distance;
        actor->x = (int32_t)actor->tile_x * WG_FIXED_ONE + WG_FIXED_ONE / 2;
        actor->y = (int32_t)actor->tile_y * WG_FIXED_ONE + WG_FIXED_ONE / 2;
        if (dodge)
        {
            WL_SelectDodgeDir(level, actor_index);
        }
        else
        {
            WL_SelectChaseDir(level, actor_index);
        }
        if (actor->direction == WG_NO_DIRECTION)
        {
            return;
        }
    }
}

static int WL_UsesStandardChase(wg_actor_class_t actor_class)
{
    return actor_class == WG_ACTOR_GUARD
           || actor_class == WG_ACTOR_OFFICER
           || actor_class == WG_ACTOR_MUTANT
           || actor_class == WG_ACTOR_SS
           || actor_class == WG_ACTOR_BOSS
           || actor_class == WG_ACTOR_GRETEL
           || actor_class == WG_ACTOR_MECHA_HITLER
           || actor_class == WG_ACTOR_REAL_HITLER;
}

static void WL_BeginDogJump(wg_actor_t *actor)
{
    actor->state = WG_STATE_DOG_JUMP1;
    actor->tic_count = 10;
    actor->shape = actor->attack_shape;
}

static void WL_SetDogJumpShape(wg_actor_t *actor)
{
    switch (actor->state)
    {
    case WG_STATE_DOG_JUMP2:
        actor->shape = (uint16_t)(actor->attack_shape + 1U);
        break;
    case WG_STATE_DOG_JUMP3:
        actor->shape = (uint16_t)(actor->attack_shape + 2U);
        break;
    case WG_STATE_DOG_JUMP5:
        actor->shape = actor->base_shape;
        break;
    default:
        actor->shape = actor->attack_shape;
        break;
    }
}

static void WL_T_Bite(wg_level_t *level, const wg_actor_t *actor)
{
    int32_t delta_x = level->player_x - actor->x;
    int32_t delta_y = level->player_y - actor->y;

    delta_x = (delta_x < 0 ? -delta_x : delta_x) - WG_FIXED_ONE;
    if (delta_x > WG_FIXED_ONE)
    {
        return;
    }
    delta_y = (delta_y < 0 ? -delta_y : delta_y) - WG_FIXED_ONE;
    if (delta_y <= WG_FIXED_ONE && WG_RandomNext(&level->random) < 180U)
    {
        WL_TakeDamage(level, WG_RandomNext(&level->random) >> 4);
    }
}

static void WL_T_DogChase(wg_level_t *level, size_t actor_index,
                          int32_t tics)
{
    wg_actor_t *actor = &level->actors[actor_index];
    int32_t move;

    if (actor->direction == WG_NO_DIRECTION)
    {
        WL_SelectDodgeDir(level, actor_index);
        if (actor->direction == WG_NO_DIRECTION)
        {
            return;
        }
    }
    move = actor->speed * tics;
    while (move > 0)
    {
        int32_t delta_x = level->player_x - actor->x;
        int32_t delta_y = level->player_y - actor->y;

        delta_x = (delta_x < 0 ? -delta_x : delta_x) - move;
        delta_y = (delta_y < 0 ? -delta_y : delta_y) - move;
        if (delta_x <= WG_FIXED_ONE && delta_y <= WG_FIXED_ONE)
        {
            WL_BeginDogJump(actor);
            return;
        }
        if (actor->distance <= 0)
        {
            actor->x = (int32_t)actor->tile_x * WG_FIXED_ONE
                       + WG_FIXED_ONE / 2;
            actor->y = (int32_t)actor->tile_y * WG_FIXED_ONE
                       + WG_FIXED_ONE / 2;
            WL_SelectDodgeDir(level, actor_index);
            if (actor->direction == WG_NO_DIRECTION)
            {
                return;
            }
            continue;
        }
        if (move < actor->distance)
        {
            WL_MoveChase(level, actor, move);
            return;
        }
        move -= actor->distance;
        actor->x = (int32_t)actor->tile_x * WG_FIXED_ONE + WG_FIXED_ONE / 2;
        actor->y = (int32_t)actor->tile_y * WG_FIXED_ONE + WG_FIXED_ONE / 2;
        WL_SelectDodgeDir(level, actor_index);
        if (actor->direction == WG_NO_DIRECTION)
        {
            return;
        }
    }
}

static const wg_view_tables_t *WL_ProjectileTrigTables(void)
{
    static wg_view_tables_t tables;
    static int initialized;

    if (!initialized)
    {
        WG_ViewBuildTrigTables(&tables);
        initialized = 1;
    }
    return &tables;
}

static int WL_ProjectileTryMove(const wg_level_t *level,
                                const wg_actor_t *actor)
{
    int low_x;
    int low_y;
    int high_x;
    int high_y;
    int x;
    int y;

    if (actor->x < WG_PROJECTILE_COLLISION_SIZE
        || actor->y < WG_PROJECTILE_COLLISION_SIZE
        || actor->x >= WG_LEVEL_SIZE * WG_FIXED_ONE
                       - WG_PROJECTILE_COLLISION_SIZE
        || actor->y >= WG_LEVEL_SIZE * WG_FIXED_ONE
                       - WG_PROJECTILE_COLLISION_SIZE)
    {
        return 0;
    }
    low_x = (actor->x - WG_PROJECTILE_COLLISION_SIZE) / WG_FIXED_ONE;
    low_y = (actor->y - WG_PROJECTILE_COLLISION_SIZE) / WG_FIXED_ONE;
    high_x = (actor->x + WG_PROJECTILE_COLLISION_SIZE) / WG_FIXED_ONE;
    high_y = (actor->y + WG_PROJECTILE_COLLISION_SIZE) / WG_FIXED_ONE;
    for (y = low_y; y <= high_y; ++y)
    {
        for (x = low_x; x <= high_x; ++x)
        {
            if (level->tiles[(size_t)y * WG_LEVEL_SIZE + (size_t)x] != 0U)
            {
                return 0;
            }
        }
    }
    return 1;
}

static void WL_RemoveProjectile(wg_actor_t *actor)
{
    actor->state = WG_STATE_NONE;
    actor->flags |= WG_ACTOR_FLAG_REMOVED;
}

static wg_actor_t *WL_AllocateTransientActor(wg_level_t *level)
{
    size_t index;

    for (index = 0U; index < level->actor_count; ++index)
    {
        if ((level->actors[index].flags & WG_ACTOR_FLAG_REMOVED) != 0U)
        {
            memset(&level->actors[index], 0, sizeof(level->actors[index]));
            return &level->actors[index];
        }
    }
    if (level->actor_count >= WG_MAX_ACTORS)
    {
        return NULL;
    }
    memset(&level->actors[level->actor_count], 0,
           sizeof(level->actors[level->actor_count]));
    return &level->actors[level->actor_count++];
}

static void WL_BeginRocketExplosion(wg_actor_t *actor)
{
    actor->actor_class = WG_ACTOR_EXPLOSION;
    actor->state = WG_STATE_BOOM1;
    actor->shape = WG_SPR_BOOM1;
    actor->rotate = 0U;
    actor->speed = 0;
    if (actor->tic_count <= 0)
    {
        actor->tic_count = 6;
    }
}

static void WL_T_Projectile(wg_level_t *level, wg_actor_t *actor,
                            int32_t tics)
{
    const wg_view_tables_t *tables = WL_ProjectileTrigTables();
    const int32_t *cosine = WG_ViewCosineTable(tables);
    int32_t speed = actor->speed * tics;
    int32_t delta_x = WG_FixedMul(speed, cosine[actor->angle]);
    int32_t delta_y = -WG_FixedMul(speed, tables->sine[actor->angle]);

    if (delta_x > WG_FIXED_ONE)
    {
        delta_x = WG_FIXED_ONE;
    }
    if (delta_y > WG_FIXED_ONE)
    {
        delta_y = WG_FIXED_ONE;
    }
    actor->x += delta_x;
    actor->y += delta_y;
    if (!WL_ProjectileTryMove(level, actor))
    {
        if (actor->actor_class == WG_ACTOR_ROCKET)
        {
            WL_BeginRocketExplosion(actor);
        }
        else
        {
            WL_RemoveProjectile(actor);
        }
        return;
    }
    delta_x = actor->x - level->player_x;
    delta_y = actor->y - level->player_y;
    delta_x = delta_x < 0 ? -delta_x : delta_x;
    delta_y = delta_y < 0 ? -delta_y : delta_y;
    if (delta_x < WG_PROJECTILE_SIZE && delta_y < WG_PROJECTILE_SIZE)
    {
        unsigned base_damage;

        if (actor->actor_class == WG_ACTOR_ROCKET)
        {
            base_damage = 30U;
        }
        else if (actor->actor_class == WG_ACTOR_FIRE)
        {
            base_damage = 0U;
        }
        else
        {
            base_damage = 20U;
        }

        WL_TakeDamage(level, (WG_RandomNext(&level->random) >> 3)
                             + base_damage);
        WL_RemoveProjectile(actor);
        return;
    }
    actor->tile_x = (uint8_t)(actor->x / WG_FIXED_ONE);
    actor->tile_y = (uint8_t)(actor->y / WG_FIXED_ONE);
}

static void WL_SpawnAimedProjectile(wg_level_t *level,
                                    const wg_actor_t *actor,
                                    wg_actor_class_t projectile_class)
{
    wg_actor_t *projectile = WL_AllocateTransientActor(level);
    double angle;

    if (projectile == NULL)
    {
        return;
    }
    angle = atan2((double)(actor->y - level->player_y),
                  (double)(level->player_x - actor->x));
    if (angle < 0.0)
    {
        angle += WG_TWO_PI;
    }
    projectile->x = actor->x;
    projectile->y = actor->y;
    projectile->shape = projectile_class == WG_ACTOR_ROCKET
                            ? WG_SPR_ROCKET1 : WG_SPR_HYPO1;
    projectile->tile_x = actor->tile_x;
    projectile->tile_y = actor->tile_y;
    projectile->direction = WG_NO_DIRECTION;
    projectile->area_number = actor->area_number;
    projectile->angle = (uint16_t)(angle / WG_TWO_PI * WG_ANGLES);
    projectile->base_shape = projectile->shape;
    projectile->rotate = projectile_class == WG_ACTOR_ROCKET ? 1U : 0U;
    projectile->tic_count = 1;
    projectile->speed = 0x2000;
    projectile->state = projectile_class == WG_ACTOR_ROCKET
                            ? WG_STATE_ROCKET : WG_STATE_NEEDLE1;
    projectile->actor_class = projectile_class;
}

static void WL_T_SchabbThrow(wg_level_t *level, const wg_actor_t *actor)
{
    WL_SpawnAimedProjectile(level, actor, WG_ACTOR_NEEDLE);
}

static void WL_T_GiftThrow(wg_level_t *level, const wg_actor_t *actor)
{
    WL_SpawnAimedProjectile(level, actor, WG_ACTOR_ROCKET);
}

static void WL_T_FakeFire(wg_level_t *level, const wg_actor_t *actor)
{
    wg_actor_t *fire = WL_AllocateTransientActor(level);
    double angle;

    if (fire == NULL)
    {
        return;
    }
    angle = atan2((double)(actor->y - level->player_y),
                  (double)(level->player_x - actor->x));
    if (angle < 0.0)
    {
        angle += WG_TWO_PI;
    }
    fire->x = actor->x;
    fire->y = actor->y;
    fire->shape = WG_SPR_FIRE1;
    fire->tile_x = actor->tile_x;
    fire->tile_y = actor->tile_y;
    fire->direction = WG_NO_DIRECTION;
    fire->area_number = actor->area_number;
    fire->angle = (uint16_t)(angle / WG_TWO_PI * WG_ANGLES);
    fire->base_shape = WG_SPR_FIRE1;
    fire->tic_count = 1;
    fire->speed = 0x1200;
    fire->state = WG_STATE_FIRE1;
    fire->actor_class = WG_ACTOR_FIRE;
}

static void WL_A_Smoke(wg_level_t *level, const wg_actor_t *rocket)
{
    wg_actor_t *smoke = WL_AllocateTransientActor(level);

    if (smoke == NULL)
    {
        return;
    }
    smoke->x = rocket->x;
    smoke->y = rocket->y;
    smoke->shape = WG_SPR_SMOKE1;
    smoke->tile_x = rocket->tile_x;
    smoke->tile_y = rocket->tile_y;
    smoke->direction = WG_NO_DIRECTION;
    smoke->area_number = rocket->area_number;
    smoke->base_shape = WG_SPR_SMOKE1;
    smoke->tic_count = 6;
    smoke->state = WG_STATE_SMOKE1;
    smoke->actor_class = WG_ACTOR_SMOKE;
}

static void WL_MoveSpecialBoss(wg_level_t *level, size_t actor_index,
                               int32_t tics, int dodge, int run_close,
                               int distance)
{
    wg_actor_t *actor = &level->actors[actor_index];
    int32_t move;

    if (actor->direction == WG_NO_DIRECTION)
    {
        if (dodge)
        {
            WL_SelectDodgeDir(level, actor_index);
        }
        else
        {
            WL_SelectChaseDir(level, actor_index);
        }
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

            if (door >= level->door_count)
            {
                return;
            }
            (void)WL_OpenDoor(level, door);
            if (level->doors[door].position != 0xffffU)
            {
                return;
            }
            actor->distance = WG_FIXED_ONE;
        }
        if (move < actor->distance)
        {
            WL_MoveChase(level, actor, move);
            return;
        }
        move -= actor->distance;
        actor->x = (int32_t)actor->tile_x * WG_FIXED_ONE + WG_FIXED_ONE / 2;
        actor->y = (int32_t)actor->tile_y * WG_FIXED_ONE + WG_FIXED_ONE / 2;
        if (run_close && distance < 4)
        {
            WL_SelectRunDir(level, actor_index);
        }
        else if (dodge)
        {
            WL_SelectDodgeDir(level, actor_index);
        }
        else
        {
            WL_SelectChaseDir(level, actor_index);
        }
        if (actor->direction == WG_NO_DIRECTION)
        {
            return;
        }
    }
}

static void WL_T_SpecialBoss(wg_level_t *level, size_t actor_index,
                             int32_t tics, int run_close)
{
    wg_actor_t *actor = &level->actors[actor_index];
    int delta_x = (int)actor->tile_x - WL_PlayerTileX(level);
    int delta_y = (int)actor->tile_y - WL_PlayerTileY(level);
    int distance;
    int dodge = 0;

    delta_x = delta_x < 0 ? -delta_x : delta_x;
    delta_y = delta_y < 0 ? -delta_y : delta_y;
    distance = delta_x > delta_y ? delta_x : delta_y;

    if (WL_CheckLine(level, actor))
    {
        if (WG_RandomNext(&level->random) < tics * 8)
        {
            WL_BeginAttack(actor);
            return;
        }
        dodge = 1;
    }
    WL_MoveSpecialBoss(level, actor_index, tics, dodge, run_close, distance);
}

static void WL_T_Fake(wg_level_t *level, size_t actor_index, int32_t tics)
{
    wg_actor_t *actor = &level->actors[actor_index];

    if (WL_CheckLine(level, actor)
        && WG_RandomNext(&level->random) < tics * 2)
    {
        WL_BeginAttack(actor);
        return;
    }
    WL_MoveSpecialBoss(level, actor_index, tics, 1, 0, 0);
}

static int32_t WL_ShootStateDuration(const wg_actor_t *actor,
                                     wg_actor_state_t state)
{
    unsigned stage = (unsigned)(state - WG_STATE_SHOOT1) + 1U;

    switch (actor->actor_class)
    {
    case WG_ACTOR_GUARD:
        return stage <= 3U ? 20 : 0;
    case WG_ACTOR_OFFICER:
        if (stage == 1U)
        {
            return 6;
        }
        return stage == 2U ? 20 : (stage == 3U ? 10 : 0);
    case WG_ACTOR_MUTANT:
        if (stage == 1U)
        {
            return 6;
        }
        if (stage == 2U || stage == 4U)
        {
            return 20;
        }
        return stage == 3U ? 10 : 0;
    case WG_ACTOR_SS:
        if (stage == 1U || stage == 2U)
        {
            return 20;
        }
        return stage <= 9U ? 10 : 0;
    case WG_ACTOR_BOSS:
    case WG_ACTOR_GRETEL:
        return stage == 1U ? 30 : (stage <= 8U ? 10 : 0);
    case WG_ACTOR_MECHA_HITLER:
    case WG_ACTOR_REAL_HITLER:
        return stage == 1U ? 30 : (stage <= 6U ? 10 : 0);
    case WG_ACTOR_SCHABBS:
        return stage == 1U ? 30 : (stage == 2U ? 10 : 0);
    case WG_ACTOR_GIFT:
        return stage == 1U ? 30 : (stage == 2U ? 10 : 0);
    case WG_ACTOR_FAT:
        return stage == 1U ? 30 : (stage <= 6U ? 10 : 0);
    case WG_ACTOR_FAKE:
        return stage <= 9U ? 8 : 0;
    default:
        return 0;
    }
}

static unsigned WL_ShootStateCount(wg_actor_class_t actor_class)
{
    switch (actor_class)
    {
    case WG_ACTOR_GUARD:
    case WG_ACTOR_OFFICER:
        return 3U;
    case WG_ACTOR_MUTANT:
        return 4U;
    case WG_ACTOR_SS:
        return 9U;
    case WG_ACTOR_BOSS:
    case WG_ACTOR_GRETEL:
        return 8U;
    case WG_ACTOR_MECHA_HITLER:
    case WG_ACTOR_REAL_HITLER:
        return 6U;
    case WG_ACTOR_SCHABBS:
        return 2U;
    case WG_ACTOR_GIFT:
        return 2U;
    case WG_ACTOR_FAT:
        return 6U;
    case WG_ACTOR_FAKE:
        return 9U;
    default:
        return 0U;
    }
}

static int WL_ShootStateHasAction(const wg_actor_t *actor)
{
    unsigned stage = (unsigned)(actor->state - WG_STATE_SHOOT1) + 1U;

    switch (actor->actor_class)
    {
    case WG_ACTOR_GUARD:
    case WG_ACTOR_OFFICER:
        return stage == 2U;
    case WG_ACTOR_MUTANT:
        return stage == 1U || stage == 3U;
    case WG_ACTOR_SS:
        return stage == 2U || stage == 4U
               || stage == 6U || stage == 8U;
    case WG_ACTOR_BOSS:
    case WG_ACTOR_GRETEL:
        return stage >= 2U && stage <= 7U;
    case WG_ACTOR_MECHA_HITLER:
    case WG_ACTOR_REAL_HITLER:
        return stage >= 2U && stage <= 6U;
    case WG_ACTOR_SCHABBS:
        return stage == 2U;
    case WG_ACTOR_GIFT:
        return stage == 2U;
    case WG_ACTOR_FAT:
        return stage >= 2U && stage <= 6U;
    case WG_ACTOR_FAKE:
        return stage <= 8U;
    default:
        return 0;
    }
}

static unsigned WL_ShootShapeFrame(const wg_actor_t *actor)
{
    unsigned stage = (unsigned)(actor->state - WG_STATE_SHOOT1) + 1U;

    if (actor->actor_class == WG_ACTOR_FAKE)
    {
        return 0U;
    }
    if ((actor->actor_class == WG_ACTOR_SS && stage >= 4U)
        || actor->actor_class == WG_ACTOR_BOSS
        || actor->actor_class == WG_ACTOR_GRETEL
        || actor->actor_class == WG_ACTOR_MECHA_HITLER
        || actor->actor_class == WG_ACTOR_REAL_HITLER)
    {
        if (stage == 1U
            || ((actor->actor_class == WG_ACTOR_BOSS
                 || actor->actor_class == WG_ACTOR_GRETEL)
                && stage == 8U))
        {
            return 0U;
        }
        return (stage & 1U) == 0U ? 1U : 2U;
    }
    if (actor->actor_class == WG_ACTOR_FAT && stage >= 5U)
    {
        return stage == 5U ? 2U : 3U;
    }
    return stage - 1U;
}

static void WL_SetShootShape(wg_actor_t *actor)
{
    actor->shape = (uint16_t)(actor->attack_shape
                              + WL_ShootShapeFrame(actor));
}

static int WL_BeginAttack(wg_actor_t *actor)
{
    if (actor->attack_shape == 0U || WL_ShootStateCount(actor->actor_class) == 0U)
    {
        actor->state = WG_STATE_ATTACK_PENDING;
        actor->tic_count = 0;
        actor->flags |= WG_ACTOR_FLAG_ATTACK_PENDING;
        return 0;
    }
    actor->state = WG_STATE_SHOOT1;
    actor->tic_count = WL_ShootStateDuration(actor, actor->state);
    actor->shape = actor->attack_shape;
    actor->flags = (uint16_t)(actor->flags & 0xfeffU);
    return 1;
}

static void WL_T_Shoot(wg_level_t *level, wg_actor_t *actor)
{
    int delta_x;
    int delta_y;
    int distance;
    int hit_chance;
    unsigned damage;

    if (actor->area_number >= WG_NUM_AREAS
        || !level->area_by_player[actor->area_number]
        || !WL_CheckLine(level, actor))
    {
        return;
    }
    delta_x = (int)actor->tile_x - WL_PlayerTileX(level);
    delta_y = (int)actor->tile_y - WL_PlayerTileY(level);
    delta_x = delta_x < 0 ? -delta_x : delta_x;
    delta_y = delta_y < 0 ? -delta_y : delta_y;
    distance = delta_x > delta_y ? delta_x : delta_y;
    if (actor->actor_class == WG_ACTOR_SS
        || actor->actor_class == WG_ACTOR_BOSS)
    {
        distance = distance * 2 / 3;
    }
    if (level->player_thrust_speed >= 6000)
    {
        hit_chance = (actor->flags & WG_ACTOR_FLAG_VISIBLE) != 0U
                         ? 160 - distance * 16 : 160 - distance * 8;
    }
    else
    {
        hit_chance = (actor->flags & WG_ACTOR_FLAG_VISIBLE) != 0U
                         ? 256 - distance * 16 : 256 - distance * 8;
    }
    if (WG_RandomNext(&level->random) < hit_chance)
    {
        unsigned random_damage = WG_RandomNext(&level->random);

        damage = distance < 2 ? random_damage >> 2
                 : (distance < 4 ? random_damage >> 3
                                  : random_damage >> 4);
        WL_TakeDamage(level, damage);
    }
}

static void WL_T_Chase(wg_level_t *level, size_t actor_index, int32_t tics)
{
    wg_actor_t *actor = &level->actors[actor_index];
    int dodge = 0;

    if (WL_CheckLine(level, actor))
    {
        int delta_x = (int)actor->tile_x - WL_PlayerTileX(level);
        int delta_y = (int)actor->tile_y - WL_PlayerTileY(level);
        int distance;
        int chance;

        delta_x = delta_x < 0 ? -delta_x : delta_x;
        delta_y = delta_y < 0 ? -delta_y : delta_y;
        distance = delta_x > delta_y ? delta_x : delta_y;
        chance = distance == 0
                     || (distance == 1 && actor->distance < 0x4000)
                     ? 300 : (tics * 16) / distance;
        if (WG_RandomNext(&level->random) < chance)
        {
            WL_BeginAttack(actor);
            return;
        }
        dodge = 1;
    }
    WL_MoveChasingActor(level, actor_index, tics, dodge);
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

            if (door >= level->door_count)
            {
                return;
            }
            (void)WL_OpenDoor(level, door);
            if (level->doors[door].position != 0xffffU)
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
    int connectivity_ready = 0;

    if (level == NULL || tics > (unsigned)(INT32_MAX / 10000))
    {
        return 0;
    }
    for (index = 0; index < level->actor_count; ++index)
    {
        wg_actor_t *actor = &level->actors[index];

        if ((actor->flags & WG_ACTOR_FLAG_REMOVED) != 0U)
        {
            continue;
        }
        if (WL_IsPainState(actor->state) || WL_IsDeathState(actor->state))
        {
            if (!WL_TickPainOrDeath(level, actor, tics))
            {
                return 0;
            }
            continue;
        }
        if (actor->state == WG_STATE_DEAD)
        {
            int32_t terminal_duration =
                WL_DeathTerminalDuration(actor->actor_class);

            if (terminal_duration != 0)
            {
                actor->tic_count -= (int32_t)tics;
                while (actor->tic_count <= 0)
                {
                    if (WL_UsesDeathCam(actor->actor_class))
                    {
                        if (level->victory_flag)
                        {
                            level->level_completed = 1U;
                        }
                        else
                        {
                            level->victory_flag = 1U;
                        }
                    }
                    actor->tic_count += terminal_duration;
                }
            }
            continue;
        }
        if (WL_IsNeedleState(actor->state))
        {
            actor->tic_count -= (int32_t)tics;
            while (actor->tic_count <= 0)
            {
                actor->state = actor->state == WG_STATE_NEEDLE4
                                   ? WG_STATE_NEEDLE1
                                   : (wg_actor_state_t)(actor->state + 1);
                actor->tic_count += 6;
                actor->shape = (uint16_t)(WG_SPR_HYPO1
                                          + actor->state
                                            - WG_STATE_NEEDLE1);
            }
            WL_T_Projectile(level, actor, (int32_t)tics);
            continue;
        }
        if (WL_IsFireState(actor->state))
        {
            actor->tic_count -= (int32_t)tics;
            while (actor->tic_count <= 0)
            {
                actor->state = actor->state == WG_STATE_FIRE1
                                   ? WG_STATE_FIRE2 : WG_STATE_FIRE1;
                actor->tic_count += 6;
                actor->shape = (uint16_t)(WG_SPR_FIRE1
                                          + actor->state - WG_STATE_FIRE1);
            }
            WL_T_Projectile(level, actor, (int32_t)tics);
            continue;
        }
        if (actor->state == WG_STATE_ROCKET)
        {
            actor->tic_count -= (int32_t)tics;
            while (actor->tic_count <= 0)
            {
                WL_A_Smoke(level, actor);
                actor->tic_count += 3;
            }
            WL_T_Projectile(level, actor, (int32_t)tics);
            continue;
        }
        if (WL_IsSmokeState(actor->state))
        {
            actor->tic_count -= (int32_t)tics;
            while (actor->tic_count <= 0)
            {
                if (actor->state == WG_STATE_SMOKE4)
                {
                    WL_RemoveProjectile(actor);
                    break;
                }
                actor->state = (wg_actor_state_t)(actor->state + 1);
                actor->tic_count += 3;
                actor->shape = (uint16_t)(WG_SPR_SMOKE1
                                          + actor->state
                                            - WG_STATE_SMOKE1);
            }
            continue;
        }
        if (WL_IsBoomState(actor->state))
        {
            actor->tic_count -= (int32_t)tics;
            while (actor->tic_count <= 0)
            {
                if (actor->state == WG_STATE_BOOM3)
                {
                    WL_RemoveProjectile(actor);
                    break;
                }
                actor->state = (wg_actor_state_t)(actor->state + 1);
                actor->tic_count += 6;
                actor->shape = (uint16_t)(WG_SPR_BOOM1
                                          + actor->state
                                            - WG_STATE_BOOM1);
            }
            continue;
        }
        if (WL_IsPathState(actor->state))
        {
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
            if (WL_PathHasThink(actor->state))
            {
                WL_T_Path(level, index, (int32_t)tics);
            }
            continue;
        }
        if (WL_IsChaseState(actor->state))
        {
            if (actor->tic_count != 0)
            {
                actor->tic_count -= (int32_t)tics;
                while (actor->tic_count <= 0)
                {
                    actor->state = WL_NextChaseState(actor->state);
                    actor->tic_count += WL_ChaseStateDuration(actor,
                                                              actor->state);
                    WL_SetChaseShape(actor);
                }
            }
            if (!WL_ChaseHasThink(actor->state))
            {
                continue;
            }
            if (actor->actor_class != WG_ACTOR_DOG
                && actor->actor_class != WG_ACTOR_SCHABBS
                && actor->actor_class != WG_ACTOR_FAKE
                && actor->actor_class != WG_ACTOR_GIFT
                && actor->actor_class != WG_ACTOR_FAT
                && !WL_UsesStandardChase(actor->actor_class))
            {
                continue;
            }
            if (!connectivity_ready)
            {
                if (!WL_UpdateAreaConnectivity(level))
                {
                    return 0;
                }
                connectivity_ready = 1;
            }
            if (actor->actor_class == WG_ACTOR_DOG)
            {
                WL_T_DogChase(level, index, (int32_t)tics);
            }
            else if (actor->actor_class == WG_ACTOR_SCHABBS)
            {
                WL_T_SpecialBoss(level, index, (int32_t)tics, 0);
            }
            else if (actor->actor_class == WG_ACTOR_FAKE)
            {
                WL_T_Fake(level, index, (int32_t)tics);
            }
            else if (actor->actor_class == WG_ACTOR_GIFT
                     || actor->actor_class == WG_ACTOR_FAT)
            {
                WL_T_SpecialBoss(level, index, (int32_t)tics, 1);
            }
            else
            {
                WL_T_Chase(level, index, (int32_t)tics);
            }
            continue;
        }
        if (WL_IsShootState(actor->state))
        {
            if (!connectivity_ready)
            {
                if (!WL_UpdateAreaConnectivity(level))
                {
                    return 0;
                }
                connectivity_ready = 1;
            }
            actor->tic_count -= (int32_t)tics;
            while (actor->tic_count <= 0 && WL_IsShootState(actor->state))
            {
                unsigned stage = (unsigned)(actor->state - WG_STATE_SHOOT1)
                                 + 1U;

                if (WL_ShootStateHasAction(actor))
                {
                    if (actor->actor_class == WG_ACTOR_SCHABBS)
                    {
                        WL_T_SchabbThrow(level, actor);
                    }
                    else if (actor->actor_class == WG_ACTOR_FAKE)
                    {
                        WL_T_FakeFire(level, actor);
                    }
                    else if ((actor->actor_class == WG_ACTOR_GIFT
                              || actor->actor_class == WG_ACTOR_FAT)
                             && stage == 2U)
                    {
                        WL_T_GiftThrow(level, actor);
                    }
                    else
                    {
                        WL_T_Shoot(level, actor);
                    }
                }
                if (stage >= WL_ShootStateCount(actor->actor_class))
                {
                    actor->state = WG_STATE_CHASE1;
                    actor->tic_count += WL_ChaseStateDuration(actor,
                                                              actor->state);
                    WL_SetChaseShape(actor);
                }
                else
                {
                    actor->state = (wg_actor_state_t)(actor->state + 1);
                    actor->tic_count += WL_ShootStateDuration(actor,
                                                              actor->state);
                    WL_SetShootShape(actor);
                }
            }
            while (actor->tic_count <= 0 && WL_IsChaseState(actor->state))
            {
                actor->state = WL_NextChaseState(actor->state);
                actor->tic_count += WL_ChaseStateDuration(actor,
                                                          actor->state);
                WL_SetChaseShape(actor);
            }
            if (WL_ChaseHasThink(actor->state)
                && WL_UsesStandardChase(actor->actor_class))
            {
                WL_T_Chase(level, index, (int32_t)tics);
            }
            else if (WL_ChaseHasThink(actor->state)
                     && actor->actor_class == WG_ACTOR_SCHABBS)
            {
                WL_T_SpecialBoss(level, index, (int32_t)tics, 0);
            }
            else if (WL_ChaseHasThink(actor->state)
                     && actor->actor_class == WG_ACTOR_FAKE)
            {
                WL_T_Fake(level, index, (int32_t)tics);
            }
            else if (WL_ChaseHasThink(actor->state)
                     && (actor->actor_class == WG_ACTOR_GIFT
                         || actor->actor_class == WG_ACTOR_FAT))
            {
                WL_T_SpecialBoss(level, index, (int32_t)tics, 1);
            }
            continue;
        }
        if (WL_IsDogJumpState(actor->state))
        {
            actor->tic_count -= (int32_t)tics;
            while (actor->tic_count <= 0 && WL_IsDogJumpState(actor->state))
            {
                if (actor->state == WG_STATE_DOG_JUMP2)
                {
                    WL_T_Bite(level, actor);
                }
                if (actor->state == WG_STATE_DOG_JUMP5)
                {
                    actor->state = WG_STATE_CHASE1;
                    actor->tic_count += WL_ChaseStateDuration(actor,
                                                              actor->state);
                    WL_SetChaseShape(actor);
                }
                else
                {
                    actor->state = (wg_actor_state_t)(actor->state + 1);
                    actor->tic_count += 10;
                    WL_SetDogJumpShape(actor);
                }
            }
            while (actor->tic_count <= 0 && WL_IsChaseState(actor->state))
            {
                actor->state = WL_NextChaseState(actor->state);
                actor->tic_count += WL_ChaseStateDuration(actor,
                                                          actor->state);
                WL_SetChaseShape(actor);
            }
            if (WL_ChaseHasThink(actor->state))
            {
                if (!connectivity_ready)
                {
                    if (!WL_UpdateAreaConnectivity(level))
                    {
                        return 0;
                    }
                    connectivity_ready = 1;
                }
                WL_T_DogChase(level, index, (int32_t)tics);
            }
        }
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

        if (door->action == WG_DOOR_CLOSED
            && door->position != 0xffffU)
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

int WL_DamageActor(wg_level_t *level, size_t actor_index, unsigned damage)
{
    wg_actor_t *actor;
    uint16_t pain_shape;

    if (level == NULL || actor_index >= level->actor_count
        || damage > (unsigned)(INT32_MAX / 2))
    {
        return 0;
    }
    actor = &level->actors[actor_index];
    if ((actor->flags & WG_ACTOR_FLAG_SHOOTABLE) == 0U
        || WL_DeathTimedFrames(actor->actor_class) == 0U)
    {
        return 0;
    }
    level->made_noise = 1U;
    if ((actor->flags & WG_ACTOR_FLAG_ATTACK_MODE) == 0U)
    {
        damage <<= 1;
    }
    actor->hit_points -= (int32_t)damage;
    if (actor->hit_points <= 0)
    {
        return WL_KillActor(level, actor_index);
    }
    if ((actor->flags & WG_ACTOR_FLAG_ATTACK_MODE) == 0U)
    {
        WL_FirstSighting(actor);
    }
    pain_shape = WL_PainShape(actor->actor_class,
                              (actor->hit_points & 1) == 0);
    if (pain_shape != 0U)
    {
        actor->state = (actor->hit_points & 1) != 0
                           ? WG_STATE_PAIN1 : WG_STATE_PAIN2;
        actor->shape = pain_shape;
        actor->rotate = 2U;
        actor->tic_count = 10;
    }
    return 1;
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
