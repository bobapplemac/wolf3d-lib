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
#include "WL_ACT1.h"
#include "WG_MAPS.h"
#include "ID_PM.h"
#include "ID_SD.h"
#include "WG_PALETTE.h"
#include "ID_US_1.h"
#include "WL_DRAW.h"
#include "WL_INTER.h"
#include "WG_RENDERER.h"
#include "WL_SCALE.h"
#include "WL_STATE.h"
#include "ID_VL.h"
#include "WL_MAIN.h"
#include "WL_PLAY.h"

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

typedef struct opl_write_log
{
    uint16_t registers[16];
    uint8_t values[16];
    size_t count;
} opl_write_log_t;

static void LogOPLWrite(void *user, uint16_t register_number, uint8_t value)
{
    opl_write_log_t *log = (opl_write_log_t *)user;

    if (log->count < sizeof(log->registers) / sizeof(log->registers[0]))
    {
        log->registers[log->count] = register_number;
        log->values[log->count] = value;
        ++log->count;
    }
}

static void TestIMFSequencer(void)
{
    static const uint8_t chunk[] =
    {
        12U, 0U,
        0x20U, 0x01U, 2U, 0U,
        0x40U, 0x02U, 0U, 0U,
        0x60U, 0x03U, 1U, 0U
    };
    static const uint8_t effect_chunk[] =
    {
        3U, 0U, 0U, 0U, 5U, 0U,
        1U, 1U, 2U, 2U, 3U, 3U, 4U, 4U,
        5U, 5U, 0U, 0U, 0U, 0U, 0U, 0U,
        2U, 0x80U, 0U, 0x90U
    };
    static const uint8_t low_priority_effect[] =
    {
        1U, 0U, 0U, 0U, 4U, 0U,
        1U, 1U, 2U, 2U, 3U, 3U, 4U, 4U,
        5U, 5U, 0U, 0U, 0U, 0U, 0U, 0U,
        2U, 0x80U
    };
    id_sd_imf_t sequence;
    id_sd_sample_clock_t clock;
    opl_write_log_t log;
    unsigned index;
    uint32_t ticks = 0U;
    id_sd_music_t *music;
    int16_t effect_pcm[1200U * 2U];
    static const uint8_t digital_samples[] = { 0U, 255U };

    CHECK(WL_MusicChunkForMap(0U) == 264U);
    CHECK(WL_MusicChunkForMap(8U) == 263U);
    CHECK(WL_MusicChunkForMap(9U) == 261U);
    CHECK(WL_MusicChunkForMap(59U) == 276U);
    CHECK(WL_MusicChunkForMap(60U) == 264U);
    CHECK(ID_SD_DigitalNumberForSound(WG_SOUND_ATTACK_PISTOL) == 5);
    CHECK(ID_SD_DigitalNumberForSound(WG_SOUND_ATTACK_KNIFE) == -1);

    memset(&sequence, 0, sizeof(sequence));
    memset(&log, 0, sizeof(log));
    CHECK(!ID_SD_IMFStart(&sequence, chunk, sizeof(chunk) - 1U,
                          LogOPLWrite, &log));
    CHECK(ID_SD_IMFStart(&sequence, chunk, sizeof(chunk), LogOPLWrite, &log));
    ID_SD_IMFService(&sequence);
    CHECK(log.count == 1U && log.registers[0] == 0x20U
          && log.values[0] == 0x01U);
    ID_SD_IMFService(&sequence);
    CHECK(log.count == 1U);
    ID_SD_IMFService(&sequence);
    CHECK(log.count == 3U && log.registers[1] == 0x40U
          && log.values[1] == 0x02U && log.registers[2] == 0x60U
          && log.values[2] == 0x03U);
    CHECK(sequence.position == 0U && sequence.time == 0U);
    ID_SD_IMFService(&sequence);
    CHECK(log.count == 4U && log.registers[3] == 0x20U);
    ID_SD_IMFStop(&sequence);
    ID_SD_IMFService(&sequence);
    CHECK(log.count == 4U);

    CHECK(!ID_SD_SampleClockStart(&clock, 699U));
    CHECK(ID_SD_SampleClockStart(&clock, 48000U));
    for (index = 0U; index < 48000U; ++index)
    {
        ticks += ID_SD_SampleClockAdvance(&clock, 1U);
    }
    CHECK(ticks == ID_SD_IMF_RATE);
    CHECK(clock.phase == 0U);
    CHECK(ID_SD_SampleClockFramesToTick(&clock) == 69U);

    CHECK(ID_SD_SampleClockStart(&clock, 44100U));
    CHECK(ID_SD_SampleClockFramesToTick(&clock) == 63U);
    CHECK(ID_SD_SampleClockAdvance(&clock, 63U) == 1U);
    CHECK(clock.phase == 0U);

    music = ID_SD_MusicCreate(48000U);
    CHECK(music != NULL);
    if (music != NULL)
    {
        CHECK(ID_SD_EffectStart(music, effect_chunk, sizeof(effect_chunk)));
        CHECK(ID_SD_EffectPlaying(music));
        CHECK(!ID_SD_EffectStart(music, low_priority_effect,
                                 sizeof(low_priority_effect)));
        CHECK(ID_SD_MusicRender(music, effect_pcm, 1200U));
        CHECK(!ID_SD_EffectPlaying(music));
        ID_SD_MusicDestroy(music);
    }
    music = ID_SD_MusicCreate(48000U);
    CHECK(music != NULL);
    if (music != NULL)
    {
        memset(effect_pcm, 0, sizeof(effect_pcm));
        CHECK(ID_SD_DigitalStart(music, digital_samples,
                                 sizeof(digital_samples), 5U, 0U, 14U));
        CHECK(!ID_SD_DigitalStart(music, digital_samples,
                                  sizeof(digital_samples), 4U, 0U, 0U));
        CHECK(ID_SD_DigitalSetPosition(music, 3U, 7U));
        CHECK(!ID_SD_DigitalSetPosition(music, 15U, 15U));
        CHECK(ID_SD_MusicRender(music, effect_pcm, 20U));
        CHECK(effect_pcm[0] == -26214);
        CHECK(effect_pcm[1] == -17476);
        CHECK(!ID_SD_DigitalPlaying(music));
        ID_SD_MusicDestroy(music);
    }
}

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

