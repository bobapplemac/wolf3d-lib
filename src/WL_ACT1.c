/* Portable door and pushwall actions from the original WL_ACT1.C. */
#include "WL_ACT1.h"

#include <stddef.h>

#include "WG_FIXED.h"
#include "WL_GAME.h"
#include "WL_MAIN.h"
#include "WL_STATE.h"

#define WG_DOOR_OPEN_TICS 300U

static int WL_DoorIsObstructed(const wg_level_t *level,
                               const wg_door_t *door)
{
    size_t index;

    if (level->player_tile_x == door->tile_x
        && level->player_tile_y == door->tile_y)
    {
        return 1;
    }
    if (door->vertical != 0U && level->player_tile_y == door->tile_y
        && (((level->player_x + WG_MIN_DISTANCE) / WG_FIXED_ONE
             == door->tile_x)
            || ((level->player_x - WG_MIN_DISTANCE) / WG_FIXED_ONE
                == door->tile_x)))
    {
        return 1;
    }
    if (door->vertical == 0U && level->player_tile_x == door->tile_x
        && (((level->player_y + WG_MIN_DISTANCE) / WG_FIXED_ONE
             == door->tile_y)
            || ((level->player_y - WG_MIN_DISTANCE) / WG_FIXED_ONE
                == door->tile_y)))
    {
        return 1;
    }
    for (index = 0U; index < level->actor_count; ++index)
    {
        const wg_actor_t *actor = &level->actors[index];

        if ((actor->flags & WG_ACTOR_FLAG_SHOOTABLE) == 0U
            || (actor->flags & WG_ACTOR_FLAG_REMOVED) != 0U)
        {
            continue;
        }
        if (actor->tile_x == door->tile_x && actor->tile_y == door->tile_y)
        {
            return 1;
        }
        if (door->vertical != 0U && actor->tile_y == door->tile_y
            && (((actor->x + WG_MIN_DISTANCE) / WG_FIXED_ONE
                 == door->tile_x)
                || ((actor->x - WG_MIN_DISTANCE) / WG_FIXED_ONE
                    == door->tile_x)))
        {
            return 1;
        }
        if (door->vertical == 0U && actor->tile_x == door->tile_x
            && (((actor->y + WG_MIN_DISTANCE) / WG_FIXED_ONE
                 == door->tile_y)
                || ((actor->y - WG_MIN_DISTANCE) / WG_FIXED_ONE
                    == door->tile_y)))
        {
            return 1;
        }
    }
    return 0;
}

int WL_OpenDoor(struct wg_level *level, size_t door_index)
{
    wg_door_t *door;

    if (level == NULL || door_index >= level->door_count)
    {
        return 0;
    }
    door = &level->doors[door_index];
    if (door->action == WG_DOOR_OPEN)
    {
        door->tic_count = 0U;
    }
    else
    {
        door->action = WG_DOOR_OPENING;
        (void)WL_UpdateAreaConnectivity(level);
    }
    return 1;
}

int WL_CloseDoor(struct wg_level *level, size_t door_index)
{
    wg_door_t *door;

    if (level == NULL || door_index >= level->door_count)
    {
        return 0;
    }
    door = &level->doors[door_index];
    if (WL_DoorIsObstructed(level, door))
    {
        return 0;
    }
    (void)WG_QueueSoundAt(level, WG_SOUND_CLOSE_DOOR,
                          door->tile_x * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                          door->tile_y * WG_FIXED_ONE + WG_FIXED_ONE / 2);
    door->action = WG_DOOR_CLOSING;
    return 1;
}

int WL_OperateDoor(struct wg_level *level, size_t door_index)
{
    wg_door_t *door;

    if (level == NULL || door_index >= level->door_count)
    {
        return 0;
    }
    door = &level->doors[door_index];
    if (door->lock >= WG_DOOR_LOCK_1 && door->lock <= WG_DOOR_LOCK_4
        && (level->player_keys
            & (1U << (door->lock - WG_DOOR_LOCK_1))) == 0U)
    {
        (void)WG_QueueSound(level, WG_SOUND_NOWAY);
        return 0;
    }
    if (door->action == WG_DOOR_CLOSED
        || door->action == WG_DOOR_CLOSING)
    {
        return WL_OpenDoor(level, door_index);
    }
    return WL_CloseDoor(level, door_index);
}

