/* Portable SetupGameLevel, ScanInfoPlane, SpawnDoor, and SpawnStatic state. */
#include "WL_GAME.h"

#include <math.h>
#include <string.h>

#include "ID_VL.h"
#include "WG_FIXED.h"
#include "WL_AGENT.h"
#include "WL_MAIN.h"

#define WL_PI 3.14159265358979323846

int WL_DrawPlayBorder(uint8_t framebuffer[320 * 200],
                      unsigned view_width)
{
    int view_height;
    int x;
    int y;

    if (framebuffer == NULL || view_width < 64U || view_width > 320U
        || (view_width & 15U) != 0U)
    {
        return 0;
    }
    view_height = (int)view_width / 2;
    x = (320 - (int)view_width) / 2;
    y = (160 - view_height) / 2;
    if (view_width == 320U)
    {
        WG_VideoBar(framebuffer, 0, 0, 320, 160, 0U);
        return 1;
    }
    WG_VideoBar(framebuffer, 0, 0, 320, 160, 127U);
    WG_VideoBar(framebuffer, x, y, (int)view_width, view_height, 0U);
    WG_VideoBar(framebuffer, x - 1, y - 1, (int)view_width + 2, 1, 0U);
    WG_VideoBar(framebuffer, x - 1, y - 1, 1, view_height + 2, 0U);
    WG_VideoBar(framebuffer, x - 1, y + view_height,
                (int)view_width + 2, 1, 125U);
    WG_VideoBar(framebuffer, x + (int)view_width, y - 1,
                1, view_height + 2, 125U);
    WG_VideoPlot(framebuffer, x - 1, y + view_height, 124U);
    return 1;
}

int WL_DrawGetPsyched(uint8_t framebuffer[320 * 200],
                      const struct wg_graphics *graphics,
                      const wg_level_t *level)
{
    wl_status_t status;
    size_t picture;

    if (framebuffer == NULL || graphics == NULL || level == NULL)
    {
        return 0;
    }
    switch (graphics->variant)
    {
    case WG_GAME_WOLF3D_SHAREWARE_14:
        /* The released v1.4 shareware data uses GFXE_WL1.H.  Chunk 138 is
           FACE8APIC (the bloody BJ face); GETPSYCHEDPIC is chunk 141. */
        picture = 141U;
        break;
    case WG_GAME_WOLF3D_FULL_GT_14:
        picture = 134U;
        break;
    case WG_GAME_SPEAR_DEMO_SDM:
        picture = 127U;
        break;
    case WG_GAME_SPEAR_FULL_SOD:
    case WG_GAME_SPEAR_MISSION_1_SD1:
    case WG_GAME_SPEAR_MISSION_2_SD2:
    case WG_GAME_SPEAR_MISSION_3_SD3:
        picture = 149U;
        break;
    default:
        return 0;
    }

    WL_StatusDefaults(&status);
    status.score = level->score;
    status.health = level->player_health;
    status.ammo = level->player_ammo;
    status.weapon = level->player_weapon;
    status.lives = level->player_lives;
    status.keys = level->player_keys;
    status.map = level->map_number;
    if (!WL_DrawStatusBar(framebuffer, graphics, &status))
    {
        return 0;
    }
    WG_VideoBar(framebuffer, 0, 0, 320, 160, 127U);
    if (!WG_VideoDrawPicture(framebuffer, graphics, picture, 48, 56))
    {
        return 0;
    }
    /* PM_Preload leaves its progress bar full before the original one-second
       IN_UserInput hold.  Resource decoding is synchronous here, so draw that
       final state directly. */
    WG_VideoBar(framebuffer, 53, 101, 214, 2, 0x37U);
    WG_VideoBar(framebuffer, 53, 101, 213, 1, 0x32U);
    return 1;
}

uint16_t WL_DeathTargetAngle(const wg_level_t *level)
{
    double angle;

    if (level == NULL)
    {
        return 0U;
    }
    angle = atan2((double)level->player_y - (double)level->killer_y,
                  (double)level->killer_x - (double)level->player_x);
    if (angle < 0.0)
    {
        angle += WL_PI * 2.0;
    }
    return (uint16_t)(angle * 360.0 / (WL_PI * 2.0));
}

