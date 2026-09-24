/* Portable SetupGameLevel, ScanInfoPlane, SpawnDoor, and SpawnStatic state. */
#include "WL_GAME.h"

#include <string.h>

static int WG_DifficultyDirection(uint16_t info, uint16_t easy_base,
                                  uint16_t tier_spacing,
                                  wg_difficulty_t difficulty,
                                  uint8_t *direction)
{
    unsigned tier;

    if (direction == NULL)
    {
        return 0;
    }
    for (tier = 0; tier < 3U; ++tier)
    {
        uint16_t base = (uint16_t)(easy_base + tier * tier_spacing);

        if (info >= base && info <= base + 3U)
        {
            if ((tier == 1U && difficulty < WG_DIFFICULTY_MEDIUM)
                || (tier == 2U && difficulty < WG_DIFFICULTY_HARD))
            {
                return 0;
            }
            *direction = (uint8_t)(info - base);
            return 1;
        }
    }
    return 0;
}

static int WG_AmbushArea(const wg_map_t *map, size_t index,
                         uint8_t *area_number)
{
    size_t x = index % WG_LEVEL_SIZE;
    size_t y = index / WG_LEVEL_SIZE;
    uint16_t tile = 0U;

    if (x + 1U < WG_LEVEL_SIZE
        && map->planes[0][index + 1U] >= WG_AREA_TILE)
    {
        tile = map->planes[0][index + 1U];
    }
    if (y > 0U && map->planes[0][index - WG_LEVEL_SIZE] >= WG_AREA_TILE)
    {
        tile = map->planes[0][index - WG_LEVEL_SIZE];
    }
    if (y + 1U < WG_LEVEL_SIZE
        && map->planes[0][index + WG_LEVEL_SIZE] >= WG_AREA_TILE)
    {
        tile = map->planes[0][index + WG_LEVEL_SIZE];
    }
    if (x > 0U && map->planes[0][index - 1U] >= WG_AREA_TILE)
    {
        tile = map->planes[0][index - 1U];
    }
    if (tile < WG_AREA_TILE
        || tile >= WG_AREA_TILE + WG_NUM_AREAS)
    {
        return 0;
    }
    *area_number = (uint8_t)(tile - WG_AREA_TILE);
    return 1;
}

static int WG_StaticBlocks(unsigned type)
{
    switch (type)
    {
    case 1U:
    case 2U:
    case 3U:
    case 5U:
    case 7U:
    case 8U:
    case 10U:
    case 11U:
    case 12U:
    case 13U:
    case 16U:
    case 17U:
    case 18U:
    case 22U:
    case 35U:
    case 36U:
    case 37U:
    case 39U:
    case 40U:
    case 45U:
    case 46U:
        return 1;
    default:
        return 0;
    }
}