static void TestStaticItemSetup(void)
{
    static const uint16_t item_codes[] =
    {
        29U, 43U, 44U, 47U, 48U, 49U, 50U, 51U,
        52U, 53U, 54U, 55U, 56U, 57U, 61U, 71U
    };
    static const wg_item_type_t expected_items[] =
    {
        WG_ITEM_ALPO, WG_ITEM_KEY1, WG_ITEM_KEY2, WG_ITEM_FOOD,
        WG_ITEM_FIRSTAID, WG_ITEM_CLIP, WG_ITEM_MACHINEGUN,
        WG_ITEM_CHAINGUN, WG_ITEM_CROSS, WG_ITEM_CHALICE, WG_ITEM_BIBLE,
        WG_ITEM_CROWN, WG_ITEM_FULLHEAL, WG_ITEM_GIBS, WG_ITEM_GIBS,
        WG_ITEM_CLIP2
    };
    uint16_t plane_zero[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    uint16_t plane_one[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    wg_map_t map;
    wg_level_t level;
    size_t index;

    memset(&map, 0, sizeof(map));
    for (index = 0U; index < WG_LEVEL_SIZE * WG_LEVEL_SIZE; ++index)
    {
        plane_zero[index] = WG_AREA_TILE;
        plane_one[index] = 0U;
    }
    plane_one[WG_LEVEL_SIZE + 1U] = 19U;
    for (index = 0U; index < sizeof(item_codes) / sizeof(item_codes[0]);
         ++index)
    {
        plane_one[2U * WG_LEVEL_SIZE + 2U + index] = item_codes[index];
    }
    map.width = WG_LEVEL_SIZE;
    map.height = WG_LEVEL_SIZE;
    map.planes[0] = plane_zero;
    map.planes[1] = plane_one;

    CHECK(WG_LevelBuild(&map, &level));
    CHECK(level.static_count == sizeof(item_codes) / sizeof(item_codes[0]));
    CHECK(level.player_lives == 3U);
    CHECK(level.next_extra == 40000U);
    for (index = 0U; index < level.static_count; ++index)
    {
        CHECK(level.statics[index].item == expected_items[index]);
        CHECK(level.statics[index].removed == 0U);
        CHECK(level.statics[index].shape
              == (item_codes[index] == 71U ? 28U
                                           : item_codes[index] - 21U));
    }
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
    CHECK(level.actors[5].attack_shape == 325U);
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
    CHECK(level.sound_event_count == 1U);
    CHECK(level.sound_events[0].sound == WG_SOUND_GUARD_SIGHT);
    CHECK(level.sound_events[0].positioned == 1U);
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
    CHECK(level.tiles[2U * WG_LEVEL_SIZE + 2U] == 0U);
    CHECK(level.ambush_tiles[2U * WG_LEVEL_SIZE + 2U] == 1U);

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
    CHECK(level.areas[2U * WG_LEVEL_SIZE + 3U] == 0U);
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
    CHECK(level.doors[0].action == WG_DOOR_OPENING);
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
    CHECK(level.sound_event_count == 1U);
    CHECK(level.sound_events[0].sound == WG_SOUND_NAZI_FIRE);
    CHECK(level.sound_events[0].positioned == 1U);

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
    CHECK(level.sound_event_count == 1U);
    CHECK(level.sound_events[0].sound == WG_SOUND_DOG_BARK);
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
    CHECK(level.sound_event_count == 2U);
    CHECK(level.sound_events[1].sound == WG_SOUND_DOG_ATTACK);
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

static void TestFakeHitlerFlames(void)
{
    wg_level_t level;
    wg_actor_t *fake;
    wg_actor_t *fire;
    unsigned stage;

    memset(&level, 0, sizeof(level));
    level.player_x = 5 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_y = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_tile_x = 5U;
    level.player_tile_y = 2U;
    level.player_health = 100U;
    level.difficulty = WG_DIFFICULTY_HARD;
    level.actor_count = 1U;
    fake = &level.actors[0];
    fake->x = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    fake->y = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    fake->tile_x = 2U;
    fake->tile_y = 2U;
    fake->area_number = 0U;
    fake->direction = 0U;
    fake->flags = WG_ACTOR_FLAG_SHOOTABLE | WG_ACTOR_FLAG_ATTACK_MODE;
    fake->actor_class = WG_ACTOR_FAKE;
    fake->base_shape = 321U;
    fake->attack_shape = 325U;
    fake->state = WG_STATE_SHOOT1;
    fake->tic_count = 8;
    fake->shape = 325U;
    fake->speed = 1536;
    WG_RandomSeed(&level.random, 0U);

    CHECK(WL_TickActors(&level, 8U));
    CHECK(fake->state == WG_STATE_SHOOT2);
    CHECK(fake->tic_count == 8);
    CHECK(fake->shape == 325U);
    CHECK(level.actor_count == 2U);
    fire = &level.actors[1];
    CHECK(fire->actor_class == WG_ACTOR_FIRE);
    CHECK(fire->state == WG_STATE_FIRE1);
    CHECK(fire->shape == 326U);
    CHECK(fire->angle == 0U);
    CHECK(fire->speed == 0x1200);
    CHECK(fire->tic_count == 5);
    CHECK(fire->x == 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2 + 0x9000);

    for (stage = 2U; stage <= 8U; ++stage)
    {
        fire->flags |= WG_ACTOR_FLAG_REMOVED;
        fake->tic_count = 1;
        CHECK(WL_TickActors(&level, 1U));
        CHECK(fake->state == (wg_actor_state_t)(WG_STATE_SHOOT1 + stage));
        CHECK(fake->shape == 325U);
        fire = &level.actors[1];
        CHECK(fire->actor_class == WG_ACTOR_FIRE);
        CHECK((fire->flags & WG_ACTOR_FLAG_REMOVED) == 0U);
    }
    fire->flags |= WG_ACTOR_FLAG_REMOVED;
    fake->tic_count = 1;
    CHECK(WL_TickActors(&level, 1U));
    CHECK(fake->state == WG_STATE_CHASE1);
    CHECK((fire->flags & WG_ACTOR_FLAG_REMOVED) != 0U);

    memset(fire, 0, sizeof(*fire));
    fake->flags |= WG_ACTOR_FLAG_REMOVED;
    fire->x = level.player_x - 0xc000;
    fire->y = level.player_y;
    fire->tile_x = 4U;
    fire->tile_y = 2U;
    fire->angle = 0U;
    fire->shape = 326U;
    fire->base_shape = 326U;
    fire->tic_count = 6;
    fire->speed = 0x1200;
    fire->state = WG_STATE_FIRE1;
    fire->actor_class = WG_ACTOR_FIRE;
    WG_RandomSeed(&level.random, 0U);
    CHECK(WL_TickActors(&level, 1U));
    CHECK((fire->flags & WG_ACTOR_FLAG_REMOVED) != 0U);
    CHECK(level.player_health == 99U);

    memset(fire, 0, sizeof(*fire));
    fire->x = 3 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    fire->y = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    fire->tile_x = 3U;
    fire->tile_y = 2U;
    fire->angle = 0U;
    fire->shape = 326U;
    fire->base_shape = 326U;
    fire->tic_count = 6;
    fire->speed = 0x1200;
    fire->state = WG_STATE_FIRE1;
    fire->actor_class = WG_ACTOR_FIRE;
    level.tiles[2U * WG_LEVEL_SIZE + 4U] = 1U;
    level.player_x = 10 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_tile_x = 10U;
    CHECK(WL_TickActors(&level, 6U));
    CHECK((fire->flags & WG_ACTOR_FLAG_REMOVED) != 0U);

    memset(&level, 0, sizeof(level));
    level.player_x = 5 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_y = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_tile_x = 5U;
    level.player_tile_y = 2U;
    level.player_health = 100U;
    level.actor_count = 1U;
    fake = &level.actors[0];
    fake->x = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    fake->y = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    fake->tile_x = 2U;
    fake->tile_y = 2U;
    fake->area_number = 0U;
    fake->direction = 0U;
    fake->flags = WG_ACTOR_FLAG_SHOOTABLE | WG_ACTOR_FLAG_ATTACK_MODE;
    fake->actor_class = WG_ACTOR_FAKE;
    fake->base_shape = 321U;
    fake->attack_shape = 325U;
    fake->state = WG_STATE_CHASE1;
    fake->tic_count = 10;
    fake->shape = 321U;
    fake->speed = 1536;
    fake->distance = WG_FIXED_ONE;
    WG_RandomSeed(&level.random, 0U);
    CHECK(WL_TickActors(&level, 4U));
    CHECK(fake->state == WG_STATE_CHASE1);
    fake->x = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    fake->y = 2 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    fake->tile_x = 2U;
    fake->tile_y = 2U;
    fake->direction = 0U;
    fake->distance = WG_FIXED_ONE;
    fake->tic_count = 10;
    WG_RandomSeed(&level.random, 0U);
    CHECK(WL_TickActors(&level, 5U));
    CHECK(fake->state == WG_STATE_SHOOT1);
    CHECK(fake->tic_count == 8);
    CHECK(fake->shape == 325U);
}

static void TestActorDamageAndDeath(void)
{
    wg_level_t level;
    wg_actor_t *actor;

    memset(&level, 0, sizeof(level));
    level.difficulty = WG_DIFFICULTY_MEDIUM;
    level.player_best_weapon = 1U;

    CHECK(WL_SpawnStand(&level, WG_ACTOR_GUARD, 10U, 10U, 0U));
    actor = &level.actors[0];
    CHECK(actor->hit_points == 25);
    CHECK(WL_DamageActor(&level, 0U, 4U));
    CHECK(level.made_noise == 1U);
    CHECK(actor->hit_points == 17);
    CHECK((actor->flags & WG_ACTOR_FLAG_ATTACK_MODE) != 0U);
    CHECK(actor->state == WG_STATE_PAIN1);
    CHECK(actor->shape == 90U);
    CHECK(actor->rotate == 2U);
    CHECK(WL_TickActors(&level, 10U));
    CHECK(actor->state == WG_STATE_CHASE1);
    CHECK(actor->shape == actor->base_shape);
    CHECK(actor->rotate == 1U);

    CHECK(WL_DamageActor(&level, 0U, 17U));
    CHECK(actor->hit_points == 0);
    CHECK(actor->state == WG_STATE_DIE1);
    CHECK(actor->shape == 91U);
    CHECK((actor->flags & WG_ACTOR_FLAG_SHOOTABLE) == 0U);
    CHECK((actor->flags & WG_ACTOR_FLAG_NONMARK) != 0U);
    CHECK(level.score == 100U);
    CHECK(level.kill_count == 1U);
    CHECK(level.static_count == 1U);
    CHECK(level.statics[0].shape == 28U);
    CHECK(level.statics[0].blocking == 0U);
    CHECK(level.statics[0].item == WG_ITEM_CLIP2);
    CHECK(!WL_DamageActor(&level, 0U, 1U));
    CHECK(WL_TickActors(&level, 15U));
    CHECK(actor->state == WG_STATE_DIE2 && actor->shape == 92U);
    CHECK(WL_TickActors(&level, 15U));
    CHECK(actor->state == WG_STATE_DIE3 && actor->shape == 93U);
    CHECK(WL_TickActors(&level, 15U));
    CHECK(actor->state == WG_STATE_DEAD && actor->shape == 95U);

    CHECK(WL_SpawnStand(&level, WG_ACTOR_DOG, 11U, 10U, 0U));
    actor = &level.actors[1];
    CHECK(actor->hit_points == 1);
    CHECK(WL_DamageActor(&level, 1U, 1U));
    CHECK(level.score == 300U);
    CHECK(level.kill_count == 2U);
    CHECK(level.static_count == 1U);
    CHECK(WL_TickActors(&level, 45U));
    CHECK(actor->state == WG_STATE_DEAD && actor->shape == 134U);
    CHECK(actor->tic_count == 15);

    CHECK(WL_SpawnStand(&level, WG_ACTOR_SS, 12U, 10U, 0U));
    actor = &level.actors[2];
    CHECK(actor->hit_points == 100);
    CHECK(WL_DamageActor(&level, 2U, 50U));
    CHECK(level.statics[1].shape == 29U);
    CHECK(level.statics[1].item == WG_ITEM_MACHINEGUN);
    CHECK(level.score == 800U);
    CHECK(WL_TickActors(&level, 45U));
    CHECK(actor->state == WG_STATE_DEAD && actor->shape == 183U);

    level.difficulty = WG_DIFFICULTY_HARD;
    CHECK(WL_SpawnStand(&level, WG_ACTOR_MUTANT, 13U, 10U, 0U));
    CHECK(level.actors[3].hit_points == 65);
    CHECK(WL_DamageActor(&level, 3U, 5U));
    CHECK(level.actors[3].hit_points == 55);
    CHECK(level.actors[3].state == WG_STATE_PAIN1);
    CHECK(level.actors[3].shape == 227U);
    CHECK(WL_DamageActor(&level, 3U, 55U));
    CHECK(WL_TickActors(&level, 28U));
    CHECK(level.actors[3].state == WG_STATE_DEAD);
    CHECK(level.actors[3].shape == 233U);
    CHECK(level.score == 1500U);

    CHECK(WL_SpawnStand(&level, WG_ACTOR_OFFICER, 14U, 10U, 0U));
    CHECK(level.actors[4].hit_points == 50);
    CHECK(WL_DamageActor(&level, 4U, 25U));
    CHECK(WL_TickActors(&level, 44U));
    CHECK(level.actors[4].state == WG_STATE_DEAD);
    CHECK(level.actors[4].shape == 284U);
    CHECK(level.score == 1900U);
    CHECK(level.kill_count == 5U);
    CHECK(level.static_count == 4U);
    CHECK(level.statics[3].shape == 28U);
    CHECK(level.statics[3].item == WG_ITEM_CLIP2);

    level.player_best_weapon = 2U;
    CHECK(WL_SpawnStand(&level, WG_ACTOR_SS, 15U, 10U, 0U));
    CHECK(WL_DamageActor(&level, 5U, 50U));
    CHECK(level.statics[4].shape == 28U);
    CHECK(level.statics[4].item == WG_ITEM_CLIP2);
    CHECK(level.score == 2400U);
}

static void TestBossDamageAndDeath(void)
{
    static const struct boss_death_case
    {
        wg_actor_class_t actor_class;
        int32_t hit_points;
        unsigned death_tics;
        uint16_t first_shape;
        uint16_t dead_shape;
        uint32_t points;
        int drops_key;
        int uses_death_cam;
    } cases[] =
    {
        {WG_ACTOR_BOSS, 850, 45U, 304U, 303U, 5000U, 1, 0},
        {WG_ACTOR_GRETEL, 850, 45U, 393U, 392U, 5000U, 1, 0},
        {WG_ACTOR_FAKE, 200, 50U, 328U, 333U, 2000U, 0, 0},
        {WG_ACTOR_SCHABBS, 850, 45U, 307U, 316U, 5000U, 0, 1},
        {WG_ACTOR_GIFT, 850, 36U, 360U, 369U, 5000U, 0, 1},
        {WG_ACTOR_FAT, 850, 36U, 396U, 407U, 5000U, 0, 1}
    };
    size_t index;

    for (index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index)
    {
        wg_level_t level;
        wg_actor_t *actor;

        memset(&level, 0, sizeof(level));
        level.difficulty = WG_DIFFICULTY_BABY;
        level.player_x = 7 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
        level.player_y = 8 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
        CHECK(WL_SpawnBoss(&level, cases[index].actor_class,
                           10U, 10U));
        actor = &level.actors[0];
        CHECK(actor->hit_points == cases[index].hit_points);
        CHECK(WL_DamageActor(&level, 0U,
                             (unsigned)cases[index].hit_points / 2U));
        CHECK(actor->state == WG_STATE_DIE1);
        CHECK(actor->shape == cases[index].first_shape);
        CHECK(level.score == cases[index].points);
        CHECK(level.kill_count == 1U);
        CHECK(level.static_count == (uint16_t)cases[index].drops_key);
        if (cases[index].drops_key)
        {
            CHECK(level.statics[0].shape == 22U);
            CHECK(level.statics[0].item == WG_ITEM_KEY1);
        }
        if (cases[index].uses_death_cam)
        {
            CHECK(level.kill_x == level.player_x);
            CHECK(level.kill_y == level.player_y);
        }
        CHECK(WL_TickActors(&level, cases[index].death_tics));
        CHECK(actor->state == WG_STATE_DEAD);
        CHECK(actor->shape == cases[index].dead_shape);
        CHECK(level.victory_flag == 0U);
        if (cases[index].uses_death_cam)
        {
            CHECK(actor->tic_count == 20);
            CHECK(WL_TickActors(&level, 20U));
            CHECK(level.victory_flag == 1U);
            CHECK(level.level_completed == 0U);
            CHECK(WL_TickActors(&level, 20U));
            CHECK(level.level_completed == 1U);
        }
        else
        {
            CHECK(actor->tic_count == 0);
        }
    }

    {
        wg_level_t level;
        wg_actor_t *mecha;
        wg_actor_t *hitler;

        memset(&level, 0, sizeof(level));
        level.difficulty = WG_DIFFICULTY_BABY;
        WG_RandomSeed(&level.random, 0U);
        CHECK(WL_SpawnBoss(&level, WG_ACTOR_MECHA_HITLER, 10U, 10U));
        mecha = &level.actors[0];
        CHECK(mecha->hit_points == 800);
        CHECK(WL_DamageActor(&level, 0U, 400U));
        CHECK(mecha->shape == 342U);
        CHECK(WL_TickActors(&level, 30U));
        CHECK(mecha->state == WG_STATE_DEAD);
        CHECK(mecha->shape == 341U);
        CHECK(level.actor_count == 2U);
        hitler = &level.actors[1];
        CHECK(hitler->actor_class == WG_ACTOR_REAL_HITLER);
        CHECK(hitler->hit_points == 500);
        CHECK(hitler->base_shape == 345U);
        CHECK(hitler->attack_shape == 349U);
        CHECK(hitler->speed == 2560);
        CHECK((hitler->flags & WG_ACTOR_FLAG_SHOOTABLE) != 0U);
        CHECK(WL_DamageActor(&level, 1U, 250U));
        CHECK(level.score == 10000U);
        CHECK(level.kill_count == 2U);
        CHECK(WL_TickActors(&level, 76U));
        CHECK(hitler->state == WG_STATE_DEAD);
        CHECK(hitler->shape == 352U);
        CHECK(hitler->tic_count == 20);
        CHECK(WL_TickActors(&level, 20U));
        CHECK(level.victory_flag == 1U);
    }
}

static void TestPlayerWeapons(void)
{
    wg_level_t level;
    wg_actor_t *actor;

    memset(&level, 0, sizeof(level));
    level.difficulty = WG_DIFFICULTY_MEDIUM;
    level.player_x = 10 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_y = 10 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_tile_x = 10U;
    level.player_tile_y = 10U;
    level.player_weapon = WG_WEAPON_PISTOL;
    CHECK(WL_SpawnStand(&level, WG_ACTOR_GUARD, 11U, 10U, 0U));
    actor = &level.actors[0];
    actor->flags |= WG_ACTOR_FLAG_VISIBLE;
    actor->view_x = WG_VIDEO_WIDTH / 2 - 1;
    actor->trans_x = WG_FIXED_ONE;
    WG_RandomSeed(&level.random, 1U);
    CHECK(WL_GunAttack(&level));
    CHECK(level.sound_event_count == 2U);
    CHECK(level.sound_events[0].sound == WG_SOUND_ATTACK_PISTOL);
    CHECK(level.sound_events[0].positioned == 0U);
    CHECK(level.sound_events[1].sound == WG_SOUND_DEATH_SCREAM_5);
    CHECK(level.sound_events[1].positioned == 1U);
    CHECK(level.made_noise == 1U);
    CHECK(actor->state == WG_STATE_DIE1);
    CHECK(level.score == 100U);

    memset(&level, 0, sizeof(level));
    level.difficulty = WG_DIFFICULTY_MEDIUM;
    level.shareware = 1U;
    CHECK(WL_SpawnStand(&level, WG_ACTOR_GUARD, 11U, 10U, 0U));
    WG_RandomSeed(&level.random, 1U);
    CHECK(WL_KillActor(&level, 0U));
    CHECK(level.sound_event_count == 1U);
    CHECK(level.sound_events[0].sound == WG_SOUND_DEATH_SCREAM_2);

    memset(&level, 0, sizeof(level));
    level.difficulty = WG_DIFFICULTY_MEDIUM;
    level.player_x = 10 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_y = 10 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_tile_x = 10U;
    level.player_tile_y = 10U;
    CHECK(WL_SpawnStand(&level, WG_ACTOR_GUARD, 11U, 10U, 0U));
    CHECK(WL_SpawnStand(&level, WG_ACTOR_OFFICER, 12U, 10U, 0U));
    level.actors[0].flags |= WG_ACTOR_FLAG_VISIBLE;
    level.actors[0].view_x = WG_VIDEO_WIDTH / 2 - 1;
    level.actors[0].trans_x = 2 * WG_FIXED_ONE;
    level.actors[1].flags |= WG_ACTOR_FLAG_VISIBLE;
    level.actors[1].view_x = WG_VIDEO_WIDTH / 2 - 1;
    level.actors[1].trans_x = WG_FIXED_ONE;
    WG_RandomSeed(&level.random, 1U);
    CHECK(WL_GunAttack(&level));
    CHECK(level.actors[0].hit_points == 25);
    CHECK(level.actors[1].hit_points == 14);

    memset(&level, 0, sizeof(level));
    level.difficulty = WG_DIFFICULTY_MEDIUM;
    level.player_x = 10 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_y = 10 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_tile_x = 10U;
    level.player_tile_y = 10U;
    CHECK(WL_SpawnStand(&level, WG_ACTOR_GUARD, 13U, 10U, 0U));
    actor = &level.actors[0];
    actor->flags |= WG_ACTOR_FLAG_VISIBLE;
    actor->view_x = WG_VIDEO_WIDTH / 2 - 1;
    actor->trans_x = 3 * WG_FIXED_ONE;
    level.tiles[10U * WG_LEVEL_SIZE + 12U] = 1U;
    WG_RandomSeed(&level.random, 0U);
    CHECK(WL_GunAttack(&level));
    CHECK(actor->hit_points == 25);
    CHECK(level.random.index == 0U);

    level.tiles[10U * WG_LEVEL_SIZE + 12U] = 0U;
    actor->tile_x = 15U;
    actor->x = 15 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    actor->trans_x = 5 * WG_FIXED_ONE;
    WG_RandomSeed(&level.random, 14U);
    CHECK(WL_GunAttack(&level));
    CHECK(actor->hit_points == 25);
    CHECK(level.random.index == 15U);

    actor->tile_x = 11U;
    actor->x = 11 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    actor->trans_x = WG_FIXED_ONE;
    WG_RandomSeed(&level.random, 1U);
    CHECK(WL_KnifeAttack(&level));
    CHECK(actor->hit_points == 13);
    CHECK(actor->state == WG_STATE_PAIN1);
    actor->hit_points = 25;
    actor->flags |= WG_ACTOR_FLAG_SHOOTABLE | WG_ACTOR_FLAG_VISIBLE;
    actor->trans_x = INT32_C(0x18001);
    WG_RandomSeed(&level.random, 0U);
    CHECK(WL_KnifeAttack(&level));
    CHECK(actor->hit_points == 25);
    CHECK(level.random.index == 0U);

    memset(&level, 0, sizeof(level));
    level.player_health = 100U;
    level.player_ammo = 1U;
    level.player_weapon = WG_WEAPON_PISTOL;
    level.player_chosen_weapon = WG_WEAPON_PISTOL;
    CHECK(WL_StartAttack(&level));
    CHECK(level.attack_active == 1U);
    CHECK(level.attack_count == 6);
    CHECK(level.weapon_frame == 1U);
    CHECK(WL_TickPlayerAttack(&level, 6U, 0));
    CHECK(level.attack_frame == 1U);
    CHECK(level.weapon_frame == 2U);
    CHECK(level.player_ammo == 1U);
    CHECK(WL_TickPlayerAttack(&level, 6U, 0));
    CHECK(level.player_ammo == 0U);
    CHECK(level.attack_frame == 2U);
    CHECK(level.weapon_frame == 3U);
    CHECK(WL_TickPlayerAttack(&level, 12U, 0));
    CHECK(level.attack_active == 0U);
    CHECK(level.player_weapon == WG_WEAPON_KNIFE);
    CHECK(level.weapon_frame == 0U);

    memset(&level, 0, sizeof(level));
    level.player_health = 100U;
    level.player_ammo = 3U;
    level.player_weapon = WG_WEAPON_MACHINEGUN;
    level.player_chosen_weapon = WG_WEAPON_MACHINEGUN;
    CHECK(WL_StartAttack(&level));
    CHECK(WL_TickPlayerAttack(&level, 12U, 1));
    CHECK(level.player_ammo == 2U);
    CHECK(WL_TickPlayerAttack(&level, 6U, 1));
    CHECK(level.attack_frame == 1U);
    CHECK(WL_TickPlayerAttack(&level, 6U, 1));
    CHECK(level.player_ammo == 1U);

    memset(&level, 0, sizeof(level));
    level.player_health = 100U;
    level.player_ammo = 3U;
    level.player_weapon = WG_WEAPON_CHAINGUN;
    level.player_chosen_weapon = WG_WEAPON_CHAINGUN;
    CHECK(WL_StartAttack(&level));
    CHECK(WL_TickPlayerAttack(&level, 18U, 1));
    CHECK(level.player_ammo == 1U);
    CHECK(level.attack_frame == 1U);
}

static void SetBonus(wg_level_t *level, wg_item_type_t item)
{
    memset(level, 0, sizeof(*level));
    level->static_count = 1U;
    level->statics[0].item = item;
    level->statics[0].blocking = 1U;
    level->player_health = 100U;
    level->player_ammo = 8U;
    level->player_lives = 3U;
    level->player_weapon = WG_WEAPON_PISTOL;
    level->player_chosen_weapon = WG_WEAPON_PISTOL;
    level->player_best_weapon = WG_WEAPON_PISTOL;
    level->next_extra = 40000U;
}

static void TestBonusPickups(void)
{
    wg_level_t level;

    SetBonus(&level, WG_ITEM_FIRSTAID);
    CHECK(!WL_GetBonus(&level, 0U));
    CHECK(level.statics[0].removed == 0U);
    level.player_health = 80U;
    CHECK(WL_GetBonus(&level, 0U));
    CHECK(level.player_health == 100U);
    CHECK(level.sound_event_count == 1U);
    CHECK(level.sound_events[0].sound == WG_SOUND_HEALTH_2);

    SetBonus(&level, WG_ITEM_FOOD);
    level.player_health = 95U;
    CHECK(WL_GetBonus(&level, 0U));
    CHECK(level.player_health == 100U);
    SetBonus(&level, WG_ITEM_ALPO);
    level.player_health = 50U;
    CHECK(WL_GetBonus(&level, 0U));
    CHECK(level.player_health == 54U);
    SetBonus(&level, WG_ITEM_GIBS);
    level.player_health = 11U;
    CHECK(!WL_GetBonus(&level, 0U));
    level.player_health = 10U;
    CHECK(WL_GetBonus(&level, 0U));
    CHECK(level.player_health == 11U);

    SetBonus(&level, WG_ITEM_CLIP);
    level.player_ammo = 95U;
    CHECK(WL_GetBonus(&level, 0U));
    CHECK(level.player_ammo == 99U);
    CHECK(!WL_GetBonus(&level, 0U));
    SetBonus(&level, WG_ITEM_CLIP2);
    level.player_ammo = 0U;
    level.player_weapon = WG_WEAPON_KNIFE;
    CHECK(WL_GetBonus(&level, 0U));
    CHECK(level.player_ammo == 4U);
    CHECK(level.player_weapon == WG_WEAPON_PISTOL);
    SetBonus(&level, WG_ITEM_CLIP);
    level.player_ammo = 99U;
    CHECK(!WL_GetBonus(&level, 0U));

    SetBonus(&level, WG_ITEM_MACHINEGUN);
    CHECK(WL_GetBonus(&level, 0U));
    CHECK(level.player_ammo == 14U);
    CHECK(level.player_weapon == WG_WEAPON_MACHINEGUN);
    CHECK(level.player_best_weapon == WG_WEAPON_MACHINEGUN);
    CHECK(level.sound_events[0].sound == WG_SOUND_GET_MACHINEGUN);
    SetBonus(&level, WG_ITEM_CHAINGUN);
    CHECK(WL_GetBonus(&level, 0U));
    CHECK(level.player_weapon == WG_WEAPON_CHAINGUN);
    CHECK(level.player_best_weapon == WG_WEAPON_CHAINGUN);

    SetBonus(&level, WG_ITEM_KEY1);
    CHECK(WL_GetBonus(&level, 0U));
    CHECK(level.player_keys == 1U);
    CHECK(level.sound_events[0].sound == WG_SOUND_GET_KEY);
    SetBonus(&level, WG_ITEM_KEY2);
    level.player_keys = 1U;
    CHECK(WL_GetBonus(&level, 0U));
    CHECK(level.player_keys == 3U);

    SetBonus(&level, WG_ITEM_CROSS);
    CHECK(WL_GetBonus(&level, 0U));
    CHECK(level.score == 100U && level.treasure_count == 1U);
    CHECK(level.sound_events[0].sound == WG_SOUND_BONUS_1);
    SetBonus(&level, WG_ITEM_CHALICE);
    CHECK(WL_GetBonus(&level, 0U));
    CHECK(level.score == 500U && level.treasure_count == 1U);
    SetBonus(&level, WG_ITEM_BIBLE);
    CHECK(WL_GetBonus(&level, 0U));
    CHECK(level.score == 1000U && level.treasure_count == 1U);
    SetBonus(&level, WG_ITEM_CROWN);
    CHECK(WL_GetBonus(&level, 0U));
    CHECK(level.score == 5000U && level.treasure_count == 1U);

    SetBonus(&level, WG_ITEM_FULLHEAL);
    level.player_health = 1U;
    level.player_ammo = 80U;
    CHECK(WL_GetBonus(&level, 0U));
    CHECK(level.player_health == 100U);
    CHECK(level.player_ammo == 99U);
    CHECK(level.player_lives == 4U);
    CHECK(level.treasure_count == 1U);
    CHECK(level.bonus_count == 18U);
    CHECK(level.statics[0].removed == 1U);
    CHECK(level.statics[0].blocking == 0U);

    memset(&level, 0, sizeof(level));
    level.player_lives = 3U;
    level.next_extra = 40000U;
    WL_GivePoints(&level, 85000U);
    CHECK(level.score == 85000U);
    CHECK(level.next_extra == 120000U);
    CHECK(level.player_lives == 5U);

    SetBonus(&level, WG_ITEM_CROSS);
    level.statics[0].tile_x = 7U;
    level.statics[0].tile_y = 8U;
    level.player_tile_x = 7U;
    level.player_tile_y = 8U;
    CHECK(WL_CollectPlayerTileBonuses(&level) == 1U);
    CHECK(WL_CollectPlayerTileBonuses(&level) == 0U);
    CHECK(level.score == 100U);
}

static void SetPlayerMovementLevel(wg_level_t *level)
{
    memset(level, 0, sizeof(*level));
    level->player_x = 10 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level->player_y = 10 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level->player_tile_x = 10U;
    level->player_tile_y = 10U;
    level->player_health = 100U;
    level->player_ammo = 8U;
    level->player_lives = 3U;
    level->player_weapon = WG_WEAPON_PISTOL;
    level->player_chosen_weapon = WG_WEAPON_PISTOL;
    level->player_best_weapon = WG_WEAPON_PISTOL;
    level->next_extra = 40000U;
}

static void TestPlayerMovementAndUse(void)
{
    wg_level_t level;
    wg_level_t next_level;
    wg_campaign_state_t campaign;
    wg_view_tables_t tables;
    int32_t start_x;
    int32_t start_y;
    uint8_t left;
    uint8_t right;

    CHECK(WG_NextMapNumber(0U, 0) == 1U);
    CHECK(WG_NextMapNumber(0U, 1) == 9U);
    CHECK(WG_NextMapNumber(9U, 0) == 1U);
    CHECK(WG_NextMapNumber(19U, 0) == 11U);
    CHECK(WG_NextMapNumber(29U, 0) == 27U);
    CHECK(WG_NextMapNumber(59U, 0) == 53U);

    SetPlayerMovementLevel(&level);
    level.score = 12345U;
    level.next_extra = 80000U;
    level.player_health = 47U;
    level.player_ammo = 63U;
    level.player_lives = 2U;
    level.player_weapon = WG_WEAPON_CHAINGUN;
    level.player_chosen_weapon = WG_WEAPON_CHAINGUN;
    level.player_best_weapon = WG_WEAPON_CHAINGUN;
    WG_CampaignCapture(&campaign, &level);
    memset(&next_level, 0, sizeof(next_level));
    next_level.player_keys = 3U;
    CHECK(WG_CampaignApply(&next_level, &campaign, 10000U, 0));
    CHECK(next_level.score == 12345U && next_level.player_health == 47U);
    CHECK(next_level.player_ammo == 63U && next_level.player_lives == 2U);
    CHECK(next_level.player_best_weapon == WG_WEAPON_CHAINGUN);
    CHECK(next_level.player_keys == 0U);
    memset(&next_level, 0, sizeof(next_level));
    CHECK(WG_CampaignApply(&next_level, &campaign, 10000U, 1));
    CHECK(next_level.score == 10000U && next_level.player_health == 100U);
    CHECK(next_level.player_ammo == 8U && next_level.player_lives == 1U);
    CHECK(next_level.player_best_weapon == WG_WEAPON_PISTOL);
    campaign.lives = 0U;
    CHECK(!WG_CampaignApply(&next_level, &campaign, 10000U, 1));

    memset(&tables, 0, sizeof(tables));
    WG_ViewBuildTrigTables(&tables);
    CHECK(WG_ViewCalculateProjection(&tables, WG_MAX_VIEW_WIDTH,
                                     WG_FOCAL_LENGTH));

    SetPlayerMovementLevel(&level);
    CHECK(WG_SoundPosition(&level, &tables, level.player_x, level.player_y,
                           &left, &right));
    CHECK(left == 0U && right == 0U);
    CHECK(WG_SoundPosition(&level, &tables, level.player_x,
                           level.player_y - 2 * WG_FIXED_ONE,
                           &left, &right));
    CHECK(left < right);
    CHECK(WG_SoundPosition(&level, &tables, level.player_x,
                           level.player_y + 2 * WG_FIXED_ONE,
                           &left, &right));
    CHECK(left > right);
    start_x = level.player_x;
    start_y = level.player_y;
    level.tiles[10U * WG_LEVEL_SIZE + 11U] = 1U;
    CHECK(!WL_TryMove(&level, start_x + WG_MIN_DISTANCE * 2,
                      start_y));
    CHECK(WL_Thrust(&level, &tables, 0U, WG_MIN_DISTANCE * 2));
    CHECK(level.player_x == start_x && level.player_y == start_y);
    CHECK(level.player_thrust_speed == WG_MIN_DISTANCE * 2);

    SetPlayerMovementLevel(&level);
    start_x = level.player_x;
    start_y = level.player_y;
    level.tiles[11U * WG_LEVEL_SIZE + 11U] = 1U;
    CHECK(WL_Thrust(&level, &tables, 315U, WG_MIN_DISTANCE * 2));
    CHECK(level.player_x > start_x);
    CHECK(level.player_y == start_y);

    SetPlayerMovementLevel(&level);
    level.static_count = 1U;
    level.statics[0].tile_x = 11U;
    level.statics[0].tile_y = 10U;
    level.statics[0].blocking = 1U;
    CHECK(!WL_TryMove(&level, 11 * WG_FIXED_ONE, level.player_y));
    level.statics[0].removed = 1U;
    CHECK(WL_TryMove(&level, 11 * WG_FIXED_ONE, level.player_y));

    SetPlayerMovementLevel(&level);
    CHECK(WL_SpawnStand(&level, WG_ACTOR_GUARD, 11U, 10U, 0U));
    CHECK(!WL_TryMove(&level, 11 * WG_FIXED_ONE, level.player_y));
    level.actors[0].flags = (uint16_t)(level.actors[0].flags
                                       & ~WG_ACTOR_FLAG_SHOOTABLE);
    CHECK(WL_TryMove(&level, 11 * WG_FIXED_ONE, level.player_y));

    SetPlayerMovementLevel(&level);
    level.static_count = 1U;
    level.statics[0].tile_x = 11U;
    level.statics[0].tile_y = 10U;
    level.statics[0].item = WG_ITEM_CROSS;
    CHECK(WL_Thrust(&level, &tables, 0U, WG_MIN_DISTANCE * 2));
    CHECK(level.player_tile_x == 11U);
    CHECK(level.statics[0].removed == 1U);
    CHECK(level.score == 100U);

    SetPlayerMovementLevel(&level);
    level.info[10U * WG_LEVEL_SIZE + 11U] = 99U;
    CHECK(WL_Thrust(&level, &tables, 0U, WG_MIN_DISTANCE * 2));
    CHECK(level.victory_flag == 1U);

    SetPlayerMovementLevel(&level);
    CHECK(WL_ControlMovement(&level, &tables, 20, 0, 0));
    CHECK(level.player_angle == 359U);
    CHECK(level.player_angle_fraction == 0);
    CHECK(WL_ControlMovement(&level, &tables, 0, -10, 0));
    CHECK(level.player_thrust_speed == 1500);
    CHECK(level.player_x > 10 * WG_FIXED_ONE + WG_FIXED_ONE / 2);

    SetPlayerMovementLevel(&level);
    level.door_count = 1U;
    level.doors[0].tile_x = 11U;
    level.doors[0].tile_y = 10U;
    level.doors[0].vertical = 1U;
    level.doors[0].lock = WG_DOOR_LOCK_1;
    level.doors[0].action = WG_DOOR_CLOSED;
    level.tiles[10U * WG_LEVEL_SIZE + 11U] = 0x80U;
    level.areas[10U * WG_LEVEL_SIZE + 10U] = 0U;
    level.areas[10U * WG_LEVEL_SIZE + 12U] = 1U;
    CHECK(!WL_CmdUse(&level));
    CHECK(level.doors[0].action == WG_DOOR_CLOSED);
    level.player_keys = 1U;
    CHECK(WL_CmdUse(&level));
    CHECK(level.doors[0].action == WG_DOOR_OPENING);
    CHECK(WL_MoveDoors(&level, 32U));
    CHECK(level.sound_event_count == 2U);
    CHECK(level.sound_events[0].sound == WG_SOUND_NOWAY);
    CHECK(level.sound_events[0].positioned == 0U);
    CHECK(level.sound_events[1].sound == WG_SOUND_OPEN_DOOR);
    CHECK(level.sound_events[1].positioned == 1U);
    CHECK(level.doors[0].position == 32768U);
    CHECK(!WL_TryMove(&level, 11 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                      level.player_y));
    CHECK(WL_UpdateAreaConnectivity(&level));
    CHECK(level.area_by_player[0] != 0U);
    CHECK(level.area_by_player[1] != 0U);
    CHECK(WL_MoveDoors(&level, 32U));
    CHECK(level.doors[0].position == 0xffffU);
    CHECK(level.doors[0].action == WG_DOOR_OPEN);
    CHECK(WL_TryMove(&level, 11 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                     level.player_y));
    level.player_x = 8 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_tile_x = 8U;
    CHECK(WL_MoveDoors(&level, 300U));
    CHECK(level.doors[0].action == WG_DOOR_CLOSING);
    CHECK(level.sound_event_count == 3U);
    CHECK(level.sound_events[2].sound == WG_SOUND_CLOSE_DOOR);
    CHECK(level.sound_events[2].positioned == 1U);
    CHECK(WL_MoveDoors(&level, 64U));
    CHECK(level.doors[0].position == 0U);
    CHECK(level.doors[0].action == WG_DOOR_CLOSED);

    level.doors[0].position = 0xffffU;
    level.doors[0].action = WG_DOOR_OPEN;
    level.player_x = 11 * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    level.player_tile_x = 11U;
    CHECK(!WL_CloseDoor(&level, 0U));
    level.doors[0].action = WG_DOOR_CLOSING;
    CHECK(WL_MoveDoors(&level, 1U));
    CHECK(level.doors[0].action == WG_DOOR_OPENING);

    SetPlayerMovementLevel(&level);
    level.tiles[10U * WG_LEVEL_SIZE + 11U] = 2U;
    level.info[10U * WG_LEVEL_SIZE + 11U] = 98U;
    CHECK(WL_CmdUse(&level));
    CHECK(level.pushwall_state == 1U);
    CHECK(level.pushwall_direction == 0U);

    SetPlayerMovementLevel(&level);
    level.tiles[10U * WG_LEVEL_SIZE + 11U] = 21U;
    level.areas[10U * WG_LEVEL_SIZE + 10U] = 0U;
    CHECK(WL_CmdUse(&level));
    CHECK(level.tiles[10U * WG_LEVEL_SIZE + 11U] == 22U);
    CHECK(level.level_completed == 1U);
    CHECK(level.secret_level == 1U);
}

static void TestPlayLoop(void)
{
    wg_level_t level;
    wg_view_tables_t tables;
    wl_play_state_t play;
    wl_input_t input;
    int32_t start_x;

    memset(&tables, 0, sizeof(tables));
    memset(&input, 0, sizeof(input));
    WG_ViewBuildTrigTables(&tables);
    WL_PlayStateReset(&play);
    SetPlayerMovementLevel(&level);

    start_x = level.player_x;
    input.up = 1U;
    CHECK(WL_PlayTick(&level, &tables, &play, &input));
    CHECK(level.player_x > start_x);
    CHECK(level.player_thrust_speed == 35 * 150);

    input.up = 0U;
    input.left = 1U;
    CHECK(WL_PlayTick(&level, &tables, &play, &input));
    CHECK(level.player_angle == 1U);

    input.left = 0U;
    input.weapon = 3U;
    level.player_best_weapon = WG_WEAPON_MACHINEGUN;
    CHECK(WL_PlayTick(&level, &tables, &play, &input));
    CHECK(level.player_weapon == WG_WEAPON_MACHINEGUN);
    CHECK(level.player_chosen_weapon == WG_WEAPON_MACHINEGUN);

    input.weapon = 0U;
    input.attack = 1U;
    CHECK(WL_PlayTick(&level, &tables, &play, &input));
    CHECK(level.attack_active == 1U);
    CHECK(level.attack_count == 6);
    CHECK(WL_PlayTick(&level, &tables, &play, &input));
    CHECK(level.attack_count == 5);

    WL_PlayStateReset(&play);
    SetPlayerMovementLevel(&level);
    memset(&input, 0, sizeof(input));
    input.mouse_x = 16;
    input.mouse_y = -4;
    start_x = level.player_x;
    CHECK(WL_PlayTick(&level, &tables, &play, &input));
    CHECK(level.player_angle == 359U);
    CHECK(level.player_x > start_x);
    CHECK(level.player_thrust_speed == 10 * 150);

    WL_PlayStateReset(&play);
    SetPlayerMovementLevel(&level);
    level.door_count = 1U;
    level.doors[0].tile_x = 11U;
    level.doors[0].tile_y = 10U;
    level.doors[0].vertical = 1U;
    level.doors[0].action = WG_DOOR_CLOSED;
    level.tiles[10U * WG_LEVEL_SIZE + 11U] = 0x80U;
    level.areas[10U * WG_LEVEL_SIZE + 10U] = 0U;
    level.areas[10U * WG_LEVEL_SIZE + 12U] = 1U;
    memset(&input, 0, sizeof(input));
    input.use = 1U;
    CHECK(WL_PlayTick(&level, &tables, &play, &input));
    CHECK(level.doors[0].action == WG_DOOR_OPENING);
    CHECK(level.doors[0].position == 0U);
    CHECK(WL_PlayTick(&level, &tables, &play, &input));
    CHECK(level.doors[0].position == 1024U);
}

static void TestIntermission(void)
{
    wg_level_t level;
    wl_intermission_t intermission;

    memset(&level, 0, sizeof(level));
    level.time_count = 75U * 70U;
    level.kill_count = level.kill_total = 20U;
    level.secret_count = level.secret_total = 2U;
    level.treasure_count = level.treasure_total = 4U;
    CHECK(WL_IntermissionCalculate(&level, 0U, &intermission));
    CHECK(intermission.seconds == 75U);
    CHECK(intermission.par_seconds == 90U);
    CHECK(intermission.kill_ratio == 100U);
    CHECK(intermission.secret_ratio == 100U);
    CHECK(intermission.treasure_ratio == 100U);
    CHECK(intermission.bonus == 37500U);
    CHECK(intermission.special_floor == 0U);

    level.time_count = 100U * 60U * 70U;
    CHECK(WL_IntermissionCalculate(&level, 8U, &intermission));
    CHECK(intermission.seconds == 99U * 60U);
    CHECK(intermission.par_seconds == 0U);
    CHECK(intermission.bonus == 15000U);
    CHECK(intermission.special_floor == 1U);
    CHECK(!WL_IntermissionCalculate(&level, 60U, &intermission));
}

static void TestPaletteShifts(void)
{
    wg_level_t level;
    uint8_t palette[256U * 3U];
    uint64_t hash;
    size_t index;

    memset(&level, 0, sizeof(level));
    level.damage_count = 40U;
    level.bonus_count = 18U;
    WL_UpdatePaletteShifts(&level, palette);
    CHECK(level.damage_count == 39U);
    CHECK(level.bonus_count == 17U);
    hash = 1469598103934665603ULL;
    for (index = 0U; index < sizeof(palette); ++index)
    {
        hash ^= palette[index];
        hash *= 1099511628211ULL;
    }
    CHECK(hash == 0xd6969024db3bdb40ULL);

    level.damage_count = 0U;
    level.bonus_count = 18U;
    WL_UpdatePaletteShifts(&level, palette);
    hash = 1469598103934665603ULL;
    for (index = 0U; index < sizeof(palette); ++index)
    {
        hash ^= palette[index];
        hash *= 1099511628211ULL;
    }
    CHECK(hash == 0x8b16358ec3225130ULL);

    level.bonus_count = 0U;
    WL_UpdatePaletteShifts(&level, palette);
    CHECK(memcmp(palette, WG_WolfPalette, sizeof(palette)) == 0);
}

static void TestPlayerDeathCamera(void)
{
    wg_level_t level;

    memset(&level, 0, sizeof(level));
    level.player_x = 100;
    level.player_y = 100;

    level.killer_x = 200;
    level.killer_y = 100;
    CHECK(WL_DeathTargetAngle(&level) == 0U);
    level.killer_x = 100;
    level.killer_y = 0;
    CHECK(WL_DeathTargetAngle(&level) == 90U);
    level.killer_x = 0;
    level.killer_y = 100;
    CHECK(WL_DeathTargetAngle(&level) == 180U);
    level.killer_x = 100;
    level.killer_y = 200;
    CHECK(WL_DeathTargetAngle(&level) == 270U);

    level.player_angle = 350U;
    CHECK(!WL_DeathRotateStep(&level, 10U, 2U));
    CHECK(level.player_angle == 352U);
    CHECK(level.player_angle_fraction == (352 << 16));
    CHECK(WL_DeathRotateStep(&level, 10U, 30U));
    CHECK(level.player_angle == 10U);

    level.player_angle = 0U;
    CHECK(!WL_DeathRotateStep(&level, 180U, 2U));
    CHECK(level.player_angle == 358U);

    level.player_health = 1U;
    level.difficulty = WG_DIFFICULTY_MEDIUM;
    WL_TakeDamageFrom(&level, 1U, 1234, 5678);
    CHECK(level.player_dead != 0U);
    CHECK(level.killer_x == 1234);
    CHECK(level.killer_y == 5678);
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

static void TestPushWalls(void)
{
    wg_level_t level;
    wg_view_tables_t tables;
    wg_wall_hit_t hits[WG_MAX_VIEW_WIDTH];
    wg_actor_t *blocker;
    size_t source = 10U * WG_LEVEL_SIZE + 10U;
    int x;
    int y;

    memset(&level, 0, sizeof(level));
    memset(&tables, 0, sizeof(tables));
    for (x = 0; x < WG_LEVEL_SIZE; ++x)
    {
        level.tiles[x] = 1U;
        level.tiles[(WG_LEVEL_SIZE - 1) * WG_LEVEL_SIZE + x] = 1U;
    }
    for (y = 0; y < WG_LEVEL_SIZE; ++y)
    {
        level.tiles[y * WG_LEVEL_SIZE] = 1U;
        level.tiles[y * WG_LEVEL_SIZE + WG_LEVEL_SIZE - 1] = 1U;
    }
    level.tiles[source] = 2U;
    level.info[source] = 98U;
    level.player_tile_x = 8U;
    level.player_tile_y = 10U;
    level.areas[10U * WG_LEVEL_SIZE + 8U] = 3U;
    CHECK(WL_PushWall(&level, 10U, 10U, 0U));
    CHECK(level.secret_count == 1U);
    CHECK(level.pushwall_state == 1U);
    CHECK(level.pushwall_position == 0U);
    CHECK(level.pushwall_x == 10U);
    CHECK(level.pushwall_y == 10U);
    CHECK(level.pushwall_direction == 0U);
    CHECK(level.tiles[source] == (2U | 0xc0U));
    CHECK(level.tiles[10U * WG_LEVEL_SIZE + 11U] == 2U);
    CHECK(level.info[source] == 0U);
    CHECK(!WL_PushWall(&level, 10U, 10U, 0U));

    CHECK(WL_MovePushWalls(&level, 63U));
    CHECK(level.pushwall_state == 64U);
    CHECK(level.pushwall_position == 32U);
    WG_ViewBuildTrigTables(&tables);
    CHECK(WG_ViewCalculateProjection(&tables, WG_MAX_VIEW_WIDTH,
                                     WG_FOCAL_LENGTH));
    CHECK(WG_RaycastWalls(&level, &tables,
                          8 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                          10 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                          0U, 10U, hits));
    CHECK(hits[159].side == WG_WALL_VERTICAL);
    CHECK(hits[159].map_x == 10U);
    CHECK(hits[159].tile == (2U | 0xc0U));
    CHECK(hits[159].x == 10 * WG_FIXED_ONE + WG_FIXED_ONE / 2);
    CHECK(hits[159].wall_page == 3U);

    CHECK(WL_MovePushWalls(&level, 64U));
    CHECK(level.pushwall_state == 128U);
    CHECK(level.pushwall_x == 11U);
    CHECK(level.tiles[source] == 0U);
    CHECK(level.areas[source] == 3U);
    CHECK(level.tiles[10U * WG_LEVEL_SIZE + 11U] == (2U | 0xc0U));
    CHECK(level.tiles[10U * WG_LEVEL_SIZE + 12U] == 2U);

    level.actor_count = 1U;
    blocker = &level.actors[0];
    blocker->tile_x = 13U;
    blocker->tile_y = 10U;
    blocker->flags = WG_ACTOR_FLAG_SHOOTABLE;
    CHECK(WL_MovePushWalls(&level, 128U));
    CHECK(level.pushwall_state == 0U);
    CHECK(level.pushwall_x == 12U);
    CHECK(level.tiles[10U * WG_LEVEL_SIZE + 11U] == 0U);
    CHECK(level.tiles[10U * WG_LEVEL_SIZE + 12U] == 2U);
    CHECK(level.tiles[10U * WG_LEVEL_SIZE + 13U] == 0U);

    memset(&level, 0, sizeof(level));
    level.tiles[source] = 2U;
    CHECK(WL_PushWall(&level, 10U, 10U, 0U));
    CHECK(WL_MovePushWalls(&level, 127U));
    CHECK(WL_MovePushWalls(&level, 128U));
    CHECK(WL_MovePushWalls(&level, 128U));
    CHECK(level.pushwall_state == 0U);
    CHECK(level.tiles[10U * WG_LEVEL_SIZE + 10U] == 0U);
    CHECK(level.tiles[10U * WG_LEVEL_SIZE + 11U] == 0U);
    CHECK(level.tiles[10U * WG_LEVEL_SIZE + 12U] == 0U);
    CHECK(level.tiles[10U * WG_LEVEL_SIZE + 13U] == 2U);

    memset(&level, 0, sizeof(level));
    level.tiles[source] = 2U;
    level.tiles[10U * WG_LEVEL_SIZE + 11U] = 1U;
    CHECK(!WL_PushWall(&level, 10U, 10U, 0U));
    CHECK(level.secret_count == 0U);
    CHECK(level.tiles[source] == 2U);

    memset(&level, 0, sizeof(level));
    level.tiles[source] = 2U;
    level.static_count = 1U;
    level.statics[0].tile_x = 11U;
    level.statics[0].tile_y = 10U;
    level.statics[0].blocking = 1U;
    CHECK(!WL_PushWall(&level, 10U, 10U, 0U));
    level.statics[0].blocking = 0U;
    CHECK(WL_PushWall(&level, 10U, 10U, 0U));
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
    uint64_t pause_hash = 1469598103934665603ULL;
    size_t index;
    size_t actor_index;
    size_t decoded_graphics = 0;
    size_t loaded_maps = 0;
    size_t present_pages = 0;
    size_t decoded_sprites = 0;
    size_t actor_classes[WG_ACTOR_FAT + 1] = { 0 };
    size_t item_types[WG_ITEM_CLIP2 + 1] = { 0 };

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
        CHECK(WG_GraphicsDecodePicture(
            &graphics,
            expected_variant == WG_GAME_WOLF3D_SHAREWARE_14 ? 145U : 133U,
            &picture_pixels, &picture_width, &picture_height));
        if (picture_pixels != NULL)
        {
            printf("%s pause picture dimensions: %ux%u\n",
                   WG_DataVariantName(data_set.variant),
                   (unsigned)picture_width, (unsigned)picture_height);
            CHECK(picture_width == 64U);
            CHECK(picture_height == 32U);
            free(picture_pixels);
        }
        memset(framebuffer, 0, sizeof(framebuffer));
        CHECK(WL_DrawPaused(framebuffer, &graphics));
        for (index = 0U; index < sizeof(framebuffer); ++index)
        {
            pause_hash ^= framebuffer[index];
            pause_hash *= 1099511628211ULL;
        }
        printf("%s pause overlay FNV-1a: %016llx\n",
               WG_DataVariantName(data_set.variant),
               (unsigned long long)pause_hash);
        CHECK(pause_hash == 0xee855388f16e0af7ULL);
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
            printf("%s map 0 totals: %u kills, %u secrets, %u treasures\n",
                   WG_DataVariantName(data_set.variant),
                   (unsigned)level.kill_total,
                   (unsigned)level.secret_total,
                   (unsigned)level.treasure_total);
            CHECK(level.kill_total == 20U);
            CHECK(level.secret_total == 5U);
            CHECK(level.treasure_total == 23U);
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
                for (actor_index = 0; actor_index < level.static_count;
                     ++actor_index)
                {
                    const wg_static_object_t *object =
                        &level.statics[actor_index];

                    CHECK(object->item <= WG_ITEM_CLIP2);
                    if (object->item <= WG_ITEM_CLIP2)
                    {
                        ++item_types[object->item];
                    }
                }
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
        CHECK(item_types[WG_ITEM_ALPO] != 0U);
        CHECK(item_types[WG_ITEM_KEY1] != 0U);
        CHECK(item_types[WG_ITEM_FOOD] != 0U);
        CHECK(item_types[WG_ITEM_FIRSTAID] != 0U);
        CHECK(item_types[WG_ITEM_CLIP] != 0U);
        CHECK(item_types[WG_ITEM_MACHINEGUN] != 0U);
        CHECK(item_types[WG_ITEM_CHAINGUN] != 0U);
        CHECK(item_types[WG_ITEM_CROSS] != 0U);
        CHECK(item_types[WG_ITEM_CHALICE] != 0U);
        CHECK(item_types[WG_ITEM_BIBLE] != 0U);
        CHECK(item_types[WG_ITEM_CROWN] != 0U);
        CHECK(item_types[WG_ITEM_FULLHEAL] != 0U);
        CHECK(item_types[WG_ITEM_GIBS] != 0U);
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
        id_sd_imf_t music;
        id_sd_music_t *music_player;
        opl_write_log_t log;
        int16_t *music_pcm;
        uint64_t music_hash = 1469598103934665603ULL;
        uint64_t mixed_hash = 1469598103934665603ULL;
        uint64_t digital_hash = 1469598103934665603ULL;
        const uint8_t *music_data;
        size_t music_size;
        size_t music_count = 0U;

        CHECK(audio.offset_count == 289U);
        CHECK(WG_AudioGetChunk(&audio, 261U, &page_data, &page_size));
        CHECK(page_size > 2U);
        music_data = page_data;
        music_size = page_size;
        music_player = ID_SD_MusicCreate(48000U);
        music_pcm = (int16_t *)malloc(48000U * 2U * sizeof(*music_pcm));
        CHECK(music_player != NULL && music_pcm != NULL);
        if (music_player != NULL && music_pcm != NULL)
        {
            const uint8_t *pcm_bytes = (const uint8_t *)music_pcm;

            CHECK(ID_SD_MusicStart(music_player, page_data, page_size));
            CHECK(ID_SD_MusicRender(music_player, music_pcm, 48000U));
            for (index = 0U;
                 index < 48000U * 2U * sizeof(*music_pcm); ++index)
            {
                music_hash ^= pcm_bytes[index];
                music_hash *= 1099511628211ULL;
            }
            printf("%s one-second OPL music FNV-1a: %016llx\n",
                   WG_DataVariantName(data_set.variant),
                   (unsigned long long)music_hash);
            CHECK(music_hash == 0x201858e57f147650ULL);
            ID_SD_MusicDestroy(music_player);
            music_player = ID_SD_MusicCreate(48000U);
            CHECK(music_player != NULL);
            CHECK(ID_SD_MusicStart(music_player, music_data, music_size));
            CHECK(WG_AudioGetChunk(&audio, 111U, &page_data, &page_size));
            CHECK(ID_SD_EffectStart(music_player, page_data, page_size));
            CHECK(ID_SD_MusicRender(music_player, music_pcm, 4800U));
            for (index = 0U;
                 index < 4800U * 2U * sizeof(*music_pcm); ++index)
            {
                mixed_hash ^= pcm_bytes[index];
                mixed_hash *= 1099511628211ULL;
            }
            printf("%s music plus pistol FNV-1a: %016llx\n",
                   WG_DataVariantName(data_set.variant),
                   (unsigned long long)mixed_hash);
            CHECK(mixed_hash == 0xbebd8fbdef66d214ULL);
            for (index = 87U; index < 174U; ++index)
            {
                CHECK(WG_AudioGetChunk(&audio, index, &page_data, &page_size));
                ID_SD_EffectStop(music_player);
                CHECK(ID_SD_EffectStart(music_player, page_data, page_size));
            }
            CHECK(WG_PagesOpen(&pages, &data_set));
            if (pages.data_set != NULL)
            {
                id_sd_digi_bank_t digi_bank;
                uint8_t *digital_data = NULL;
                size_t digital_length = 0U;
                size_t loaded_digital = 0U;

                CHECK(ID_SD_DigiBankOpen(&digi_bank, &pages));
                CHECK(digi_bank.count == 46U);
                for (index = 0U; index < digi_bank.count; ++index)
                {
                    if (ID_SD_DigiBankLoad(&digi_bank, index,
                                           &digital_data, &digital_length))
                    {
                        CHECK(digital_length
                              == digi_bank.entries[index].length);
                        ++loaded_digital;
                        free(digital_data);
                        digital_data = NULL;
                    }
                }
                printf("%s loadable digitized sounds: %llu/%llu\n",
                       WG_DataVariantName(data_set.variant),
                       (unsigned long long)loaded_digital,
                       (unsigned long long)digi_bank.count);
                CHECK(loaded_digital
                      == (expected_variant == WG_GAME_WOLF3D_SHAREWARE_14
                              ? 20U : 46U));
                CHECK(ID_SD_DigiBankLoad(&digi_bank, 5U,
                                         &digital_data, &digital_length));
                ID_SD_MusicDestroy(music_player);
                music_player = ID_SD_MusicCreate(48000U);
                CHECK(ID_SD_DigitalStart(music_player, digital_data,
                                         digital_length, 10U, 0U, 0U));
                CHECK(ID_SD_MusicRender(music_player, music_pcm, 48000U));
                for (index = 0U;
                     index < 48000U * 2U * sizeof(*music_pcm); ++index)
                {
                    digital_hash ^= pcm_bytes[index];
                    digital_hash *= 1099511628211ULL;
                }
                printf("%s one-second digital pistol FNV-1a: %016llx\n",
                       WG_DataVariantName(data_set.variant),
                       (unsigned long long)digital_hash);
                CHECK(digital_hash == 0x44dc84b3f78798a3ULL);
                free(digital_data);
                WG_PagesClose(&pages);
            }
        }
        free(music_pcm);
        ID_SD_MusicDestroy(music_player);
        CHECK(!WG_AudioGetChunk(&audio, 288U, &page_data, &page_size));
        for (index = 0; index + 1U < audio.offset_count; ++index)
        {
            CHECK(WG_AudioGetChunk(&audio, index, &page_data, &page_size));
        }
        memset(&log, 0, sizeof(log));
        for (index = 261U; index < 288U; ++index)
        {
            CHECK(WG_AudioGetChunk(&audio, index, &page_data, &page_size));
            if (page_size >= 2U
                && (page_data[0] != 0U || page_data[1] != 0U))
            {
                CHECK(ID_SD_IMFStart(&music, page_data, page_size,
                                     LogOPLWrite, &log));
                ++music_count;
            }
        }
        CHECK(music_count == (expected_variant == WG_GAME_WOLF3D_SHAREWARE_14
                                  ? 11U : 27U));
        WG_AudioClose(&audio);
    }
    WG_DataClose(&data_set);
}

int main(int argc, char **argv)
{
    TestIMFSequencer();
    TestHuffman();
    TestCarmack();
    TestRLEW();
    TestMalformedCompression();
    TestVideo();
    TestRandom();
    TestActorSetup();
    TestStaticItemSetup();
    TestBossAndGhostSetup();
    TestPatrolMovement();
    TestActorAwareness();
    TestDoorAreaConnectivity();
    TestOrdinaryShootingStates();
    TestDogChaseAndBite();
    TestBossShootingStates();
    TestSchabbsNeedle();
    TestRocketBossAttacks();
    TestFakeHitlerFlames();
    TestActorDamageAndDeath();
    TestBossDamageAndDeath();
    TestPlayerWeapons();
    TestBonusPickups();
    TestPlayerMovementAndUse();
    TestPlayLoop();
    TestIntermission();
    TestPaletteShifts();
    TestPlayerDeathCamera();
    TestViewMath();
    TestWallScaler();
    TestStaticRaycaster();
    TestPushWalls();
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
