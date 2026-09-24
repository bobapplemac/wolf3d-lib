#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "WG_AUDIO.h"
#include "WG_ASSETS.h"
#include "ID_CA.h"
#include "WG_DATA.h"
#include "WG_FIXED.h"
#include "ID_VH.h"
#include "WG_GRAPHICS.h"
#include "WL_GAME.h"
#include "WL_AGENT.h"
#include "WG_MAPS.h"
#include "ID_PM.h"
#include "WG_PALETTE.h"
#include "ID_US_1.h"
#include "WL_DRAW.h"
#include "WG_RENDERER.h"
#include "WL_SCALE.h"
#include "WL_STATE.h"
#include "ID_VL.h"
#include "WL_MAIN.h"

static int failures;

#define CHECK(expression)                                                       \
    do                                                                          \
    {                                                                           \
        if (!(expression))                                                      \
        {                                                                       \
            fprintf(stderr, "%s:%d: check failed: %s\n",                      \
                    __FILE__, __LINE__, #expression);                           \
            ++failures;                                                         \
        }                                                                       \
    } while (0)

static size_t ActorClassCount(const wg_level_t *level,
                              wg_actor_class_t actor_class)
{
    size_t count = 0;
    size_t index;

    for (index = 0; index < level->actor_count; ++index)
    {
        if (level->actors[index].actor_class == actor_class)
        {
            ++count;
        }
    }
    return count;
}

static const wg_actor_t *FindLastLiveGuard(const wg_level_t *level)
{
    size_t index;

    for (index = level->actor_count; index > 0U; --index)
    {
        const wg_actor_t *actor = &level->actors[index - 1U];

        if (actor->actor_class == WG_ACTOR_GUARD && actor->rotate != 0U)
        {
            return actor;
        }
    }
    return NULL;
}

static void TestActorSetup(void)
{
    static const uint16_t easy_codes[] =
    {
        108U, 112U, 116U, 120U, 124U, 126U, 130U, 134U, 138U,
        216U, 220U
    };
    static const uint16_t medium_codes[] =
    {
        144U, 148U, 152U, 156U, 162U, 166U, 170U, 174U, 234U,
        238U
    };
    static const uint16_t hard_codes[] =
    {
        180U, 184U, 188U, 192U, 198U, 202U, 206U, 210U, 252U,
        256U
    };
    uint16_t plane_zero[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    uint16_t plane_one[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    wg_map_t map;
    wg_level_t level;
    size_t index;

    memset(&map, 0, sizeof(map));
    for (index = 0; index < WG_LEVEL_SIZE * WG_LEVEL_SIZE; ++index)
    {
        plane_zero[index] = WG_AREA_TILE;
        plane_one[index] = 0U;
    }
    plane_one[WG_LEVEL_SIZE + 1U] = 19U;
    for (index = 0; index < sizeof(easy_codes) / sizeof(easy_codes[0]); ++index)
    {
        plane_one[2U * WG_LEVEL_SIZE + 2U + index] = easy_codes[index];
    }
    for (index = 0;
         index < sizeof(medium_codes) / sizeof(medium_codes[0]); ++index)
    {
        plane_one[3U * WG_LEVEL_SIZE + 2U + index] = medium_codes[index];
    }
    for (index = 0; index < sizeof(hard_codes) / sizeof(hard_codes[0]); ++index)
    {
        plane_one[4U * WG_LEVEL_SIZE + 2U + index] = hard_codes[index];
    }
    map.width = WG_LEVEL_SIZE;
    map.height = WG_LEVEL_SIZE;
    map.planes[0] = plane_zero;
    map.planes[1] = plane_one;

    CHECK(WG_LevelBuildForDifficulty(&map, WG_DIFFICULTY_BABY, &level));
    CHECK(level.actor_count == 11U);
    CHECK(ActorClassCount(&level, WG_ACTOR_GUARD) == 2U);
    CHECK(ActorClassCount(&level, WG_ACTOR_OFFICER) == 2U);
    CHECK(ActorClassCount(&level, WG_ACTOR_SS) == 2U);
    CHECK(ActorClassCount(&level, WG_ACTOR_DOG) == 2U);
    CHECK(ActorClassCount(&level, WG_ACTOR_MUTANT) == 2U);
    CHECK(ActorClassCount(&level, WG_ACTOR_INERT) == 1U);
    CHECK(level.actors[0].shape == 50U);
    CHECK(level.actors[1].shape == 58U);
    CHECK(level.actors[2].shape == 238U);
    CHECK(level.actors[3].shape == 246U);
    CHECK(level.actors[4].shape == 95U);
    CHECK(level.actors[5].shape == 138U);
    CHECK(level.actors[6].shape == 146U);
    CHECK(level.actors[7].shape == 99U);
    CHECK(level.actors[8].shape == 99U);
    CHECK(level.actors[9].shape == 187U);
    CHECK(level.actors[10].shape == 195U);

    CHECK(WG_LevelBuildForDifficulty(&map, WG_DIFFICULTY_MEDIUM, &level));
    CHECK(level.actor_count == 21U);
    CHECK(WG_LevelBuildForDifficulty(&map, WG_DIFFICULTY_HARD, &level));
    CHECK(level.actor_count == 31U);
}

static void TestBossAndGhostSetup(void)
{
    static const uint16_t actor_codes[] =
    {
        214U, 197U, 215U, 179U, 196U, 160U, 178U,
        224U, 225U, 226U, 227U
    };
    static const wg_actor_class_t expected_classes[] =
    {
        WG_ACTOR_BOSS, WG_ACTOR_GRETEL, WG_ACTOR_GIFT, WG_ACTOR_FAT,
        WG_ACTOR_SCHABBS, WG_ACTOR_FAKE, WG_ACTOR_MECHA_HITLER,
        WG_ACTOR_GHOST, WG_ACTOR_GHOST, WG_ACTOR_GHOST, WG_ACTOR_GHOST
    };
    static const uint16_t expected_shapes[] =
    {
        296U, 385U, 360U, 396U, 307U, 321U, 334U,
        288U, 292U, 290U, 294U
    };
    uint16_t plane_zero[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    uint16_t plane_one[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    wg_map_t map;
    wg_level_t level;
    size_t index;

    memset(&map, 0, sizeof(map));
    for (index = 0; index < WG_LEVEL_SIZE * WG_LEVEL_SIZE; ++index)
    {
        plane_zero[index] = WG_AREA_TILE;
        plane_one[index] = 0U;
    }
    plane_one[WG_LEVEL_SIZE + 1U] = 19U;
    for (index = 0; index < sizeof(actor_codes) / sizeof(actor_codes[0]); ++index)
    {
        plane_one[2U * WG_LEVEL_SIZE + 2U + index] = actor_codes[index];
    }
    map.width = WG_LEVEL_SIZE;
    map.height = WG_LEVEL_SIZE;
    map.planes[0] = plane_zero;
    map.planes[1] = plane_one;

    CHECK(WG_LevelBuildForDifficulty(&map, WG_DIFFICULTY_BABY, &level));
    CHECK(level.actor_count
          == sizeof(actor_codes) / sizeof(actor_codes[0]));
    for (index = 0; index < level.actor_count; ++index)
    {
        CHECK(level.actors[index].actor_class == expected_classes[index]);
        CHECK(level.actors[index].shape == expected_shapes[index]);
        CHECK(level.actors[index].rotate == 0U);
    }
    CHECK(level.actors[0].direction == 6U);
    CHECK(level.actors[1].direction == 2U);
    CHECK(level.actors[7].direction == 0U);
    CHECK(level.actors[0].attack_shape == 300U);
    CHECK(level.actors[1].attack_shape == 389U);
    CHECK(level.actors[2].attack_shape == 364U);
    CHECK(level.actors[3].attack_shape == 400U);
    CHECK(level.actors[4].attack_shape == 311U);
    CHECK(level.actors[5].attack_shape == 0U);
    CHECK(level.actors[6].attack_shape == 338U);
}

static void TestPatrolMovement(void)
{
    uint16_t plane_zero[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    uint16_t plane_one[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    wg_map_t map;
    wg_level_t level;
    wg_actor_t *actor;
    size_t index;

    memset(&map, 0, sizeof(map));
    for (index = 0; index < WG_LEVEL_SIZE * WG_LEVEL_SIZE; ++index)
    {
        plane_zero[index] = WG_AREA_TILE;
        plane_one[index] = 0U;
    }
    plane_one[WG_LEVEL_SIZE + 1U] = 19U;
    plane_one[2U * WG_LEVEL_SIZE + 2U] = 112U;
    plane_one[2U * WG_LEVEL_SIZE + 3U] = 96U;
    map.width = WG_LEVEL_SIZE;
    map.height = WG_LEVEL_SIZE;
    map.planes[0] = plane_zero;
    map.planes[1] = plane_one;

    CHECK(WG_LevelBuild(&map, &level));
    CHECK(level.actor_count == 1U);
    actor = &level.actors[0];
    CHECK(actor->state == WG_STATE_PATH1);
    CHECK(actor->tic_count == 8);
    CHECK(actor->speed == 512);
    CHECK(actor->distance == WG_FIXED_ONE);
    CHECK(actor->tile_x == 3U);
    CHECK(actor->tile_y == 2U);
    CHECK(WL_TickActors(&level, 128U));
    CHECK(actor->state == WG_STATE_PATH3S);
    CHECK(actor->tic_count == 5);
    CHECK(actor->shape == 74U);
    CHECK(actor->x == 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2);
    CHECK(actor->y == 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2);
    CHECK(actor->direction == 0U);
    CHECK(actor->tile_x == 3U);
    CHECK(actor->tile_y == 2U);
    CHECK(actor->distance == WG_FIXED_ONE);
    CHECK(WL_TickActors(&level, 1U));
    CHECK(actor->tic_count == 4);
    CHECK(actor->x == 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2);
    CHECK(actor->distance == WG_FIXED_ONE);
    CHECK(WL_TickActors(&level, 4U));
    CHECK(actor->state == WG_STATE_PATH4);
    CHECK(actor->tic_count == 15);
    CHECK(actor->shape == 82U);
    CHECK(actor->x == 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2 + 2048);
    CHECK(actor->y == 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2);
    CHECK(actor->distance == WG_FIXED_ONE - 2048);
}

static void TestActorAwareness(void)
{
    uint16_t plane_zero[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    uint16_t plane_one[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    wg_map_t map;
    wg_level_t level;
    wg_actor_t *actor;
    size_t index;

    memset(&map, 0, sizeof(map));
    for (index = 0U; index < WG_LEVEL_SIZE * WG_LEVEL_SIZE; ++index)
    {
        plane_zero[index] = WG_AREA_TILE;
        plane_one[index] = 0U;
    }
    plane_zero[2U * WG_LEVEL_SIZE + 2U] = WG_AMBUSH_TILE;
    plane_one[2U * WG_LEVEL_SIZE + 2U] = 108U;
    plane_one[2U * WG_LEVEL_SIZE + 5U] = 19U;
    map.width = WG_LEVEL_SIZE;
    map.height = WG_LEVEL_SIZE;
    map.planes[0] = plane_zero;
    map.planes[1] = plane_one;

    CHECK(WG_LevelBuild(&map, &level));
    CHECK(level.actor_count == 1U);
    actor = &level.actors[0];
    CHECK(level.tiles[2U * WG_LEVEL_SIZE + 2U] == 0U);
    CHECK(level.areas[2U * WG_LEVEL_SIZE + 2U] == 0U);
    CHECK(actor->area_number == 0U);
    CHECK((actor->flags & WG_ACTOR_FLAG_AMBUSH) != 0U);
    CHECK(WL_UpdateAreaConnectivity(&level));
    CHECK(level.area_by_player[0] != 0U);
    CHECK(WL_CheckLine(&level, actor));
    CHECK(WL_CheckSight(&level, actor));
    CHECK(WL_TickAwareness(&level, 1U, 0));
    CHECK((actor->flags & WG_ACTOR_FLAG_AMBUSH) == 0U);
    CHECK(actor->reaction_time == 3);
    CHECK(actor->state == WG_STATE_STAND);
    CHECK(WL_TickAwareness(&level, 2U, 0));
    CHECK(actor->reaction_time == 1);
    CHECK(WL_TickAwareness(&level, 1U, 0));
    CHECK(actor->state == WG_STATE_CHASE1);
    CHECK(actor->tic_count == 10);
    CHECK(actor->shape == 58U);
    CHECK(actor->speed == 1536);
    CHECK((actor->flags & WG_ACTOR_FLAG_ATTACK_MODE) != 0U);
    CHECK((actor->flags & WG_ACTOR_FLAG_FIRST_ATTACK) != 0U);
    CHECK(WL_TickActors(&level, 1U));
    CHECK(actor->state == WG_STATE_CHASE1);
    CHECK(actor->tic_count == 9);
    CHECK(actor->direction == 1U);
    CHECK(actor->tile_x == 3U);
    CHECK(actor->tile_y == 1U);
    CHECK(actor->x == 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2 + 1536);
    CHECK(actor->y == 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2 - 1536);
    CHECK((actor->flags & WG_ACTOR_FLAG_FIRST_ATTACK) == 0U);
    CHECK(WL_TickActors(&level, 9U));
    CHECK(actor->state == WG_STATE_CHASE1S);
    CHECK(actor->tic_count == 3);
    CHECK(actor->x == 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2 + 1536);
    CHECK(actor->y == 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2 - 1536);
    CHECK(WL_TickActors(&level, 3U));
    CHECK(actor->state == WG_STATE_CHASE2);
    CHECK(actor->tic_count == 8);
    CHECK(actor->shape == 66U);
    CHECK(actor->x == 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2 + 6144);
    CHECK(actor->y == 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2 - 6144);
    actor->tile_x = level.player_tile_x;
    actor->tile_y = level.player_tile_y;
    actor->distance = 0;
    actor->state = WG_STATE_CHASE1;
    actor->tic_count = 10;
    CHECK(WL_TickActors(&level, 1U));
    CHECK(actor->state == WG_STATE_SHOOT1);
    CHECK(actor->tic_count == 20);
    CHECK(actor->shape == 96U);
    actor->flags |= WG_ACTOR_FLAG_VISIBLE;
    CHECK(WL_TickActors(&level, 20U));
    CHECK(actor->state == WG_STATE_SHOOT2);
    CHECK(actor->tic_count == 20);
    CHECK(actor->shape == 97U);
    CHECK(level.player_health == 100U);
    CHECK(WL_TickActors(&level, 20U));
    CHECK(actor->state == WG_STATE_SHOOT3);
    CHECK(actor->tic_count == 20);
    CHECK(actor->shape == 98U);
    CHECK(level.player_health == 74U);
    CHECK(level.damage_count == 26U);

    plane_one[2U * WG_LEVEL_SIZE + 2U] = 180U;
    CHECK(WG_LevelBuild(&map, &level));
    CHECK(level.actor_count == 0U);
    CHECK(level.tiles[2U * WG_LEVEL_SIZE + 2U] == WG_AMBUSH_TILE);

    plane_one[2U * WG_LEVEL_SIZE + 2U] = 108U;
    plane_zero[2U * WG_LEVEL_SIZE + 2U] = WG_AREA_TILE;
    plane_zero[2U * WG_LEVEL_SIZE + 4U] = 1U;
    CHECK(WG_LevelBuild(&map, &level));
    actor = &level.actors[0];
    CHECK(WL_UpdateAreaConnectivity(&level));
    CHECK(!WL_CheckLine(&level, actor));
    CHECK(!WL_CheckSight(&level, actor));
    plane_zero[2U * WG_LEVEL_SIZE + 4U] = WG_AREA_TILE;
    CHECK(WG_LevelBuild(&map, &level));
    actor = &level.actors[0];
    actor->direction = 4U;
    CHECK(WL_UpdateAreaConnectivity(&level));
    CHECK(WL_CheckLine(&level, actor));
    CHECK(!WL_CheckSight(&level, actor));
}

static void TestDoorAreaConnectivity(void)
{
    uint16_t plane_zero[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    uint16_t plane_one[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    wg_map_t map;
    wg_level_t level;
    size_t x;
    size_t y;

    memset(&map, 0, sizeof(map));
    for (y = 0U; y < WG_LEVEL_SIZE; ++y)
    {
        for (x = 0U; x < WG_LEVEL_SIZE; ++x)
        {
            size_t index = y * WG_LEVEL_SIZE + x;

            plane_zero[index] = x < 3U ? WG_AREA_TILE : WG_AREA_TILE + 1U;
            plane_one[index] = 0U;
        }
        plane_zero[y * WG_LEVEL_SIZE + 3U] = 1U;
    }
    plane_zero[2U * WG_LEVEL_SIZE + 3U] = 90U;
    plane_one[2U * WG_LEVEL_SIZE + 1U] = 19U;
    plane_one[2U * WG_LEVEL_SIZE + 4U] = 110U;
    map.width = WG_LEVEL_SIZE;
    map.height = WG_LEVEL_SIZE;
    map.planes[0] = plane_zero;
    map.planes[1] = plane_one;

    CHECK(WG_LevelBuild(&map, &level));
    CHECK(level.door_count == 1U);
    CHECK(WL_UpdateAreaConnectivity(&level));
    CHECK(level.area_by_player[0] != 0U);
    CHECK(level.area_by_player[1] == 0U);
    CHECK(!WL_CheckLine(&level, &level.actors[0]));
    level.actors[0].state = WG_STATE_CHASE1;
    level.actors[0].tic_count = 10;
    level.actors[0].flags |= WG_ACTOR_FLAG_ATTACK_MODE;
    level.actors[0].distance = 0;
    CHECK(WL_TickActors(&level, 1U));
    CHECK(level.actors[0].tile_x == 3U);
    CHECK(level.actors[0].tile_y == 2U);
    CHECK(level.actors[0].distance == -1);
    CHECK(level.actors[0].x == 4 * WG_FIXED_ONE + WG_FIXED_ONE / 2);
    level.doors[0].position = 0xffffU;
    CHECK(WL_UpdateAreaConnectivity(&level));
    CHECK(level.area_by_player[1] != 0U);
    CHECK(WL_CheckLine(&level, &level.actors[0]));
    CHECK(WL_CheckSight(&level, &level.actors[0]));
    CHECK(WL_TickActors(&level, 1U));
    CHECK(level.actors[0].distance == WG_FIXED_ONE - 512);
    CHECK(level.actors[0].x
          == 4 * WG_FIXED_ONE + WG_FIXED_ONE / 2 - 512);
}

static void TestOrdinaryShootingStates(void)
{
    wg_level_t level;
    wg_actor_t *actor;

    memset(&level, 0, sizeof(level));
    level.player_x = 5 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_y = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_tile_x = 5U;
    level.player_tile_y = 2U;
    level.player_health = 100U;
    level.difficulty = WG_DIFFICULTY_HARD;
    level.actor_count = 1U;
    actor = &level.actors[0];
    actor->x = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    actor->y = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    actor->tile_x = 2U;
    actor->tile_y = 2U;
    actor->area_number = 0U;
    actor->direction = 0U;
    actor->flags = WG_ACTOR_FLAG_SHOOTABLE | WG_ACTOR_FLAG_ATTACK_MODE
                   | WG_ACTOR_FLAG_VISIBLE;

    WG_RandomSeed(&level.random, 0U);
    actor->actor_class = WG_ACTOR_OFFICER;
    actor->attack_shape = 285U;
    actor->state = WG_STATE_SHOOT1;
    actor->tic_count = 6;
    actor->shape = 285U;
    CHECK(WL_TickActors(&level, 6U));
    CHECK(actor->state == WG_STATE_SHOOT2);
    CHECK(actor->tic_count == 20);
    CHECK(actor->shape == 286U);
    CHECK(level.player_health == 100U);
    CHECK(WL_TickActors(&level, 20U));
    CHECK(actor->state == WG_STATE_SHOOT3);
    CHECK(actor->shape == 287U);
    CHECK(level.player_health < 100U);

    WG_RandomSeed(&level.random, 0U);
    level.player_health = 100U;
    level.damage_count = 0U;
    actor->actor_class = WG_ACTOR_MUTANT;
    actor->attack_shape = 234U;
    actor->state = WG_STATE_SHOOT1;
    actor->tic_count = 6;
    actor->shape = 234U;
    CHECK(WL_TickActors(&level, 6U));
    CHECK(actor->state == WG_STATE_SHOOT2);
    CHECK(actor->shape == 235U);
    CHECK(level.player_health < 100U);
    CHECK(WL_TickActors(&level, 20U));
    CHECK(actor->state == WG_STATE_SHOOT3);
    CHECK(actor->shape == 236U);

    WG_RandomSeed(&level.random, 0U);
    level.player_health = 100U;
    level.damage_count = 0U;
    actor->actor_class = WG_ACTOR_SS;
    actor->attack_shape = 184U;
    actor->state = WG_STATE_SHOOT1;
    actor->tic_count = 20;
    actor->shape = 184U;
    CHECK(WL_TickActors(&level, 20U));
    CHECK(actor->state == WG_STATE_SHOOT2);
    CHECK(actor->shape == 185U);
    CHECK(WL_TickActors(&level, 20U));
    CHECK(actor->state == WG_STATE_SHOOT3);
    CHECK(actor->shape == 186U);
    CHECK(level.player_health < 100U);
    CHECK(WL_TickActors(&level, 10U));
    CHECK(actor->state == WG_STATE_SHOOT4);
    CHECK(actor->shape == 185U);

    level.difficulty = WG_DIFFICULTY_BABY;
    level.player_health = 100U;
    level.damage_count = 0U;
    level.player_dead = 0U;
    WL_TakeDamage(&level, 20U);
    CHECK(level.player_health == 95U);
    CHECK(level.damage_count == 5U);
    WL_TakeDamage(&level, 400U);
    CHECK(level.player_health == 0U);
    CHECK(level.player_dead != 0U);
}

static void TestDogChaseAndBite(void)
{
    uint16_t plane_zero[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    uint16_t plane_one[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    wg_map_t map;
    wg_level_t level;
    wg_actor_t *dog;
    size_t index;

    memset(&map, 0, sizeof(map));
    for (index = 0U; index < WG_LEVEL_SIZE * WG_LEVEL_SIZE; ++index)
    {
        plane_zero[index] = WG_AREA_TILE;
        plane_one[index] = 0U;
    }
    plane_one[2U * WG_LEVEL_SIZE + 2U] = 134U;
    plane_one[2U * WG_LEVEL_SIZE + 5U] = 19U;
    map.width = WG_LEVEL_SIZE;
    map.height = WG_LEVEL_SIZE;
    map.planes[0] = plane_zero;
    map.planes[1] = plane_one;

    CHECK(WG_LevelBuild(&map, &level));
    CHECK(level.actor_count == 1U);
    dog = &level.actors[0];
    CHECK(dog->actor_class == WG_ACTOR_DOG);
    CHECK(dog->attack_shape == 135U);
    CHECK(WL_TickAwareness(&level, 1U, 0));
    CHECK(dog->reaction_time == 2);
    CHECK(WL_TickAwareness(&level, 2U, 0));
    CHECK(dog->state == WG_STATE_CHASE1);
    CHECK(dog->speed == 3000);
    CHECK(WL_TickActors(&level, 1U));
    CHECK(dog->direction == 1U);
    CHECK(dog->tile_x == 3U);
    CHECK(dog->tile_y == 1U);
    CHECK(dog->x == 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2 + 3000);
    CHECK(dog->y == 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2 - 3000);
    CHECK(dog->distance == WG_FIXED_ONE - 3000);
    CHECK((dog->flags & WG_ACTOR_FLAG_FIRST_ATTACK) == 0U);

    dog->x = level.player_x - WG_FIXED_ONE - 1000;
    dog->y = level.player_y;
    dog->tile_x = (uint8_t)(level.player_tile_x - 1U);
    dog->tile_y = level.player_tile_y;
    dog->direction = 0U;
    dog->distance = WG_FIXED_ONE;
    dog->state = WG_STATE_CHASE1;
    dog->tic_count = 10;
    CHECK(WL_TickActors(&level, 1U));
    CHECK(dog->state == WG_STATE_DOG_JUMP1);
    CHECK(dog->tic_count == 10);
    CHECK(dog->shape == 135U);
    CHECK(WL_TickActors(&level, 10U));
    CHECK(dog->state == WG_STATE_DOG_JUMP2);
    CHECK(dog->shape == 136U);
    WG_RandomSeed(&level.random, 0U);
    CHECK(WL_TickActors(&level, 10U));
    CHECK(dog->state == WG_STATE_DOG_JUMP3);
    CHECK(dog->shape == 137U);
    CHECK(level.player_health == 94U);
    CHECK(level.damage_count == 6U);
    CHECK(WL_TickActors(&level, 20U));
    CHECK(dog->state == WG_STATE_DOG_JUMP5);
    CHECK(dog->shape == 99U);
}

static void TestBossShootingStates(void)
{
    static const wg_actor_class_t classes[] =
    {
        WG_ACTOR_BOSS, WG_ACTOR_GRETEL,
        WG_ACTOR_MECHA_HITLER, WG_ACTOR_REAL_HITLER
    };
    static const uint16_t attack_shapes[] = { 300U, 389U, 338U, 349U };
    wg_level_t level;
    size_t index;

    for (index = 0U; index < sizeof(classes) / sizeof(classes[0]); ++index)
    {
        wg_actor_t *actor;

        memset(&level, 0, sizeof(level));
        level.player_x = 6 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
        level.player_y = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
        level.player_tile_x = 6U;
        level.player_tile_y = 2U;
        level.player_health = 100U;
        level.difficulty = WG_DIFFICULTY_HARD;
        level.actor_count = 1U;
        actor = &level.actors[0];
        actor->x = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
        actor->y = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
        actor->tile_x = 2U;
        actor->tile_y = 2U;
        actor->area_number = 0U;
        actor->direction = 0U;
        actor->flags = WG_ACTOR_FLAG_SHOOTABLE | WG_ACTOR_FLAG_ATTACK_MODE
                       | WG_ACTOR_FLAG_VISIBLE;
        actor->actor_class = classes[index];
        actor->base_shape = (uint16_t)(attack_shapes[index] - 4U);
        actor->attack_shape = attack_shapes[index];
        actor->state = WG_STATE_SHOOT1;
        actor->tic_count = 30;
        actor->shape = attack_shapes[index];
        WG_RandomSeed(&level.random, 0U);

        CHECK(WL_TickActors(&level, 30U));
        CHECK(actor->state == WG_STATE_SHOOT2);
        CHECK(actor->tic_count == 10);
        CHECK(actor->shape == attack_shapes[index] + 1U);
        CHECK(level.player_health == 100U);
        CHECK(WL_TickActors(&level, 10U));
        CHECK(actor->state == WG_STATE_SHOOT3);
        CHECK(actor->shape == attack_shapes[index] + 2U);
        CHECK(level.player_health
              == (classes[index] == WG_ACTOR_BOSS ? 87U : 94U));

        if (classes[index] == WG_ACTOR_BOSS
            || classes[index] == WG_ACTOR_GRETEL)
        {
            CHECK(WL_TickActors(&level, 40U));
            CHECK(actor->state == WG_STATE_SHOOT7);
            CHECK(actor->shape == attack_shapes[index] + 2U);
            CHECK(WL_TickActors(&level, 10U));
            CHECK(actor->state == WG_STATE_SHOOT8);
            CHECK(actor->shape == attack_shapes[index]);
        }
        else
        {
            CHECK(WL_TickActors(&level, 30U));
            CHECK(actor->state == WG_STATE_SHOOT6);
            CHECK(actor->shape == attack_shapes[index] + 1U);
        }
    }
}

static void TestSchabbsNeedle(void)
{
    wg_level_t level;
    wg_actor_t *schabbs;
    wg_actor_t *needle;

    memset(&level, 0, sizeof(level));
    level.player_x = 5 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_y = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_tile_x = 5U;
    level.player_tile_y = 2U;
    level.player_health = 100U;
    level.difficulty = WG_DIFFICULTY_HARD;
    level.actor_count = 1U;
    schabbs = &level.actors[0];
    schabbs->x = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    schabbs->y = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    schabbs->tile_x = 2U;
    schabbs->tile_y = 2U;
    schabbs->area_number = 0U;
    schabbs->direction = 0U;
    schabbs->flags = WG_ACTOR_FLAG_SHOOTABLE | WG_ACTOR_FLAG_ATTACK_MODE;
    schabbs->actor_class = WG_ACTOR_SCHABBS;
    schabbs->base_shape = 307U;
    schabbs->attack_shape = 311U;
    schabbs->state = WG_STATE_SHOOT1;
    schabbs->tic_count = 30;
    schabbs->shape = 311U;
    schabbs->speed = 1536;
    WG_RandomSeed(&level.random, 0U);

    CHECK(WL_TickActors(&level, 30U));
    CHECK(schabbs->state == WG_STATE_SHOOT2);
    CHECK(schabbs->tic_count == 10);
    CHECK(schabbs->shape == 312U);
    CHECK(level.actor_count == 1U);
    schabbs->tic_count = 1;
    CHECK(WL_TickActors(&level, 1U));
    CHECK(level.actor_count == 2U);
    needle = &level.actors[1];
    CHECK(needle->actor_class == WG_ACTOR_NEEDLE);
    CHECK(needle->state == WG_STATE_NEEDLE2);
    CHECK(needle->tic_count == 6);
    CHECK(needle->shape == 318U);
    CHECK(needle->angle == 0U);
    CHECK(needle->speed == 0x2000);
    CHECK(needle->x == 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2 + 0x2000);
    CHECK(needle->y == 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2);

    needle->x = level.player_x - 0xc000;
    needle->y = level.player_y;
    needle->tile_x = 4U;
    needle->tile_y = 2U;
    schabbs->flags |= WG_ACTOR_FLAG_REMOVED;
    WG_RandomSeed(&level.random, 0U);
    CHECK(WL_TickActors(&level, 1U));
    CHECK((needle->flags & WG_ACTOR_FLAG_REMOVED) != 0U);
    CHECK(level.player_health == 79U);

    needle->x = 3 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    needle->y = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    needle->tile_x = 3U;
    needle->tile_y = 2U;
    needle->angle = 0U;
    needle->state = WG_STATE_NEEDLE1;
    needle->tic_count = 6;
    needle->flags = 0U;
    level.tiles[2U * WG_LEVEL_SIZE + 4U] = 1U;
    level.player_x = 10 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_tile_x = 10U;
    CHECK(WL_TickActors(&level, 3U));
    CHECK((needle->flags & WG_ACTOR_FLAG_REMOVED) != 0U);
    CHECK(level.player_health == 79U);
}

static void TestRocketBossAttacks(void)
{
    wg_level_t level;
    wg_actor_t *boss;
    wg_actor_t *rocket;
    wg_actor_t *smoke;

    memset(&level, 0, sizeof(level));
    level.player_x = 5 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_y = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_tile_x = 5U;
    level.player_tile_y = 2U;
    level.player_health = 100U;
    level.difficulty = WG_DIFFICULTY_HARD;
    level.actor_count = 1U;
    boss = &level.actors[0];
    boss->x = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    boss->y = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    boss->tile_x = 2U;
    boss->tile_y = 2U;
    boss->area_number = 0U;
    boss->direction = 0U;
    boss->flags = WG_ACTOR_FLAG_SHOOTABLE | WG_ACTOR_FLAG_ATTACK_MODE;
    boss->actor_class = WG_ACTOR_GIFT;
    boss->base_shape = 360U;
    boss->attack_shape = 364U;
    boss->state = WG_STATE_SHOOT1;
    boss->tic_count = 30;
    boss->shape = 364U;
    boss->speed = 1536;
    WG_RandomSeed(&level.random, 0U);

    CHECK(WL_TickActors(&level, 30U));
    CHECK(boss->state == WG_STATE_SHOOT2);
    CHECK(boss->shape == 365U);
    CHECK(level.actor_count == 1U);
    boss->tic_count = 1;
    CHECK(WL_TickActors(&level, 1U));
    CHECK(level.actor_count == 3U);
    rocket = &level.actors[1];
    smoke = &level.actors[2];
    CHECK(rocket->actor_class == WG_ACTOR_ROCKET);
    CHECK(rocket->state == WG_STATE_ROCKET);
    CHECK(rocket->shape == 370U);
    CHECK(rocket->rotate == 1U);
    CHECK(rocket->angle == 0U);
    CHECK(rocket->tic_count == 3);
    CHECK(rocket->x == 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2 + 0x2000);
    CHECK(smoke->actor_class == WG_ACTOR_SMOKE);
    CHECK(smoke->state == WG_STATE_SMOKE1);
    CHECK(smoke->shape == 378U);
    CHECK(smoke->tic_count == 5);
    CHECK(smoke->x == 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2);

    boss->flags |= WG_ACTOR_FLAG_REMOVED;
    smoke->flags |= WG_ACTOR_FLAG_REMOVED;
    rocket->x = level.player_x - 0xc000;
    rocket->y = level.player_y;
    rocket->tile_x = 4U;
    rocket->tile_y = 2U;
    WG_RandomSeed(&level.random, 0U);
    CHECK(WL_TickActors(&level, 1U));
    CHECK((rocket->flags & WG_ACTOR_FLAG_REMOVED) != 0U);
    CHECK(level.player_health == 69U);

    memset(rocket, 0, sizeof(*rocket));
    rocket->x = 3 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    rocket->y = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    rocket->tile_x = 3U;
    rocket->tile_y = 2U;
    rocket->angle = 0U;
    rocket->shape = 370U;
    rocket->base_shape = 370U;
    rocket->rotate = 1U;
    rocket->tic_count = 3;
    rocket->speed = 0x2000;
    rocket->state = WG_STATE_ROCKET;
    rocket->actor_class = WG_ACTOR_ROCKET;
    level.tiles[2U * WG_LEVEL_SIZE + 4U] = 1U;
    level.player_x = 10 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_tile_x = 10U;
    CHECK(WL_TickActors(&level, 3U));
    CHECK(rocket->actor_class == WG_ACTOR_EXPLOSION);
    CHECK(rocket->state == WG_STATE_BOOM1);
    CHECK(rocket->shape == 382U);
    CHECK(rocket->tic_count == 3);
    CHECK(WL_TickActors(&level, 3U));
    CHECK(rocket->state == WG_STATE_BOOM2);
    CHECK(rocket->shape == 383U);

    memset(&level, 0, sizeof(level));
    level.player_x = 6 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_y = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_tile_x = 6U;
    level.player_tile_y = 2U;
    level.player_health = 100U;
    level.difficulty = WG_DIFFICULTY_HARD;
    level.actor_count = 1U;
    boss = &level.actors[0];
    boss->x = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    boss->y = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    boss->tile_x = 2U;
    boss->tile_y = 2U;
    boss->area_number = 0U;
    boss->flags = WG_ACTOR_FLAG_SHOOTABLE | WG_ACTOR_FLAG_ATTACK_MODE
                  | WG_ACTOR_FLAG_VISIBLE;
    boss->actor_class = WG_ACTOR_FAT;
    boss->base_shape = 396U;
    boss->attack_shape = 400U;
    boss->state = WG_STATE_SHOOT2;
    boss->tic_count = 1;
    boss->shape = 401U;
    WG_RandomSeed(&level.random, 0U);
    CHECK(WL_TickActors(&level, 1U));
    CHECK(boss->state == WG_STATE_SHOOT3);
    CHECK(boss->shape == 402U);
    CHECK(level.actor_count >= 2U);
    CHECK(level.actors[1].actor_class == WG_ACTOR_ROCKET);
    boss->tic_count = 1;
    WG_RandomSeed(&level.random, 0U);
    CHECK(WL_TickActors(&level, 1U));
    CHECK(boss->state == WG_STATE_SHOOT4);
    CHECK(boss->shape == 403U);
    CHECK(level.player_health == 94U);

    memset(&level, 0, sizeof(level));
    level.player_x = 5 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_y = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_tile_x = 5U;
    level.player_tile_y = 2U;
    level.player_health = 100U;
    level.actor_count = 1U;
    boss = &level.actors[0];
    boss->x = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    boss->y = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    boss->tile_x = 3U;
    boss->tile_y = 2U;
    boss->area_number = 0U;
    boss->direction = 0U;
    boss->flags = WG_ACTOR_FLAG_SHOOTABLE | WG_ACTOR_FLAG_ATTACK_MODE;
    boss->actor_class = WG_ACTOR_GIFT;
    boss->base_shape = 360U;
    boss->attack_shape = 364U;
    boss->state = WG_STATE_CHASE1;
    boss->tic_count = 10;
    boss->speed = 1536;
    boss->distance = 1000;
    WG_RandomSeed(&level.random, 0U);
    CHECK(WL_TickActors(&level, 1U));
    CHECK(boss->direction == 4U);
    CHECK(boss->tile_x == 2U);
    CHECK(boss->x == 3 * WG_FIXED_ONE + WG_FIXED_ONE / 2 - 536);
}

static void TestHuffman(void)
{
    wg_huffman_node_t nodes[255];
    const uint8_t source[] = { 0x06 };
    uint8_t destination[4];

    memset(nodes, 0, sizeof(nodes));
    nodes[254].bit0 = 'A';
    nodes[254].bit1 = 'B';
    CHECK(WG_HuffmanExpand(source, sizeof(source), destination,
                           sizeof(destination), nodes));
    CHECK(memcmp(destination, "ABBA", sizeof(destination)) == 0);
}

static void TestCarmack(void)
{
    const uint8_t source[] =
    {
        0x11, 0x11, 0x22, 0x22,
        0x02, 0xa7, 0x02,
        0x00, 0xa7, 0x34
    };
    const uint16_t expected[] =
    {
        0x1111, 0x2222, 0x1111, 0x2222, 0xa734
    };
    uint16_t destination[5];

    CHECK(WG_CarmackExpand(source, sizeof(source), destination,
                           sizeof(destination) / sizeof(destination[0])));
    CHECK(memcmp(destination, expected, sizeof(expected)) == 0);
}

static void TestRLEW(void)
{
    const uint16_t source[] = { 1, 0xabcd, 3, 2, 3 };
    const uint16_t expected[] = { 1, 2, 2, 2, 3 };
    uint16_t destination[5];

    CHECK(WG_RLEWExpand(source, sizeof(source) / sizeof(source[0]),
                        destination,
                        sizeof(destination) / sizeof(destination[0]),
                        0xabcd));
    CHECK(memcmp(destination, expected, sizeof(expected)) == 0);
}

static void TestMalformedCompression(void)
{
    wg_huffman_node_t nodes[255];
    uint8_t byte_output[2];
    uint16_t word_output[4];
    const uint8_t bad_carmack[] = { 0x04, 0xa7, 0x01 };
    const uint16_t bad_rlew[] = { 0xabcd, 5, 1 };

    memset(nodes, 0, sizeof(nodes));
    nodes[254].bit0 = 510;
    nodes[254].bit1 = 510;
    CHECK(!WG_HuffmanExpand((const uint8_t *)"\0", 1, byte_output,
                            sizeof(byte_output), nodes));
    CHECK(!WG_CarmackExpand(bad_carmack, sizeof(bad_carmack), word_output,
                            sizeof(word_output) / sizeof(word_output[0])));
    CHECK(!WG_RLEWExpand(bad_rlew,
                         sizeof(bad_rlew) / sizeof(bad_rlew[0]), word_output,
                         sizeof(word_output) / sizeof(word_output[0]), 0xabcd));
}

static void TestVideo(void)
{
    uint8_t source[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT];
    uint8_t destination[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT];
    uint8_t small[] = { 1, 2, 3, 4, 5, 6 };
    uint8_t black[256 * 3];
    uint8_t faded[256 * 3];
    uint8_t converted[256 * 3];
    wg_fizzle_t fizzle;
    size_t index;
    unsigned iterations = 0;

    WG_VideoClear(destination, 7);
    CHECK(destination[0] == 7);
    CHECK(destination[sizeof(destination) - 1U] == 7);
    WG_VideoBar(destination, -2, -1, 4, 3, 9);
    CHECK(destination[0] == 9);
    CHECK(destination[1] == 9);
    CHECK(destination[WG_VIDEO_WIDTH] == 9);
    CHECK(destination[WG_VIDEO_WIDTH + 1U] == 9);
    WG_VideoPlot(destination, WG_VIDEO_WIDTH, 0, 3);
    CHECK(destination[WG_VIDEO_WIDTH - 1U] == 7);

    WG_VideoClear(destination, 0);
    WG_VideoBlit(destination, -1, 1, small, 3, 2);
    CHECK(destination[WG_VIDEO_WIDTH] == 2);
    CHECK(destination[WG_VIDEO_WIDTH + 1U] == 3);
    CHECK(destination[WG_VIDEO_WIDTH * 2U] == 5);
    CHECK(destination[WG_VIDEO_WIDTH * 2U + 1U] == 6);

    memset(black, 0, sizeof(black));
    WG_PaletteFade(black, WG_WolfPaletteVGA, 15, 30, faded);
    CHECK(faded[5] == 21U);
    WG_PaletteFade(black, WG_WolfPaletteVGA, 30, 30, faded);
    CHECK(memcmp(faded, WG_WolfPaletteVGA, sizeof(faded)) == 0);
    WG_PaletteFromVGA(WG_WolfPaletteVGA, converted);
    CHECK(memcmp(converted, WG_WolfPalette, sizeof(converted)) == 0);

    for (index = 0; index < sizeof(source); ++index)
    {
        source[index] = (uint8_t)(index * 37U + 11U);
    }
    memset(destination, 0, sizeof(destination));
    WG_FizzleStart(&fizzle);
    while (!WG_FizzleStep(&fizzle, source, destination,
                           WG_VIDEO_WIDTH, WG_VIDEO_HEIGHT, 1000U))
    {
        CHECK(++iterations < 1000U);
    }
    CHECK(memcmp(source, destination, sizeof(source)) == 0);
}

static void TestRandom(void)
{
    wg_random_t random;

    WG_RandomSeed(&random, 0);
    CHECK(WG_RandomNext(&random) == 8U);
    CHECK(WG_RandomNext(&random) == 109U);
    WG_RandomSeed(&random, 255U);
    CHECK(WG_RandomNext(&random) == 0U);
}

static void TestViewMath(void)
{
    wg_view_tables_t tables;
    const int32_t *cosine;

    memset(&tables, 0, sizeof(tables));
    CHECK(WG_FixedMul(WG_FIXED_ONE, WG_FIXED_ONE / 2) ==
          WG_FIXED_ONE / 2);
    CHECK(WG_FixedMul(-WG_FIXED_ONE, WG_FIXED_ONE / 2) ==
          -WG_FIXED_ONE / 2);
    WG_ViewBuildTrigTables(&tables);
    cosine = WG_ViewCosineTable(&tables);
    CHECK(cosine != NULL);
    CHECK(tables.sine[0] == 0);
    CHECK(tables.sine[WG_ANGLE_QUADRANT] == WG_FIXED_ONE);
    CHECK(tables.sine[2 * WG_ANGLE_QUADRANT] == 0);
    CHECK(tables.sine[3 * WG_ANGLE_QUADRANT] == -WG_FIXED_ONE);
    CHECK(cosine[0] == WG_FIXED_ONE);
    CHECK(cosine[WG_ANGLE_QUADRANT] == 0);
    CHECK(tables.fine_tangent[0] > 0);
    CHECK(tables.fine_tangent[WG_FINE_ANGLES / 4 - 1] > WG_FIXED_ONE);
    CHECK(WG_ViewCalculateProjection(&tables, WG_MAX_VIEW_WIDTH,
                                     WG_FOCAL_LENGTH));
    CHECK(tables.pixel_angle[159] == 0);
    CHECK(tables.pixel_angle[160] == 0);
    CHECK(tables.pixel_angle[0] == -tables.pixel_angle[319]);
    CHECK(tables.scale == 218);
    CHECK(tables.height_numerator == 223232);
    CHECK(tables.min_height_divisor == 7);
    CHECK(tables.max_slope > 0);
    CHECK(!WG_ViewCalculateProjection(&tables, 319, WG_FOCAL_LENGTH));
}

static void TestWallScaler(void)
{
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT];
    uint8_t texture[WG_TEXTURE_SIZE * WG_TEXTURE_SIZE];
    int x;
    int y;

    memset(framebuffer, 3, sizeof(framebuffer));
    for (y = 0; y < WG_TEXTURE_SIZE; ++y)
    {
        for (x = 0; x < WG_TEXTURE_SIZE; ++x)
        {
            texture[y * WG_TEXTURE_SIZE + x] = (uint8_t)y;
        }
    }
    CHECK(WG_ScaleWallPost(framebuffer, 0, 0, 320, 160, 10, 2,
                           texture, 7, 256));
    CHECK(framebuffer[47 * WG_VIDEO_WIDTH + 10] == 3);
    CHECK(framebuffer[48 * WG_VIDEO_WIDTH + 10] == 0);
    CHECK(framebuffer[48 * WG_VIDEO_WIDTH + 11] == 0);
    CHECK(framebuffer[111 * WG_VIDEO_WIDTH + 10] == 63);
    CHECK(framebuffer[112 * WG_VIDEO_WIDTH + 10] == 3);
    CHECK(!WG_ScaleWallPost(framebuffer, 0, 0, 320, 160, 319, 2,
                            texture, 0, 256));
}

static void TestStaticRaycaster(void)
{
    wg_level_t level;
    wg_view_tables_t tables;
    wg_wall_hit_t hits[WG_MAX_VIEW_WIDTH];
    int x;
    int y;

    memset(&level, 0, sizeof(level));
    memset(&tables, 0, sizeof(tables));
    for (x = 0; x < WG_LEVEL_SIZE; ++x)
    {
        level.tiles[x] = 1;
        level.tiles[(WG_LEVEL_SIZE - 1) * WG_LEVEL_SIZE + x] = 1;
    }
    for (y = 0; y < WG_LEVEL_SIZE; ++y)
    {
        level.tiles[y * WG_LEVEL_SIZE] = 1;
        level.tiles[y * WG_LEVEL_SIZE + WG_LEVEL_SIZE - 1] = 1;
    }
    WG_ViewBuildTrigTables(&tables);
    CHECK(WG_ViewCalculateProjection(&tables, WG_MAX_VIEW_WIDTH,
                                     WG_FOCAL_LENGTH));
    CHECK(WG_RaycastStaticWalls(&level, &tables,
                                32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                                32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                                0, hits));
    CHECK(hits[159].side == WG_WALL_VERTICAL);
    CHECK(hits[159].map_x == WG_LEVEL_SIZE - 1);
    CHECK(hits[159].map_y == 32);
    CHECK(hits[159].tile == 1);
    CHECK(hits[159].wall_page == 1);
    CHECK(hits[159].texture_column == 30);
    CHECK(hits[159].height == 28);
    CHECK(hits[160].x == hits[159].x);
    CHECK(hits[160].y == hits[159].y);
    CHECK(WG_RaycastStaticWalls(&level, &tables,
                                32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                                32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                                90, hits));
    CHECK(hits[159].side == WG_WALL_HORIZONTAL);
    CHECK(hits[159].map_y == 0);
    CHECK(WG_RaycastStaticWalls(&level, &tables,
                                32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                                32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                                180, hits));
    CHECK(hits[159].side == WG_WALL_VERTICAL);
    CHECK(hits[159].map_x == 0);
    CHECK(WG_RaycastStaticWalls(&level, &tables,
                                32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                                32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                                270, hits));
    CHECK(hits[159].side == WG_WALL_HORIZONTAL);
    CHECK(hits[159].map_y == WG_LEVEL_SIZE - 1);

    level.tiles[32 * WG_LEVEL_SIZE + 40] = 0x80U;
    level.door_count = 1;
    level.doors[0].tile_x = 40;
    level.doors[0].tile_y = 32;
    level.doors[0].vertical = 1;
    level.doors[0].lock = WG_DOOR_NORMAL;
    CHECK(WG_RaycastWalls(&level, &tables,
                          32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                          32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                          0, 10, hits));
    CHECK(hits[159].side == WG_WALL_DOOR);
    CHECK(hits[159].map_x == 40);
    CHECK(hits[159].wall_page == 11);
    level.doors[0].position = 0xffffU;
    CHECK(WG_RaycastWalls(&level, &tables,
                          32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                          32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                          0, 10, hits));
    CHECK(hits[159].side == WG_WALL_VERTICAL);
    CHECK(hits[159].map_x == WG_LEVEL_SIZE - 1);
}

static void TestStaticRenderer(void)
{
    wg_level_t level;
    wg_view_tables_t tables;
    wg_wall_cache_t walls;
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT];
    uint8_t wall_pixels[8 * WG_TEXTURE_SIZE * WG_TEXTURE_SIZE];
    wg_wall_hit_t hits[WG_MAX_VIEW_WIDTH];
    uint8_t visible_tiles[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    int x;
    int y;

    memset(&level, 0, sizeof(level));
    memset(&tables, 0, sizeof(tables));
    memset(framebuffer, 0, sizeof(framebuffer));
    memset(wall_pixels, 66, WG_TEXTURE_SIZE * WG_TEXTURE_SIZE);
    memset(wall_pixels + WG_TEXTURE_SIZE * WG_TEXTURE_SIZE, 77,
           WG_TEXTURE_SIZE * WG_TEXTURE_SIZE);
    walls.pixels = wall_pixels;
    walls.count = 8;
    for (x = 0; x < WG_LEVEL_SIZE; ++x)
    {
        level.tiles[x] = 1;
        level.tiles[(WG_LEVEL_SIZE - 1) * WG_LEVEL_SIZE + x] = 1;
    }
    for (y = 0; y < WG_LEVEL_SIZE; ++y)
    {
        level.tiles[y * WG_LEVEL_SIZE] = 1;
        level.tiles[y * WG_LEVEL_SIZE + WG_LEVEL_SIZE - 1] = 1;
    }
    WG_ViewBuildTrigTables(&tables);
    CHECK(WG_ViewCalculateProjection(&tables, WG_MAX_VIEW_WIDTH,
                                     WG_FOCAL_LENGTH));
    CHECK(WG_RenderStaticView(framebuffer, &level, &tables, &walls, 0, 0,
                              32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                              32 * WG_FIXED_ONE + WG_FIXED_ONE / 2, 0, hits,
                              visible_tiles));
    CHECK(framebuffer[0] == 0x1d);
    CHECK(framebuffer[159 * WG_VIDEO_WIDTH] == 0x19);
    CHECK(framebuffer[80 * WG_VIDEO_WIDTH + 159] == 77);
    CHECK(visible_tiles[32 * WG_LEVEL_SIZE + 32] == 1U);
    CHECK(visible_tiles[32 * WG_LEVEL_SIZE + 40] == 1U);
}

static void TestDataSet(const char *path, wg_game_variant_t expected_variant,
                        size_t expected_graphics_offsets)
{
    wg_data_set_t data_set;
    wg_graphics_t graphics;
    wg_font_t font;
    wg_audio_t audio;
    wg_pages_t pages;
    wg_maps_t maps;
    wg_map_t map;
    wg_level_t level;
    const uint8_t *page_data;
    size_t page_size;
    uint8_t framebuffer[320 * 200];
    uint8_t *picture_pixels;
    uint16_t picture_width;
    uint16_t picture_height;
    uint64_t frame_hash = 1469598103934665603ULL;
    uint64_t map_hash = 1469598103934665603ULL;
    uint64_t view_hash = 1469598103934665603ULL;
    uint64_t scenery_hash = 1469598103934665603ULL;
    uint64_t hud_hash = 1469598103934665603ULL;
    uint64_t open_view_hash = 1469598103934665603ULL;
    uint64_t guard_view_hash = 1469598103934665603ULL;
    size_t index;
    size_t actor_index;
    size_t decoded_graphics = 0;
    size_t loaded_maps = 0;
    size_t present_pages = 0;
    size_t decoded_sprites = 0;
    size_t actor_classes[WG_ACTOR_FAT + 1] = { 0 };

    CHECK(WG_DataOpen(&data_set, path));
    if (data_set.variant == WG_GAME_UNKNOWN)
    {
        return;
    }

    CHECK(data_set.variant == expected_variant);
    CHECK(data_set.page_count == 663);
    CHECK(data_set.sprite_start == 106);
    CHECK(data_set.sound_start == 542);
    CHECK(data_set.rlew_tag == 0xabcd);
    CHECK(data_set.graphics_offset_count == expected_graphics_offsets);
    CHECK(data_set.audio_offset_count == 289);
    CHECK(data_set.pages[0].offset == 4096);
    CHECK(data_set.pages[0].length != 0);

    CHECK(WG_GraphicsOpen(&graphics, &data_set));
    if (graphics.offsets != NULL)
    {
        CHECK(graphics.picture_count == (expected_variant
              == WG_GAME_WOLF3D_SHAREWARE_14 ? 144U : 132U));
        CHECK(WG_GraphicsDecodeTitle(&graphics, data_set.variant, framebuffer));
        for (index = 0; index < sizeof(framebuffer); ++index)
        {
            frame_hash ^= framebuffer[index];
            frame_hash *= 1099511628211ULL;
        }
        printf("%s title framebuffer FNV-1a: %016llx\n",
               WG_DataVariantName(data_set.variant),
               (unsigned long long)frame_hash);
        CHECK(frame_hash == 0x01e337d015f1541cULL);
        CHECK(WG_WolfPalette[0] == 0);
        CHECK(WG_WolfPalette[1] == 0);
        CHECK(WG_WolfPalette[2] == 0);
        CHECK(WG_WolfPalette[5] == 170);
        CHECK(WG_FontOpen(&font, &graphics, 0));
        if (font.data != NULL)
        {
            CHECK(font.height != 0U);
            CHECK(WG_FontMeasure(&font, "WOLF") != 0U);
            memset(framebuffer, 0, sizeof(framebuffer));
            WG_FontDraw(&font, framebuffer, 4, 4, "WOLF", 15);
            CHECK(memchr(framebuffer, 15, sizeof(framebuffer)) != NULL);
            WG_FontClose(&font);
        }
        CHECK(WG_GraphicsDecodePicture(&graphics,
              expected_variant == WG_GAME_WOLF3D_SHAREWARE_14 ? 98U : 86U,
              &picture_pixels, &picture_width, &picture_height));
        if (picture_pixels != NULL)
        {
            CHECK(picture_width == 320U);
            CHECK(picture_height == 40U);
            free(picture_pixels);
        }
        for (index = 0; index + 1U < graphics.offset_count; ++index)
        {
            uint8_t *chunk_data = NULL;
            size_t chunk_size;

            if (graphics.offsets[index] == 0x00ffffffU)
            {
                continue;
            }
            if ((expected_variant == WG_GAME_WOLF3D_SHAREWARE_14
                 && index == 147U)
                || (expected_variant == WG_GAME_WOLF3D_FULL_GT_14
                    && index == 135U))
            {
                continue;
            }
            if (!WG_GraphicsDecodeChunk(&graphics, index,
                                         &chunk_data, &chunk_size))
            {
                fprintf(stderr, "failed to decode graphics chunk %u\n",
                        (unsigned)index);
                CHECK(0);
            }
            if (chunk_data != NULL)
            {
                CHECK(chunk_size != 0U);
                ++decoded_graphics;
                free(chunk_data);
            }
        }
        CHECK(decoded_graphics != 0U);
        WG_GraphicsClose(&graphics);
    }

    CHECK(WG_PagesOpen(&pages, &data_set));
    if (pages.file.data != NULL)
    {
        CHECK(WG_PagesGet(&pages, 0, &page_data, &page_size));
        CHECK(page_size == 4096U);
        CHECK(WG_PagesGet(&pages, data_set.sprite_start,
                          &page_data, &page_size));
        CHECK(page_size != 0U);
        CHECK(!WG_PagesGet(&pages, data_set.page_count,
                           &page_data, &page_size));
        for (index = 0; index < data_set.page_count; ++index)
        {
            if (data_set.pages[index].offset == 0
                || data_set.pages[index].length == 0)
            {
                continue;
            }
            CHECK(WG_PagesGet(&pages, index, &page_data, &page_size));
            if (page_data != NULL)
            {
                CHECK(page_size == data_set.pages[index].length);
                ++present_pages;
            }
        }
        CHECK(present_pages != 0U);
        {
            uint8_t wall[WG_TEXTURE_SIZE * WG_TEXTURE_SIZE];
            wg_sprite_image_t sprite;
            wg_wall_cache_t wall_cache;

            CHECK(WG_DecodeWall(&pages, 0, wall));
            CHECK(WG_WallCacheLoad(&wall_cache, &pages));
            CHECK(wall_cache.count == data_set.sprite_start);
            WG_WallCacheFree(&wall_cache);
            for (index = 0;
                 index < (size_t)(data_set.sound_start - data_set.sprite_start);
                 ++index)
            {
                size_t sprite_page = (size_t)data_set.sprite_start + index;

                if (data_set.pages[sprite_page].offset == 0
                    || data_set.pages[sprite_page].length == 0)
                {
                    continue;
                }
                if (!WG_DecodeSprite(&pages, index, &sprite))
                {
                    fprintf(stderr, "failed to decode sprite %u\n",
                            (unsigned)index);
                    CHECK(0);
                }
                else
                {
                    ++decoded_sprites;
                }
            }
            CHECK(decoded_sprites == (expected_variant
                  == WG_GAME_WOLF3D_SHAREWARE_14 ? 226U : 436U));
        }
        WG_PagesClose(&pages);
    }

    CHECK(WG_MapsOpen(&maps, &data_set));
    if (maps.header_offsets != NULL)
    {
        CHECK(maps.rlew_tag == data_set.rlew_tag);
        CHECK(WG_MapsLoad(&maps, 0, &map));
        if (map.planes[0] != NULL)
        {
            CHECK(map.width == 64U);
            CHECK(map.height == 64U);
            CHECK(WG_LevelBuild(&map, &level));
            CHECK(level.player_tile_x == 29U);
            CHECK(level.player_tile_y == 57U);
            CHECK(level.player_x == 0x001d8000L);
            CHECK(level.player_y == 0x00398000L);
            CHECK(level.player_angle == 0U);
            CHECK(level.door_count > 0U);
            CHECK(level.static_count == 121U);
            CHECK(level.actor_count == 21U);
            CHECK(ActorClassCount(&level, WG_ACTOR_GUARD) == 17U);
            CHECK(ActorClassCount(&level, WG_ACTOR_DOG) == 3U);
            CHECK(ActorClassCount(&level, WG_ACTOR_OFFICER) == 0U);
            CHECK(ActorClassCount(&level, WG_ACTOR_SS) == 0U);
            CHECK(ActorClassCount(&level, WG_ACTOR_MUTANT) == 0U);
            CHECK(ActorClassCount(&level, WG_ACTOR_INERT) == 1U);
            CHECK(level.actors[19].tile_x == 31U);
            CHECK(level.actors[19].tile_y == 57U);
            CHECK(level.actors[19].shape == 95U);
            CHECK(level.actors[19].rotate == 0U);
            CHECK(level.actors[20].tile_x == 39U);
            CHECK(level.actors[20].tile_y == 61U);
            CHECK(level.actors[20].direction == 4U);
            CHECK(level.actors[20].shape == 50U);
            printf("%s map 0 static objects: %u\n",
                   WG_DataVariantName(data_set.variant),
                   (unsigned)level.static_count);
            printf("%s map 0 initial actors: %u\n",
                   WG_DataVariantName(data_set.variant),
                   (unsigned)level.actor_count);
            {
                wg_view_tables_t view_tables;
                wg_wall_cache_t wall_cache;
                wl_status_t status;
                wg_wall_hit_t render_hits[WG_MAX_VIEW_WIDTH];
                uint8_t visible_tiles[WG_LEVEL_SIZE * WG_LEVEL_SIZE];

                memset(&view_tables, 0, sizeof(view_tables));
                memset(&wall_cache, 0, sizeof(wall_cache));
                WG_ViewBuildTrigTables(&view_tables);
                CHECK(WG_ViewCalculateProjection(&view_tables,
                                                 WG_MAX_VIEW_WIDTH,
                                                 WG_FOCAL_LENGTH));
                CHECK(WG_PagesOpen(&pages, &data_set));
                CHECK(WG_WallCacheLoad(&wall_cache, &pages));
                memset(framebuffer, 0, sizeof(framebuffer));
                CHECK(WG_RenderStaticView(framebuffer, &level, &view_tables,
                                          &wall_cache, 0, 0,
                                          level.player_x, level.player_y,
                                          level.player_angle, render_hits,
                                          visible_tiles));
                for (index = 0; index < sizeof(framebuffer); ++index)
                {
                    view_hash ^= framebuffer[index];
                    view_hash *= 1099511628211ULL;
                }
                printf("%s initial play view FNV-1a: %016llx\n",
                       WG_DataVariantName(data_set.variant),
                       (unsigned long long)view_hash);
                CHECK(view_hash == 0x52a9cf2dd9dcab66ULL);
                CHECK(WL_DrawScaleds(framebuffer, &pages, &level,
                                     &view_tables, render_hits, visible_tiles,
                                     level.player_x, level.player_y,
                                     level.player_angle));
                for (index = 0; index < sizeof(framebuffer); ++index)
                {
                    scenery_hash ^= framebuffer[index];
                    scenery_hash *= 1099511628211ULL;
                }
                printf("%s initial scenery view FNV-1a: %016llx\n",
                       WG_DataVariantName(data_set.variant),
                       (unsigned long long)scenery_hash);
                CHECK(scenery_hash == 0x723ccdefbb003ac1ULL);
                CHECK(WL_DrawPlayerWeapon(framebuffer, &pages, 1, 0));
                CHECK(WG_GraphicsOpen(&graphics, &data_set));
                WL_StatusDefaults(&status);
                CHECK(WL_DrawStatusBar(framebuffer, &graphics, &status));
                for (index = 0; index < sizeof(framebuffer); ++index)
                {
                    hud_hash ^= framebuffer[index];
                    hud_hash *= 1099511628211ULL;
                }
                printf("%s initial HUD view FNV-1a: %016llx\n",
                       WG_DataVariantName(data_set.variant),
                       (unsigned long long)hud_hash);
                CHECK(hud_hash == 0x0b077346cfd7b513ULL);
                for (index = 0; index < level.door_count; ++index)
                {
                    level.doors[index].position = 0xffffU;
                }
                CHECK(WG_RenderStaticView(framebuffer, &level, &view_tables,
                                          &wall_cache, 0, 0,
                                          level.player_x, level.player_y,
                                          level.player_angle, render_hits,
                                          visible_tiles));
                CHECK(WL_DrawScaleds(framebuffer, &pages, &level,
                                     &view_tables, render_hits, visible_tiles,
                                     level.player_x, level.player_y,
                                     level.player_angle));
                CHECK(WL_DrawPlayerWeapon(framebuffer, &pages, 1, 0));
                CHECK(WL_DrawStatusBar(framebuffer, &graphics, &status));
                for (index = 0; index < sizeof(framebuffer); ++index)
                {
                    open_view_hash ^= framebuffer[index];
                    open_view_hash *= 1099511628211ULL;
                }
                printf("%s open-door scenery FNV-1a: %016llx\n",
                       WG_DataVariantName(data_set.variant),
                       (unsigned long long)open_view_hash);
                CHECK(open_view_hash == 0xd2e29d9233c925ccULL);
                for (index = 0; index < level.door_count; ++index)
                {
                    level.doors[index].position = 0U;
                }
                {
                    const wg_actor_t *guard = FindLastLiveGuard(&level);

                    CHECK(guard != NULL);
                    if (guard != NULL)
                    {
                        CHECK(WG_RenderStaticView(
                            framebuffer, &level, &view_tables, &wall_cache,
                            0, 0, guard->x - 3 * WG_FIXED_ONE, guard->y, 0,
                            render_hits, visible_tiles));
                        CHECK(WL_DrawScaleds(
                            framebuffer, &pages, &level, &view_tables,
                            render_hits, visible_tiles,
                            guard->x - 3 * WG_FIXED_ONE, guard->y, 0));
                    }
                }
                CHECK(WL_DrawPlayerWeapon(framebuffer, &pages, 1, 0));
                CHECK(WL_DrawStatusBar(framebuffer, &graphics, &status));
                for (index = 0; index < sizeof(framebuffer); ++index)
                {
                    guard_view_hash ^= framebuffer[index];
                    guard_view_hash *= 1099511628211ULL;
                }
                printf("%s guard view FNV-1a: %016llx\n",
                       WG_DataVariantName(data_set.variant),
                       (unsigned long long)guard_view_hash);
                CHECK(guard_view_hash == 0xa6db229142f7150bULL);
                WG_GraphicsClose(&graphics);
                WG_WallCacheFree(&wall_cache);
                WG_PagesClose(&pages);
            }
            {
                wg_level_t difficulty_level;

                CHECK(WG_LevelBuildForDifficulty(
                    &map, WG_DIFFICULTY_BABY, &difficulty_level));
                printf("%s baby actors: %u\n",
                       WG_DataVariantName(data_set.variant),
                       (unsigned)difficulty_level.actor_count);
                CHECK(difficulty_level.actor_count == 12U);
                CHECK(WG_LevelBuildForDifficulty(
                    &map, WG_DIFFICULTY_HARD, &difficulty_level));
                printf("%s hard actors: %u\n",
                       WG_DataVariantName(data_set.variant),
                       (unsigned)difficulty_level.actor_count);
                CHECK(difficulty_level.actor_count == 38U);
            }
            printf("%s map 0 player: (%u,%u) angle %u\n",
                   WG_DataVariantName(data_set.variant),
                   (unsigned)level.player_tile_x,
                   (unsigned)level.player_tile_y,
                   (unsigned)level.player_angle);
            for (index = 0; index < (size_t)map.width * map.height; ++index)
            {
                uint16_t value = map.planes[0][index];
                map_hash ^= value & 0xffU;
                map_hash *= 1099511628211ULL;
                map_hash ^= value >> 8;
                map_hash *= 1099511628211ULL;
            }
            printf("%s map 0 plane 0 FNV-1a: %016llx (%s)\n",
                   WG_DataVariantName(data_set.variant),
                   (unsigned long long)map_hash, map.name);
            CHECK(map_hash == 0x2f163ae2e768c7e8ULL);
            WG_MapFree(&map);
        }
        for (index = 0; index < maps.header_offset_count; ++index)
        {
            if (maps.header_offsets[index] == 0xffffffffU
                || maps.header_offsets[index] == 0U)
            {
                continue;
            }
            if (!WG_MapsLoad(&maps, index, &map))
            {
                fprintf(stderr, "failed to load map %u\n", (unsigned)index);
                CHECK(0);
            }
            if (map.planes[0] != NULL)
            {
                CHECK(map.width == 64U);
                CHECK(map.height == 64U);
                CHECK(WG_LevelBuild(&map, &level));
                CHECK(level.door_count <= WG_MAX_DOORS);
                for (actor_index = 0; actor_index < level.actor_count;
                     ++actor_index)
                {
                    const wg_actor_t *actor = &level.actors[actor_index];
                    size_t sprite_page =
                        (size_t)data_set.sprite_start + actor->shape;

                    CHECK(actor->actor_class <= WG_ACTOR_FAT);
                    if (actor->actor_class <= WG_ACTOR_FAT)
                    {
                        ++actor_classes[actor->actor_class];
                    }
                    CHECK(sprite_page < data_set.sound_start);
                    if (sprite_page < data_set.sound_start)
                    {
                        CHECK(data_set.pages[sprite_page].offset != 0U);
                        CHECK(data_set.pages[sprite_page].length != 0U);
                    }
                }
                ++loaded_maps;
                WG_MapFree(&map);
            }
        }
        CHECK(loaded_maps == (expected_variant
              == WG_GAME_WOLF3D_SHAREWARE_14 ? 10U : 60U));
        CHECK(actor_classes[WG_ACTOR_BOSS] != 0U);
        if (expected_variant == WG_GAME_WOLF3D_FULL_GT_14)
        {
            CHECK(actor_classes[WG_ACTOR_GHOST] != 0U);
            CHECK(actor_classes[WG_ACTOR_SCHABBS] != 0U);
            CHECK(actor_classes[WG_ACTOR_FAKE] != 0U);
            CHECK(actor_classes[WG_ACTOR_MECHA_HITLER] != 0U);
            CHECK(actor_classes[WG_ACTOR_GRETEL] != 0U);
            CHECK(actor_classes[WG_ACTOR_GIFT] != 0U);
            CHECK(actor_classes[WG_ACTOR_FAT] != 0U);
        }
        WG_MapsClose(&maps);
    }

    CHECK(WG_AudioOpen(&audio, &data_set));
    if (audio.offsets != NULL)
    {
        CHECK(audio.offset_count == 289U);
        CHECK(WG_AudioGetChunk(&audio, 261U, &page_data, &page_size));
        CHECK(page_size > 2U);
        CHECK(!WG_AudioGetChunk(&audio, 288U, &page_data, &page_size));
        for (index = 0; index + 1U < audio.offset_count; ++index)
        {
            CHECK(WG_AudioGetChunk(&audio, index, &page_data, &page_size));
        }
        WG_AudioClose(&audio);
    }
    WG_DataClose(&data_set);
}

int main(int argc, char **argv)
{
    TestHuffman();
    TestCarmack();
    TestRLEW();
    TestMalformedCompression();
    TestVideo();
    TestRandom();
    TestActorSetup();
    TestBossAndGhostSetup();
    TestPatrolMovement();
    TestActorAwareness();
    TestDoorAreaConnectivity();
    TestOrdinaryShootingStates();
    TestDogChaseAndBite();
    TestBossShootingStates();
    TestSchabbsNeedle();
    TestRocketBossAttacks();
    TestViewMath();
    TestWallScaler();
    TestStaticRaycaster();
    TestStaticRenderer();

    if (argc == 4 && strcmp(argv[1], "--data") == 0)
    {
        if (strcmp(argv[2], "wl1") == 0)
        {
            TestDataSet(argv[3], WG_GAME_WOLF3D_SHAREWARE_14, 157);
        }
        else if (strcmp(argv[2], "wl6") == 0)
        {
            TestDataSet(argv[3], WG_GAME_WOLF3D_FULL_GT_14, 150);
        }
        else
        {
            fprintf(stderr, "Unknown test data variant: %s\n", argv[2]);
            return 2;
        }
    }
    else if (argc != 1)
    {
        fprintf(stderr, "usage: wg-tests [--data wl1|wl6 PATH]\n");
        return 2;
    }

    if (failures != 0)
    {
        fprintf(stderr, "%d test check(s) failed.\n", failures);
        return 1;
    }
    return 0;
}
