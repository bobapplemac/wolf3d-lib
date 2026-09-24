/* Portable SetupGameLevel, ScanInfoPlane, SpawnDoor, and SpawnStatic state. */
#include "WL_GAME.h"

#include <string.h>

int WG_LevelBuild(const wg_map_t *map, wg_level_t *level)
{
    size_t index;
    int player_found = 0;

    if (map == NULL || level == NULL || map->planes[0] == NULL
        || map->planes[1] == NULL || map->width != WG_LEVEL_SIZE
        || map->height != WG_LEVEL_SIZE)
    {
        return 0;
    }
    memset(level, 0, sizeof(*level));
    for (index = 0; index < WG_LEVEL_SIZE * WG_LEVEL_SIZE; ++index)
    {
        uint16_t tile = map->planes[0][index];
        uint16_t info = map->planes[1][index];

        level->tiles[index] = tile < WG_AREA_TILE ? (uint8_t)tile : 0;
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

            if (level->static_count >= WG_MAX_STATICS)
            {
                return 0;
            }
            object = &level->statics[level->static_count++];
            object->tile_x = (uint8_t)(index % WG_LEVEL_SIZE);
            object->tile_y = (uint8_t)(index / WG_LEVEL_SIZE);
            /*
             * SPR_DEMO and SPR_DEATHCAM precede SPR_STAT_0. The final
             * non-Spear statinfo entry is the duplicate ammo clip.
             */
            object->shape = info == 71U ? 28U
                                        : (uint16_t)(info - 23U + 2U);
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