int WG_LevelBuildForDifficulty(const wg_map_t *map, wg_difficulty_t difficulty,
                               wg_level_t *level)
{
    size_t index;
    int player_found = 0;

    if (map == NULL || level == NULL || map->planes[0] == NULL
        || map->planes[1] == NULL || map->width != WG_LEVEL_SIZE
        || map->height != WG_LEVEL_SIZE
        || difficulty < WG_DIFFICULTY_BABY
        || difficulty > WG_DIFFICULTY_HARD)
    {
        return 0;
    }
    memset(level, 0, sizeof(*level));
    memset(level->areas, WG_NO_AREA, sizeof(level->areas));
    level->difficulty = difficulty;
    level->player_health = 100U;
    level->player_best_weapon = 1U;
    WG_RandomSeed(&level->random, 0U);
    for (index = 0; index < WG_LEVEL_SIZE * WG_LEVEL_SIZE; ++index)
    {
        uint16_t tile = map->planes[0][index];
        uint16_t info = map->planes[1][index];

        if (tile == WG_AMBUSH_TILE)
        {
            level->tiles[index] = WG_AMBUSH_TILE;
            level->ambush_tiles[index] = 1U;
            if (!WG_AmbushArea(map, index, &level->areas[index]))
            {
                return 0;
            }
        }
        else
        {
            level->tiles[index] = tile < WG_AREA_TILE ? (uint8_t)tile : 0U;
            if (tile >= WG_AREA_TILE
                && tile < WG_AREA_TILE + WG_NUM_AREAS)
            {
                level->areas[index] = (uint8_t)(tile - WG_AREA_TILE);
            }
        }
        level->info[index] = info;
        if (info >= 19U && info <= 22U)
        {
            size_t x = index % WG_LEVEL_SIZE;
            size_t y = index / WG_LEVEL_SIZE;
            int angle = (1 - (int)(info - 19U)) * 90;

            if (angle < 0)
            {
                angle += 360;
            }
            level->player_tile_x = (uint8_t)x;
            level->player_tile_y = (uint8_t)y;
            level->player_x = (int32_t)(x * 65536U + 32768U);
            level->player_y = (int32_t)(y * 65536U + 32768U);
            level->player_angle = (uint16_t)angle;
            player_found = 1;
        }
        else if (info >= 23U && info <= 71U)
        {
            wg_static_object_t *object;
            unsigned type = info - 23U;

            if (level->static_count >= WG_MAX_STATICS)
            {
                return 0;
            }
            object = &level->statics[level->static_count++];
            object->tile_x = (uint8_t)(index % WG_LEVEL_SIZE);
            object->tile_y = (uint8_t)(index / WG_LEVEL_SIZE);
            object->blocking = (uint8_t)WG_StaticBlocks(type);
            /*
             * SPR_DEMO and SPR_DEATHCAM precede SPR_STAT_0. The final
             * non-Spear statinfo entry is the duplicate ammo clip.
             */
            object->shape = info == 71U ? 28U : (uint16_t)(type + 2U);
        }
        else
        {
            uint8_t direction;

            if (WG_DifficultyDirection(info, 108U, 36U, difficulty,
                                       &direction))
            {
                if (!WL_SpawnStand(level, WG_ACTOR_GUARD,
                                   (uint8_t)(index % WG_LEVEL_SIZE),
                                   (uint8_t)(index / WG_LEVEL_SIZE), direction))
                {
                    return 0;
                }
            }
            else if (WG_DifficultyDirection(info, 112U, 36U, difficulty,
                                            &direction))
            {
                if (!WL_SpawnPatrol(level, WG_ACTOR_GUARD,
                                    (uint8_t)(index % WG_LEVEL_SIZE),
                                    (uint8_t)(index / WG_LEVEL_SIZE), direction))
                {
                    return 0;
                }
            }
            else if (info == 124U)
            {
                if (!WL_SpawnDeadGuard(
                    level, (uint8_t)(index % WG_LEVEL_SIZE),
                    (uint8_t)(index / WG_LEVEL_SIZE)))
                {
                    return 0;
                }
            }
            else if (WG_DifficultyDirection(info, 116U, 36U, difficulty,
                                            &direction))
            {
                if (!WL_SpawnStand(level, WG_ACTOR_OFFICER,
                                   (uint8_t)(index % WG_LEVEL_SIZE),
                                   (uint8_t)(index / WG_LEVEL_SIZE), direction))
                {
                    return 0;
                }
            }
            else if (WG_DifficultyDirection(info, 120U, 36U, difficulty,
                                            &direction))
            {
                if (!WL_SpawnPatrol(level, WG_ACTOR_OFFICER,
                                    (uint8_t)(index % WG_LEVEL_SIZE),
                                    (uint8_t)(index / WG_LEVEL_SIZE), direction))
                {
                    return 0;
                }
            }
            else if (WG_DifficultyDirection(info, 126U, 36U, difficulty,
                                            &direction))
            {
                if (!WL_SpawnStand(level, WG_ACTOR_SS,
                                   (uint8_t)(index % WG_LEVEL_SIZE),
                                   (uint8_t)(index / WG_LEVEL_SIZE), direction))
                {
                    return 0;
                }
            }
            else if (WG_DifficultyDirection(info, 130U, 36U, difficulty,
                                            &direction))
            {
                if (!WL_SpawnPatrol(level, WG_ACTOR_SS,
                                    (uint8_t)(index % WG_LEVEL_SIZE),
                                    (uint8_t)(index / WG_LEVEL_SIZE), direction))
                {
                    return 0;
                }
            }
            else if (WG_DifficultyDirection(info, 134U, 36U, difficulty,
                                            &direction))
            {
                if (!WL_SpawnStand(level, WG_ACTOR_DOG,
                                   (uint8_t)(index % WG_LEVEL_SIZE),
                                   (uint8_t)(index / WG_LEVEL_SIZE), direction))
                {
                    return 0;
                }
            }
            else if (WG_DifficultyDirection(info, 138U, 36U, difficulty,
                                            &direction))
            {
                if (!WL_SpawnPatrol(level, WG_ACTOR_DOG,
                                    (uint8_t)(index % WG_LEVEL_SIZE),
                                    (uint8_t)(index / WG_LEVEL_SIZE), direction))
                {
                    return 0;
                }
            }
            else if (info == 214U
                     && !WL_SpawnBoss(level, WG_ACTOR_BOSS,
                                     (uint8_t)(index % WG_LEVEL_SIZE),
                                     (uint8_t)(index / WG_LEVEL_SIZE)))
            {
                return 0;
            }
            else if (info == 197U
                     && !WL_SpawnBoss(level, WG_ACTOR_GRETEL,
                                     (uint8_t)(index % WG_LEVEL_SIZE),
                                     (uint8_t)(index / WG_LEVEL_SIZE)))
            {
                return 0;
            }
            else if (info == 215U
                     && !WL_SpawnBoss(level, WG_ACTOR_GIFT,
                                     (uint8_t)(index % WG_LEVEL_SIZE),
                                     (uint8_t)(index / WG_LEVEL_SIZE)))
            {
                return 0;
            }
            else if (info == 179U
                     && !WL_SpawnBoss(level, WG_ACTOR_FAT,
                                     (uint8_t)(index % WG_LEVEL_SIZE),
                                     (uint8_t)(index / WG_LEVEL_SIZE)))
            {
                return 0;
            }
            else if (info == 196U
                     && !WL_SpawnBoss(level, WG_ACTOR_SCHABBS,
                                     (uint8_t)(index % WG_LEVEL_SIZE),
                                     (uint8_t)(index / WG_LEVEL_SIZE)))
            {
                return 0;
            }
            else if (info == 160U
                     && !WL_SpawnBoss(level, WG_ACTOR_FAKE,
                                     (uint8_t)(index % WG_LEVEL_SIZE),
                                     (uint8_t)(index / WG_LEVEL_SIZE)))
            {
                return 0;
            }
            else if (info == 178U
                     && !WL_SpawnBoss(level, WG_ACTOR_MECHA_HITLER,
                                     (uint8_t)(index % WG_LEVEL_SIZE),
                                     (uint8_t)(index / WG_LEVEL_SIZE)))
            {
                return 0;
            }
            else if (WG_DifficultyDirection(info, 216U, 18U, difficulty,
                                            &direction))
            {
                if (!WL_SpawnStand(level, WG_ACTOR_MUTANT,
                                   (uint8_t)(index % WG_LEVEL_SIZE),
                                   (uint8_t)(index / WG_LEVEL_SIZE), direction))
                {
                    return 0;
                }
            }
            else if (WG_DifficultyDirection(info, 220U, 18U, difficulty,
                                            &direction))
            {
                if (!WL_SpawnPatrol(level, WG_ACTOR_MUTANT,
                                    (uint8_t)(index % WG_LEVEL_SIZE),
                                    (uint8_t)(index / WG_LEVEL_SIZE), direction))
                {
                    return 0;
                }
            }
            else if (info >= 224U && info <= 227U
                     && !WL_SpawnGhost(level,
                         (wg_ghost_kind_t)(info - 224U),
                         (uint8_t)(index % WG_LEVEL_SIZE),
                         (uint8_t)(index / WG_LEVEL_SIZE)))
            {
                return 0;
            }
        }
    }

    for (index = 0; index < WG_LEVEL_SIZE * WG_LEVEL_SIZE; ++index)
    {
        uint16_t tile = map->planes[0][index];

        if (tile >= 90U && tile <= 101U)
        {
            size_t x = index % WG_LEVEL_SIZE;
            size_t y = index / WG_LEVEL_SIZE;
            wg_door_t *door;

            if (level->door_count >= WG_MAX_DOORS)
            {
                return 0;
            }
            door = &level->doors[level->door_count];
            door->tile_x = (uint8_t)x;
            door->tile_y = (uint8_t)y;
            door->vertical = (uint8_t)((tile & 1U) == 0U);
            door->lock = (wg_door_lock_t)((tile - (door->vertical ? 90U : 91U))
                                          / 2U);
            level->tiles[index] = (uint8_t)(0x80U | level->door_count);
            if (door->vertical)
            {
                if (y == 0U || y + 1U >= WG_LEVEL_SIZE)
                {
                    return 0;
                }
                level->tiles[(y - 1U) * WG_LEVEL_SIZE + x] |= 0x40U;
                level->tiles[(y + 1U) * WG_LEVEL_SIZE + x] |= 0x40U;
            }
            else
            {
                if (x == 0U || x + 1U >= WG_LEVEL_SIZE)
                {
                    return 0;
                }
                level->tiles[y * WG_LEVEL_SIZE + x - 1U] |= 0x40U;
                level->tiles[y * WG_LEVEL_SIZE + x + 1U] |= 0x40U;
            }
            ++level->door_count;
        }
    }
    return player_found;
}

int WG_LevelBuild(const wg_map_t *map, wg_level_t *level)
{
    return WG_LevelBuildForDifficulty(map, WG_DIFFICULTY_MEDIUM, level);
}