int WL_DeathRotateStep(wg_level_t *level, uint16_t target_angle,
                       unsigned degrees)
{
    unsigned current;
    unsigned clockwise;
    unsigned counterclockwise;

    if (level == NULL || target_angle >= 360U || degrees == 0U)
    {
        return 0;
    }
    current = level->player_angle;
    if (current == target_angle)
    {
        return 1;
    }
    clockwise = (target_angle + 360U - current) % 360U;
    counterclockwise = (current + 360U - target_angle) % 360U;
    if (clockwise < counterclockwise)
    {
        if (degrees >= clockwise)
        {
            current = target_angle;
        }
        else
        {
            current = (current + degrees) % 360U;
        }
    }
    else if (degrees >= counterclockwise)
    {
        current = target_angle;
    }
    else
    {
        current = (current + 360U - degrees) % 360U;
    }
    level->player_angle = (uint16_t)current;
    level->player_angle_fraction = (int32_t)current << 16;
    return current == target_angle;
}

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

static void WG_AmbushArea(const wg_map_t *map, size_t index,
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
    if (tile >= WG_AREA_TILE
        && tile < WG_AREA_TILE + WG_NUM_AREAS)
    {
        *area_number = (uint8_t)(tile - WG_AREA_TILE);
    }
    else
    {
        /* The original accepted isolated ambush markers and left their area
           byte at -1. Several mission-pack maps rely on that tolerance. */
        *area_number = WG_NO_AREA;
    }
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

static wg_item_type_t WG_StaticItem(unsigned type)
{
    switch (type)
    {
    case 6U: return WG_ITEM_ALPO;
    case 20U: return WG_ITEM_KEY1;
    case 21U: return WG_ITEM_KEY2;
    case 24U: return WG_ITEM_FOOD;
    case 25U: return WG_ITEM_FIRSTAID;
    case 26U: return WG_ITEM_CLIP;
    case 27U: return WG_ITEM_MACHINEGUN;
    case 28U: return WG_ITEM_CHAINGUN;
    case 29U: return WG_ITEM_CROSS;
    case 30U: return WG_ITEM_CHALICE;
    case 31U: return WG_ITEM_BIBLE;
    case 32U: return WG_ITEM_CROWN;
    case 33U: return WG_ITEM_FULLHEAL;
    case 34U:
    case 38U:
        return WG_ITEM_GIBS;
    case 49U: return WG_ITEM_AMMO25;
    case 51U: return WG_ITEM_SPEAR;
    default:
        return WG_ITEM_NONE;
    }
}

int WG_LevelBuildForVariant(const wg_map_t *map, wg_difficulty_t difficulty,
                            wg_game_variant_t variant, wg_level_t *level)
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
    level->variant = variant;
    level->difficulty = difficulty;
    level->player_health = 100U;
    level->player_ammo = 8U;
    level->player_lives = 3U;
    level->next_extra = 40000U;
    level->player_weapon = 1U;
    level->player_chosen_weapon = 1U;
    level->player_best_weapon = 1U;
    WG_RandomSeed(&level->random, 0U);
    /* The DOS actorat array begins as a copy of every solid map tile.  Actor
       pointers later overwrite these small integer tokens. */
    for (index = 0U; index < WG_LEVEL_SIZE * WG_LEVEL_SIZE; ++index)
    {
        uint16_t tile = map->planes[0][index];

        if (tile < WG_AREA_TILE)
        {
            level->actor_at[index] = tile;
        }
    }
    for (index = 0; index < WG_LEVEL_SIZE * WG_LEVEL_SIZE; ++index)
    {
        uint16_t tile = map->planes[0][index];
        uint16_t info = map->planes[1][index];

        if (tile == WG_AMBUSH_TILE)
        {
            level->tiles[index] = 0U;
            level->ambush_tiles[index] = 1U;
            WG_AmbushArea(map, index, &level->areas[index]);
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
        else if (info >= 23U
                 && info <= (WG_DataVariantFamily(variant)
                                  == WG_GAME_FAMILY_SPEAR ? 74U : 71U))
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
            if (WG_DataVariantFamily(variant) == WG_GAME_FAMILY_SPEAR)
            {
                if (type == 15U || type == 44U || type == 48U
                    || type == 50U)
                {
                    object->blocking = 1U;
                }
                else if (type == 40U)
                {
                    object->blocking = 0U;
                }
            }
            object->removed = 0U;
            /*
             * SPR_DEMO and SPR_DEATHCAM precede SPR_STAT_0. The final
             * non-Spear statinfo entry is the duplicate ammo clip.
             */
            object->shape = info == 71U
                                && WG_DataVariantFamily(variant)
                                   != WG_GAME_FAMILY_SPEAR
                                ? 28U : (uint16_t)(type + 2U);
            object->item = info == 71U
                               && WG_DataVariantFamily(variant)
                                  != WG_GAME_FAMILY_SPEAR
                               ? WG_ITEM_CLIP2 : WG_StaticItem(type);
            if (object->item == WG_ITEM_CROSS
                || object->item == WG_ITEM_CHALICE
                || object->item == WG_ITEM_BIBLE
                || object->item == WG_ITEM_CROWN
                || object->item == WG_ITEM_FULLHEAL)
            {
                ++level->treasure_total;
            }
        }
        else
        {
            uint8_t direction;
            int spear = WG_DataVariantFamily(variant)
                        == WG_GAME_FAMILY_SPEAR;

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
            else if (!spear && info == 214U
                     && !WL_SpawnBoss(level, WG_ACTOR_BOSS,
                                     (uint8_t)(index % WG_LEVEL_SIZE),
                                     (uint8_t)(index / WG_LEVEL_SIZE)))
            {
                return 0;
            }
            else if (!spear && info == 197U
                     && !WL_SpawnBoss(level, WG_ACTOR_GRETEL,
                                     (uint8_t)(index % WG_LEVEL_SIZE),
                                     (uint8_t)(index / WG_LEVEL_SIZE)))
            {
                return 0;
            }
            else if (!spear && info == 215U
                     && !WL_SpawnBoss(level, WG_ACTOR_GIFT,
                                     (uint8_t)(index % WG_LEVEL_SIZE),
                                     (uint8_t)(index / WG_LEVEL_SIZE)))
            {
                return 0;
            }
            else if (!spear && info == 179U
                     && !WL_SpawnBoss(level, WG_ACTOR_FAT,
                                     (uint8_t)(index % WG_LEVEL_SIZE),
                                     (uint8_t)(index / WG_LEVEL_SIZE)))
            {
                return 0;
            }
            else if (!spear && info == 196U
                     && !WL_SpawnBoss(level, WG_ACTOR_SCHABBS,
                                     (uint8_t)(index % WG_LEVEL_SIZE),
                                     (uint8_t)(index / WG_LEVEL_SIZE)))
            {
                return 0;
            }
            else if (!spear && info == 160U
                     && !WL_SpawnBoss(level, WG_ACTOR_FAKE,
                                     (uint8_t)(index % WG_LEVEL_SIZE),
                                     (uint8_t)(index / WG_LEVEL_SIZE)))
            {
                return 0;
            }
            else if (!spear && info == 178U
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
            else if (!spear && info >= 224U && info <= 227U
                     && !WL_SpawnGhost(level,
                         (wg_ghost_kind_t)(info - 224U),
                         (uint8_t)(index % WG_LEVEL_SIZE),
                         (uint8_t)(index / WG_LEVEL_SIZE)))
            {
                return 0;
            }
            else if (spear && info == 106U
                     && !WL_SpawnBoss(level, WG_ACTOR_SPECTRE,
                                     (uint8_t)(index % WG_LEVEL_SIZE),
                                     (uint8_t)(index / WG_LEVEL_SIZE)))
            {
                return 0;
            }
            else if (spear && info == 107U
                     && !WL_SpawnBoss(level, WG_ACTOR_ANGEL,
                                     (uint8_t)(index % WG_LEVEL_SIZE),
                                     (uint8_t)(index / WG_LEVEL_SIZE)))
            {
                return 0;
            }
            else if (spear && info == 125U
                     && !WL_SpawnBoss(level, WG_ACTOR_TRANS,
                                     (uint8_t)(index % WG_LEVEL_SIZE),
                                     (uint8_t)(index / WG_LEVEL_SIZE)))
            {
                return 0;
            }
            else if (spear && info == 142U
                     && !WL_SpawnBoss(level, WG_ACTOR_UBER,
                                     (uint8_t)(index % WG_LEVEL_SIZE),
                                     (uint8_t)(index / WG_LEVEL_SIZE)))
            {
                return 0;
            }
            else if (spear && info == 143U
                     && !WL_SpawnBoss(level, WG_ACTOR_WILL,
                                     (uint8_t)(index % WG_LEVEL_SIZE),
                                     (uint8_t)(index / WG_LEVEL_SIZE)))
            {
                return 0;
            }
            else if (spear && info == 161U
                     && !WL_SpawnBoss(level, WG_ACTOR_DEATH,
                                     (uint8_t)(index % WG_LEVEL_SIZE),
                                     (uint8_t)(index / WG_LEVEL_SIZE)))
            {
                return 0;
            }
        }
        if (info == 98U)
        {
            ++level->secret_total;
        }
    }

    for (index = 0U; index < level->actor_count; ++index)
    {
        if ((level->actors[index].flags & WG_ACTOR_FLAG_SHOOTABLE) != 0U)
        {
            ++level->kill_total;
        }
    }

    for (index = 0U; index < WG_LEVEL_SIZE * WG_LEVEL_SIZE; ++index)
    {
        if (map->planes[0][index] == WG_AMBUSH_TILE
            && level->actor_at[index] == WG_AMBUSH_TILE)
        {
            level->actor_at[index] = 0U;
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
            door->action = WG_DOOR_CLOSED;
            level->tiles[index] = (uint8_t)(0x80U | level->door_count);
            level->actor_at[index] = (uint16_t)(0x80U | level->door_count);
            if (door->vertical)
            {
                if (x == 0U || x + 1U >= WG_LEVEL_SIZE
                    || y == 0U || y + 1U >= WG_LEVEL_SIZE)
                {
                    return 0;
                }
                level->areas[index] = level->areas[y * WG_LEVEL_SIZE + x - 1U];
                level->tiles[(y - 1U) * WG_LEVEL_SIZE + x] |= 0x40U;
                level->tiles[(y + 1U) * WG_LEVEL_SIZE + x] |= 0x40U;
            }
            else
            {
                if (x == 0U || x + 1U >= WG_LEVEL_SIZE
                    || y == 0U || y + 1U >= WG_LEVEL_SIZE)
                {
                    return 0;
                }
                level->areas[index] = level->areas[(y - 1U) * WG_LEVEL_SIZE + x];
                level->tiles[y * WG_LEVEL_SIZE + x - 1U] |= 0x40U;
                level->tiles[y * WG_LEVEL_SIZE + x + 1U] |= 0x40U;
            }
            ++level->door_count;
        }
    }
    if (player_found)
    {
        uint8_t player_area = level->areas[
            (size_t)level->player_tile_y * WG_LEVEL_SIZE
            + level->player_tile_x];

        /* SpawnPlayer ended by calling InitAreas: with every door initially
           closed, only the player's starting area is connected. */
        if (player_area >= WG_NUM_AREAS)
        {
            return 0;
        }
        level->area_by_player[player_area] = 1U;
    }
    return player_found;
}

int WG_LevelBuild(const wg_map_t *map, wg_level_t *level)
{
    return WG_LevelBuildForVariant(map, WG_DIFFICULTY_MEDIUM,
                                   WG_GAME_WOLF3D_FULL_GT_14, level);
}

int WG_LevelBuildForDifficulty(const wg_map_t *map, wg_difficulty_t difficulty,
                               wg_level_t *level)
{
    return WG_LevelBuildForVariant(map, difficulty,
                                   WG_GAME_WOLF3D_FULL_GT_14, level);
}

int WG_QueueSound(wg_level_t *level, wg_sound_t sound)
{
    wg_sound_event_t *event;

    if (level == NULL || level->sound_event_count >= WG_MAX_SOUND_EVENTS
        || sound < 0 || sound > UINT8_MAX)
    {
        return 0;
    }
    event = &level->sound_events[level->sound_event_count++];
    memset(event, 0, sizeof(*event));
    event->sound = (uint8_t)sound;
    return 1;
}

int WG_SoundNumberForVariant(wg_game_variant_t variant,
                             wg_sound_t sound, unsigned *sound_number)
{
    unsigned result;

    if (sound_number == NULL || sound < 0)
    {
        return 0;
    }
    if (WG_DataVariantFamily(variant) != WG_GAME_FAMILY_SPEAR)
    {
        if ((unsigned)sound >= 87U)
        {
            return 0;
        }
        *sound_number = (unsigned)sound;
        return 1;
    }
    if ((unsigned)sound <= 49U
        || sound == WG_SOUND_SS_SIGHT || sound == WG_SOUND_AHHHG
        || sound == WG_SOUND_LEBEN || sound == WG_SOUND_NAZI_FIRE
        || sound == WG_SOUND_BOSS_FIRE || sound == WG_SOUND_SS_FIRE
        || sound == WG_SOUND_SLURPIE || sound == WG_SOUND_OFFICER_SIGHT
        || sound == WG_SOUND_NEIN_SOWAS || sound == WG_SOUND_DOG_ATTACK)
    {
        result = (unsigned)sound;
    }
    else
    {
        switch (sound)
        {
        case WG_SOUND_DEATH_SCREAM_4: result = 50U; break;
        case WG_SOUND_DEATH_SCREAM_5: result = 53U; break;
        case WG_SOUND_DEATH_SCREAM_6: result = 57U; break;
        case WG_SOUND_DEATH_SCREAM_7: result = 54U; break;
        case WG_SOUND_DEATH_SCREAM_8: result = 55U; break;
        case WG_SOUND_DEATH_SCREAM_9: result = 63U; break;
        case WG_SOUND_MISSILE_FIRE: result = 8U; break;
        case WG_SOUND_MISSILE_HIT: result = 1U; break;
        case WG_SOUND_GHOST_SIGHT: result = 3U; break;
        case WG_SOUND_GHOST_FADE: result = 62U; break;
        case WG_SOUND_ANGEL_SIGHT: result = 65U; break;
        case WG_SOUND_ANGEL_FIRE: result = 69U; break;
        case WG_SOUND_ANGEL_DEATH: result = 77U; break;
        case WG_SOUND_ANGEL_TIRED: result = 80U; break;
        case WG_SOUND_TRANS_SIGHT: result = 70U; break;
        case WG_SOUND_TRANS_DEATH: result = 71U; break;
        case WG_SOUND_WILHELM_SIGHT: result = 72U; break;
        case WG_SOUND_WILHELM_DEATH: result = 73U; break;
        case WG_SOUND_UBER_DEATH: result = 74U; break;
        case WG_SOUND_KNIGHT_SIGHT: result = 75U; break;
        case WG_SOUND_KNIGHT_DEATH: result = 76U; break;
        case WG_SOUND_KNIGHT_MISSILE: result = 78U; break;
        case WG_SOUND_GET_SPEAR: result = 79U; break;
        case WG_SOUND_GET_AMMO_BOX: result = 64U; break;
        default: return 0;
        }
    }
    *sound_number = result;
    return 1;
}

unsigned WG_NextMapNumber(unsigned map_number, int secret_level)
{
    static const uint8_t elevator_back_to[6] = {1U, 1U, 7U, 3U, 5U, 3U};
    unsigned episode = map_number / 10U;
    unsigned floor = map_number % 10U;

    if (floor == 9U)
    {
        return episode * 10U
               + elevator_back_to[episode < 6U ? episode : 0U];
    }
    if (secret_level)
    {
        return episode * 10U + 9U;
    }
    return map_number + 1U;
}

unsigned WG_NextMapNumberForVariant(wg_game_variant_t variant,
                                    unsigned map_number, int secret_level)
{
    if (WG_DataVariantFamily(variant) != WG_GAME_FAMILY_SPEAR)
    {
        return WG_NextMapNumber(map_number, secret_level);
    }
    if (secret_level)
    {
        if (map_number == 3U)
        {
            return 18U;
        }
        if (map_number == 11U)
        {
            return 19U;
        }
    }
    if (map_number == 18U)
    {
        return 4U;
    }
    if (map_number == 19U)
    {
        return 12U;
    }
    return map_number + 1U;
}

int WG_CampaignEndsAfterIntermission(wg_game_variant_t variant,
                                     unsigned map_number)
{
    return variant == WG_GAME_SPEAR_DEMO_SDM && map_number == 1U;
}

void WG_CampaignCapture(wg_campaign_state_t *state,
                        const wg_level_t *level)
{
    if (state == NULL || level == NULL)
    {
        return;
    }
    state->score = level->score;
    state->next_extra = level->next_extra;
    state->health = level->player_health;
    state->ammo = level->player_ammo;
    state->lives = level->player_lives;
    state->weapon = level->player_weapon;
    state->chosen_weapon = level->player_chosen_weapon;
    state->best_weapon = level->player_best_weapon;
}

int WG_CampaignApply(wg_level_t *level,
                     const wg_campaign_state_t *state,
                     uint32_t level_start_score, int died)
{
    if (level == NULL || state == NULL || (died && state->lives == 0U))
    {
        return 0;
    }
    level->score = died ? level_start_score : state->score;
    level->next_extra = state->next_extra;
    level->player_lives = (uint8_t)(state->lives - (died ? 1U : 0U));
    level->player_health = died ? 100U : state->health;
    level->player_ammo = died ? 8U : state->ammo;
    level->player_weapon = died ? WG_WEAPON_PISTOL : state->weapon;
    level->player_chosen_weapon = died ? WG_WEAPON_PISTOL
                                       : state->chosen_weapon;
    level->player_best_weapon = died ? WG_WEAPON_PISTOL
                                     : state->best_weapon;
    level->player_keys = 0U;
    return 1;
}

int WG_QueueSoundAt(wg_level_t *level, wg_sound_t sound,
                    int32_t x, int32_t y)
{
    wg_sound_event_t *event;

    if (!WG_QueueSound(level, sound))
    {
        return 0;
    }
    event = &level->sound_events[level->sound_event_count - 1U];
    event->x = x;
    event->y = y;
    event->positioned = 1U;
    return 1;
}

static int WG_SoundTile(int32_t value)
{
    if (value >= 0)
    {
        return (int)(value / WG_FIXED_ONE);
    }
    return -(int)((-(int64_t)value + WG_FIXED_ONE - 1) / WG_FIXED_ONE);
}

int WG_SoundPosition(const wg_level_t *level,
                     const wg_view_tables_t *tables,
                     int32_t sound_x, int32_t sound_y,
                     uint8_t *left, uint8_t *right)
{
    static const uint8_t left_table[15][30] =
    {
        {8,8,8,8,8,8,8,8,5,3,1,0,0,0,0,0,6,7,7,7,7,7,7,8,8,8,8,8,8,8},
        {8,8,8,8,8,8,8,8,6,4,2,0,0,0,0,0,4,6,7,7,7,7,7,8,8,8,8,8,8,8},
        {8,8,8,8,8,8,8,8,6,4,2,1,0,0,0,1,4,6,6,7,7,7,7,8,8,8,8,8,8,8},
        {8,8,8,8,8,8,8,8,7,5,3,2,1,0,1,2,4,5,6,7,7,7,7,8,8,8,8,8,8,8},
        {8,8,8,8,8,8,8,8,8,6,5,3,3,2,2,3,4,5,6,7,7,7,8,8,8,8,8,8,8,8},
        {8,8,8,8,8,8,8,8,8,7,6,5,4,4,4,4,5,6,6,7,7,7,8,8,8,8,8,8,8,8},
        {8,8,8,8,8,8,8,8,8,8,7,6,6,5,5,5,6,6,7,7,7,8,8,8,8,8,8,8,8,8},
        {8,8,8,8,8,8,8,8,8,8,8,8,7,7,6,6,7,7,7,8,8,8,8,8,8,8,8,8,8,8},
        {8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8},
        {8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8},
        {8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8},
        {8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8},
        {8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8},
        {8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8},
        {8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8}
    };
    const int32_t *cosine;
    int32_t view_cosine;
    int32_t view_sine;
    int32_t view_x;
    int32_t view_y;
    int32_t relative_x;
    int32_t relative_y;
    int distance_x;
    int distance_y;

    if (level == NULL || tables == NULL || left == NULL || right == NULL
        || level->player_angle >= WG_ANGLES)
    {
        return 0;
    }
    cosine = WG_ViewCosineTable(tables);
    view_cosine = cosine[level->player_angle];
    view_sine = tables->sine[level->player_angle];
    view_x = level->player_x
             - WG_FixedByFrac(tables->focal_length, view_cosine);
    view_y = level->player_y
             + WG_FixedByFrac(tables->focal_length, view_sine);
    sound_x -= view_x;
    sound_y -= view_y;
    relative_x = WG_FixedByFrac(sound_x, view_cosine)
                 - WG_FixedByFrac(sound_y, view_sine);
    relative_y = WG_FixedByFrac(sound_x, view_sine)
                 + WG_FixedByFrac(sound_y, view_cosine);
    distance_x = WG_SoundTile(relative_x);
    distance_y = WG_SoundTile(relative_y);
    if (distance_y >= 15)
    {
        distance_y = 14;
    }
    else if (distance_y <= -15)
    {
        distance_y = -15;
    }
    if (distance_x < 0)
    {
        distance_x = -distance_x;
    }
    if (distance_x >= 15)
    {
        distance_x = 14;
    }
    *left = left_table[distance_x][distance_y + 15];
    *right = left_table[distance_x][14 - distance_y];
    return 1;
}

void WG_ClearSoundEvents(wg_level_t *level)
{
    if (level != NULL)
    {
        level->sound_event_count = 0U;
    }
}