int WL_MoveDoors(struct wg_level *level, unsigned tics)
{
    size_t index;

    if (level == NULL)
    {
        return 0;
    }
    if (level->victory_flag != 0U)
    {
        return 1;
    }
    for (index = 0U; index < level->door_count; ++index)
    {
        wg_door_t *door = &level->doors[index];

        if (door->action == WG_DOOR_OPEN)
        {
            uint64_t elapsed = (uint64_t)door->tic_count + tics;

            door->tic_count = (uint16_t)(elapsed > UINT16_MAX
                                             ? UINT16_MAX : elapsed);
            if (elapsed >= WG_DOOR_OPEN_TICS)
            {
                (void)WL_CloseDoor(level, index);
            }
        }
        else if (door->action == WG_DOOR_OPENING)
        {
            if (door->position == 0U)
            {
                (void)WG_QueueSoundAt(
                    level, WG_SOUND_OPEN_DOOR,
                    door->tile_x * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                    door->tile_y * WG_FIXED_ONE + WG_FIXED_ONE / 2);
            }
            if (tics >= 64U
                || (uint32_t)door->position + ((uint32_t)tics << 10)
                       >= 0xffffU)
            {
                door->position = 0xffffU;
                door->tic_count = 0U;
                door->action = WG_DOOR_OPEN;
            }
            else
            {
                door->position = (uint16_t)(door->position
                                             + ((uint32_t)tics << 10));
            }
        }
        else if (door->action == WG_DOOR_CLOSING)
        {
            uint32_t movement = tics >= 64U ? 0xffffU
                                            : (uint32_t)tics << 10;

            if (WL_DoorIsObstructed(level, door))
            {
                (void)WL_OpenDoor(level, index);
            }
            else if (movement >= door->position)
            {
                door->position = 0U;
                door->action = WG_DOOR_CLOSED;
                (void)WL_UpdateAreaConnectivity(level);
            }
            else
            {
                door->position = (uint16_t)(door->position - movement);
            }
        }
    }
    return 1;
}

static int WL_PushWallSpotBlocked(const wg_level_t *level, int tile_x,
                                  int tile_y)
{
    size_t index;

    if (tile_x < 0 || tile_x >= WG_LEVEL_SIZE
        || tile_y < 0 || tile_y >= WG_LEVEL_SIZE
        || level->tiles[(size_t)tile_y * WG_LEVEL_SIZE
                        + (size_t)tile_x] != 0U)
    {
        return 1;
    }
    for (index = 0U; index < level->actor_count; ++index)
    {
        const wg_actor_t *actor = &level->actors[index];

        if ((actor->flags & WG_ACTOR_FLAG_SHOOTABLE) != 0U
            && (actor->flags & WG_ACTOR_FLAG_REMOVED) == 0U
            && actor->tile_x == (uint8_t)tile_x
            && actor->tile_y == (uint8_t)tile_y)
        {
            return 1;
        }
    }
    for (index = 0U; index < level->static_count; ++index)
    {
        const wg_static_object_t *object = &level->statics[index];

        if (object->removed == 0U && object->blocking != 0U
            && object->tile_x == (uint8_t)tile_x
            && object->tile_y == (uint8_t)tile_y)
        {
            return 1;
        }
    }
    return 0;
}

static int WL_PushWallStep(uint8_t direction, int *delta_x, int *delta_y)
{
    if (delta_x == NULL || delta_y == NULL)
    {
        return 0;
    }
    *delta_x = 0;
    *delta_y = 0;
    switch (direction)
    {
    case 0U:
        *delta_x = 1;
        return 1;
    case 2U:
        *delta_y = -1;
        return 1;
    case 4U:
        *delta_x = -1;
        return 1;
    case 6U:
        *delta_y = 1;
        return 1;
    default:
        return 0;
    }
}

int WL_PushWall(struct wg_level *level, uint8_t tile_x, uint8_t tile_y,
                uint8_t direction)
{
    int delta_x;
    int delta_y;
    int destination_x;
    int destination_y;
    size_t source;
    size_t destination;
    uint8_t old_tile;

    if (level == NULL || level->pushwall_state != 0U
        || tile_x >= WG_LEVEL_SIZE || tile_y >= WG_LEVEL_SIZE
        || !WL_PushWallStep(direction, &delta_x, &delta_y))
    {
        return 0;
    }
    source = (size_t)tile_y * WG_LEVEL_SIZE + tile_x;
    old_tile = level->tiles[source];
    if ((old_tile & 63U) == 0U || (old_tile & 0x80U) != 0U)
    {
        return 0;
    }
    destination_x = (int)tile_x + delta_x;
    destination_y = (int)tile_y + delta_y;
    if (WL_PushWallSpotBlocked(level, destination_x, destination_y))
    {
        (void)WG_QueueSound(level, WG_SOUND_NOWAY);
        return 0;
    }
    destination = (size_t)destination_y * WG_LEVEL_SIZE
                  + (size_t)destination_x;
    level->tiles[destination] = old_tile;
    ++level->secret_count;
    level->pushwall_x = tile_x;
    level->pushwall_y = tile_y;
    level->pushwall_direction = direction;
    level->pushwall_state = 1U;
    level->pushwall_position = 0U;
    level->tiles[source] = (uint8_t)(old_tile | 0xc0U);
    level->info[source] = 0U;
    (void)WG_QueueSound(level, WG_SOUND_PUSHWALL);
    return 1;
}

int WL_MovePushWalls(struct wg_level *level, unsigned tics)
{
    static const int direction_x[] = { 1, 0, -1, 0 };
    static const int direction_y[] = { 0, -1, 0, 1 };
    unsigned old_block;
    unsigned direction_index;
    unsigned next_state;
    int delta_x;
    int delta_y;
    int next_x;
    int next_y;
    size_t current;
    size_t next;
    uint8_t old_tile;
    uint8_t player_area = 0U;

    if (level == NULL)
    {
        return 0;
    }
    if (level->pushwall_state == 0U)
    {
        return 1;
    }
    if (tics > (unsigned)UINT16_MAX - level->pushwall_state)
    {
        return 0;
    }
    old_block = level->pushwall_state / 128U;
    next_state = level->pushwall_state + tics;
    level->pushwall_state = (uint16_t)next_state;
    if (next_state / 128U != old_block)
    {
        current = (size_t)level->pushwall_y * WG_LEVEL_SIZE
                  + level->pushwall_x;
        old_tile = (uint8_t)(level->tiles[current] & 63U);
        level->tiles[current] = 0U;
        if (level->player_tile_x < WG_LEVEL_SIZE
            && level->player_tile_y < WG_LEVEL_SIZE)
        {
            uint8_t area = level->areas[(size_t)level->player_tile_y
                                        * WG_LEVEL_SIZE
                                        + level->player_tile_x];

            if (area < WG_NUM_AREAS)
            {
                player_area = area;
            }
        }
        level->areas[current] = player_area;
        if (next_state > 256U)
        {
            level->pushwall_state = 0U;
            return 1;
        }
        direction_index = level->pushwall_direction / 2U;
        if (direction_index >= 4U)
        {
            return 0;
        }
        delta_x = direction_x[direction_index];
        delta_y = direction_y[direction_index];
        level->pushwall_x = (uint8_t)((int)level->pushwall_x + delta_x);
        level->pushwall_y = (uint8_t)((int)level->pushwall_y + delta_y);
        next_x = (int)level->pushwall_x + delta_x;
        next_y = (int)level->pushwall_y + delta_y;
        if (WL_PushWallSpotBlocked(level, next_x, next_y))
        {
            level->pushwall_state = 0U;
            return 1;
        }
        next = (size_t)next_y * WG_LEVEL_SIZE + (size_t)next_x;
        level->tiles[next] = old_tile;
        current = (size_t)level->pushwall_y * WG_LEVEL_SIZE
                  + level->pushwall_x;
        level->tiles[current] = (uint8_t)(old_tile | 0xc0U);
    }
    level->pushwall_position = (uint8_t)((next_state / 2U) & 63U);
    return 1;
}
