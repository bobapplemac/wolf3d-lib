#include "WOLF3DGENERIC.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "WG_DATA.h"
#include "WG_AUDIO.h"
#include "WG_FIXED.h"
#include "WG_GRAPHICS.h"
#include "WG_ENDIAN.h"
#include "ID_US_1.h"
#include "WL_AGENT.h"
#include "WL_ACT1.h"
#include "WL_GAME.h"
#include "WG_MAPS.h"
#include "WG_PALETTE.h"
#include "ID_PM.h"
#include "ID_SD.h"
#include "ID_VL.h"
#include "WG_PLATFORM.h"
#include "WG_RENDERER.h"
#include "WL_DRAW.h"
#include "WL_INTER.h"
#include "WL_MAIN.h"
#include "WL_PLAY.h"
#include "WL_STATE.h"

uint8_t *WG_ScreenBuffer;
uint8_t WG_Palette[WG_PALETTE_COLORS * 3];

static int wg_initialized;
static int wg_data_loaded;
static unsigned wg_start_map;
static wg_data_set_t wg_data_set;
static wl_high_score_t wg_high_scores[WL_MAX_HIGH_SCORES];

typedef enum wg_death_phase
{
    WG_DEATH_NONE = 0,
    WG_DEATH_ROTATE,
    WG_DEATH_FIZZLE_PENDING,
    WG_DEATH_FIZZLE,
    WG_DEATH_HOLD
} wg_death_phase_t;

typedef struct wg_game_session
{
    int active;
    wg_level_t level;
    wg_pages_t pages;
    wg_wall_cache_t walls;
    wg_view_tables_t view;
    wg_graphics_t graphics;
    wg_audio_t audio;
    id_sd_digi_bank_t digi_bank;
    id_sd_music_t *music;
    wg_wall_hit_t hits[WG_MAX_VIEW_WIDTH];
    uint8_t visible_tiles[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    uint8_t keys[128];
    uint8_t mouse_buttons;
    int mouse_x;
    int mouse_y;
    wl_play_state_t play;
    uint8_t audio_active;
    uint8_t sound_positioned;
    uint8_t game_over;
    uint8_t paused;
    uint8_t intermission;
    uint8_t victory;
    uint8_t high_scores;
    int8_t high_score_entry;
    uint8_t high_score_cursor;
    uint8_t high_score_caps_lock;
    wg_death_phase_t death_phase;
    uint16_t death_target_angle;
    uint16_t death_hold_tics;
    uint8_t death_acknowledged;
    wg_fizzle_t death_fizzle;
    uint8_t death_source[WG_SCREEN_WIDTH * WG_PLAY_VIEW_HEIGHT];
    unsigned map_number;
    unsigned next_map_number;
    uint32_t level_start_score;
    wl_intermission_t intermission_state;
    wl_intermission_t level_ratios[8];
    int32_t sound_x;
    int32_t sound_y;
} wg_game_session_t;

static wg_game_session_t wg_game;

static unsigned WG_GameSessionNextMap(void);

static const char *WG_FindDataPath(int argc, char **argv)
{
    int index;

    for (index = 1; index < argc; ++index)
    {
        if (strcmp(argv[index], "--data") == 0)
        {
            return index + 1 < argc ? argv[index + 1] : NULL;
        }
    }
    return NULL;
}

static int WG_HasArgument(int argc, char **argv, const char *argument)
{
    int index;

    for (index = 1; index < argc; ++index)
    {
        if (strcmp(argv[index], argument) == 0)
        {
            return 1;
        }
    }
    return 0;
}

static int WG_FindUnsignedArgument(int argc, char **argv,
                                   const char *argument,
                                   unsigned default_value,
                                   unsigned maximum, unsigned *value_out)
{
    int index;

    if (argument == NULL || value_out == NULL)
    {
        return 0;
    }
    *value_out = default_value;
    for (index = 1; index < argc; ++index)
    {
        if (strcmp(argv[index], argument) == 0)
        {
            char *end;
            unsigned long value;

            if (index + 1 >= argc)
            {
                return 0;
            }
            end = NULL;
            value = strtoul(argv[index + 1], &end, 10);
            if (end == argv[index + 1] || *end != '\0' || value > maximum)
            {
                return 0;
            }
            *value_out = (unsigned)value;
            return 1;
        }
    }
    return 1;
}

static wg_actor_t *WG_FindGuardViewActor(wg_level_t *level)
{
    size_t index;

    if (level == NULL)
    {
        return NULL;
    }
    for (index = level->actor_count; index > 0U; --index)
    {
        wg_actor_t *actor = &level->actors[index - 1U];

        if (actor->actor_class == WG_ACTOR_GUARD
            && actor->rotate != 0U && actor->tile_x >= 3U)
        {
            return actor;
        }
    }
    return NULL;
}

static int WG_IsBossClass(wg_actor_class_t actor_class)
{
    return actor_class == WG_ACTOR_BOSS
           || actor_class == WG_ACTOR_SCHABBS
           || actor_class == WG_ACTOR_FAKE
           || actor_class == WG_ACTOR_MECHA_HITLER
           || actor_class == WG_ACTOR_GRETEL
           || actor_class == WG_ACTOR_GIFT
           || actor_class == WG_ACTOR_FAT;
}

static wg_actor_t *WG_FindBossViewActor(wg_level_t *level)
{
    size_t index;

    if (level == NULL)
    {
        return NULL;
    }
    for (index = 0; index < level->actor_count; ++index)
    {
        if (WG_IsBossClass(level->actors[index].actor_class))
        {
            return &level->actors[index];
        }
    }
    return NULL;
}

static const wg_actor_t *WG_FindPatrolViewActor(const wg_level_t *level)
{
    size_t index;

    if (level == NULL)
    {
        return NULL;
    }
    for (index = 0; index < level->actor_count; ++index)
    {
        if (level->actors[index].state >= WG_STATE_PATH1
            && level->actors[index].state <= WG_STATE_PATH4)
        {
            return &level->actors[index];
        }
    }
    return NULL;
}

static wg_actor_t *WG_FindDogViewActor(wg_level_t *level)
{
    size_t index;

    if (level == NULL)
    {
        return NULL;
    }
    for (index = 0; index < level->actor_count; ++index)
    {
        if (level->actors[index].actor_class == WG_ACTOR_DOG)
        {
            return &level->actors[index];
        }
    }
    return NULL;
}

static wg_actor_t *WG_FindActorClass(wg_level_t *level,
                                     wg_actor_class_t actor_class)
{
    size_t index;

    if (level == NULL)
    {
        return NULL;
    }
    for (index = 0; index < level->actor_count; ++index)
    {
        if (level->actors[index].actor_class == actor_class)
        {
            return &level->actors[index];
        }
    }
    return NULL;
}

static int WG_SetActorViewPose(wg_level_t *level, const wg_actor_t *actor,
                               int distance)
{
    static const int directions_x[] = { -1, 1, 0, 0 };
    static const int directions_y[] = { 0, 0, -1, 1 };
    static const uint16_t angles[] = { 0U, 180U, 270U, 90U };
    size_t direction;

    if (level == NULL || actor == NULL || distance <= 0)
    {
        return 0;
    }
    for (direction = 0; direction < 4U; ++direction)
    {
        int step;
        int clear = 1;

        for (step = 1; step <= distance; ++step)
        {
            int x = (int)actor->tile_x + directions_x[direction] * step;
            int y = (int)actor->tile_y + directions_y[direction] * step;

            if (x < 0 || x >= WG_LEVEL_SIZE || y < 0 || y >= WG_LEVEL_SIZE
                || level->tiles[(size_t)y * WG_LEVEL_SIZE + (size_t)x] != 0U)
            {
                clear = 0;
                break;
            }
        }
        if (clear)
        {
            level->player_x = actor->x
                              + directions_x[direction] * distance
                                * WG_FIXED_ONE;
            level->player_y = actor->y
                              + directions_y[direction] * distance
                                * WG_FIXED_ONE;
            level->player_angle = angles[direction];
            level->player_tile_x = (uint8_t)(level->player_x / WG_FIXED_ONE);
            level->player_tile_y = (uint8_t)(level->player_y / WG_FIXED_ONE);
            return 1;
        }
    }
    return 0;
}

static int WG_SetPushWallView(wg_level_t *level)
{
    static const int direction_x[] = { 1, 0, -1, 0 };
    static const int direction_y[] = { 0, -1, 0, 1 };
    static const uint16_t angles[] = { 0U, 90U, 180U, 270U };
    size_t index;

    if (level == NULL)
    {
        return 0;
    }
    for (index = 0U; index < WG_LEVEL_SIZE * WG_LEVEL_SIZE; ++index)
    {
        int wall_x;
        int wall_y;
        size_t direction;

        if (level->info[index] != 98U)
        {
            continue;
        }
        wall_x = (int)(index % WG_LEVEL_SIZE);
        wall_y = (int)(index / WG_LEVEL_SIZE);
        for (direction = 0U; direction < 4U; ++direction)
        {
            int target_x = wall_x + direction_x[direction];
            int target_y = wall_y + direction_y[direction];
            int near_x = wall_x - direction_x[direction];
            int near_y = wall_y - direction_y[direction];
            int view_x = wall_x - direction_x[direction] * 2;
            int view_y = wall_y - direction_y[direction] * 2;

            if (target_x <= 0 || target_x + 1 >= WG_LEVEL_SIZE
                || target_y <= 0 || target_y + 1 >= WG_LEVEL_SIZE
                || view_x <= 0 || view_x + 1 >= WG_LEVEL_SIZE
                || view_y <= 0 || view_y + 1 >= WG_LEVEL_SIZE
                || level->tiles[(size_t)target_y * WG_LEVEL_SIZE
                                + (size_t)target_x] != 0U
                || level->tiles[(size_t)near_y * WG_LEVEL_SIZE
                                + (size_t)near_x] != 0U
                || level->tiles[(size_t)view_y * WG_LEVEL_SIZE
                                + (size_t)view_x] != 0U)
            {
                continue;
            }
            level->player_tile_x = (uint8_t)view_x;
            level->player_tile_y = (uint8_t)view_y;
            level->player_x = view_x * WG_FIXED_ONE + WG_FIXED_ONE / 2;
            level->player_y = view_y * WG_FIXED_ONE + WG_FIXED_ONE / 2;
            level->player_angle = angles[direction];
            return WL_PushWall(level, (uint8_t)wall_x, (uint8_t)wall_y,
                               (uint8_t)(direction * 2U))
                   && WL_MovePushWalls(level, 63U);
        }
    }
    return 0;
}

static int WG_SetPickupView(wg_level_t *level)
{
    static const int directions_x[] = { -1, 1, 0, 0 };
    static const int directions_y[] = { 0, 0, -1, 1 };
    static const uint16_t angles[] = { 0U, 180U, 270U, 90U };
    size_t index;

    if (level == NULL)
    {
        return 0;
    }
    for (index = 0U; index < level->static_count; ++index)
    {
        const wg_static_object_t *object = &level->statics[index];
        size_t direction;

        if (object->item != WG_ITEM_CROSS || object->removed != 0U)
        {
            continue;
        }
        for (direction = 0U; direction < 4U; ++direction)
        {
            int step;
            int clear = 1;

            for (step = 1; step <= 2; ++step)
            {
                int x = (int)object->tile_x
                        + directions_x[direction] * step;
                int y = (int)object->tile_y
                        + directions_y[direction] * step;

                if (x < 0 || x >= WG_LEVEL_SIZE
                    || y < 0 || y >= WG_LEVEL_SIZE
                    || level->tiles[(size_t)y * WG_LEVEL_SIZE
                                    + (size_t)x] != 0U)
                {
                    clear = 0;
                    break;
                }
            }
            if (clear)
            {
                int view_x = (int)object->tile_x
                             + directions_x[direction] * 2;
                int view_y = (int)object->tile_y
                             + directions_y[direction] * 2;

                level->player_tile_x = object->tile_x;
                level->player_tile_y = object->tile_y;
                if (WL_CollectPlayerTileBonuses(level) == 0U)
                {
                    return 0;
                }
                level->player_tile_x = (uint8_t)view_x;
                level->player_tile_y = (uint8_t)view_y;
                level->player_x = view_x * WG_FIXED_ONE + WG_FIXED_ONE / 2;
                level->player_y = view_y * WG_FIXED_ONE + WG_FIXED_ONE / 2;
                level->player_angle = angles[direction];
                return 1;
            }
        }
    }
    return 0;
}

static int WG_SetDoorUseView(wg_level_t *level)
{
    static const int side_x[] = { -1, 1, 0, 0 };
    static const int side_y[] = { 0, 0, -1, 1 };
    static const uint16_t angles[] = { 0U, 180U, 270U, 90U };
    size_t door_index;

    if (level == NULL)
    {
        return 0;
    }
    for (door_index = 0U; door_index < level->door_count; ++door_index)
    {
        const wg_door_t *door = &level->doors[door_index];
        size_t first_side = door->vertical != 0U ? 0U : 2U;
        size_t last_side = first_side + 2U;
        size_t side;

        if (door->lock != WG_DOOR_NORMAL)
        {
            continue;
        }
        for (side = first_side; side < last_side; ++side)
        {
            int near_x = (int)door->tile_x + side_x[side];
            int near_y = (int)door->tile_y + side_y[side];
            int view_x = (int)door->tile_x + side_x[side] * 2;
            int view_y = (int)door->tile_y + side_y[side] * 2;

            if (near_x <= 0 || near_x + 1 >= WG_LEVEL_SIZE
                || near_y <= 0 || near_y + 1 >= WG_LEVEL_SIZE
                || view_x <= 0 || view_x + 1 >= WG_LEVEL_SIZE
                || view_y <= 0 || view_y + 1 >= WG_LEVEL_SIZE
                || level->tiles[(size_t)near_y * WG_LEVEL_SIZE
                                + (size_t)near_x] != 0U
                || level->tiles[(size_t)view_y * WG_LEVEL_SIZE
                                + (size_t)view_x] != 0U)
            {
                continue;
            }
            level->player_tile_x = (uint8_t)near_x;
            level->player_tile_y = (uint8_t)near_y;
            level->player_x = near_x * WG_FIXED_ONE + WG_FIXED_ONE / 2;
            level->player_y = near_y * WG_FIXED_ONE + WG_FIXED_ONE / 2;
            level->player_angle = angles[side];
            if (!WL_CmdUse(level) || !WL_MoveDoors(level, 32U))
            {
                return 0;
            }
            level->player_tile_x = (uint8_t)view_x;
            level->player_tile_y = (uint8_t)view_y;
            level->player_x = view_x * WG_FIXED_ONE + WG_FIXED_ONE / 2;
            level->player_y = view_y * WG_FIXED_ONE + WG_FIXED_ONE / 2;
            return 1;
        }
    }
    return 0;
}

static int WG_LoadTitleScreen(const char *data_path)
{
    wg_graphics_t graphics;
    char title[160];
    int title_length;

    if (!WG_DataOpen(&wg_data_set, data_path))
    {
        WG_ReportError("The selected directory is not a supported Wolf3D v1.4 data set.");
        return 0;
    }
    if (!WG_GraphicsOpen(&graphics, &wg_data_set))
    {
        WG_ReportError("Unable to open the Wolf3D graphics resources.");
        WG_DataClose(&wg_data_set);
        return 0;
    }
    if (!WG_GraphicsDecodeTitle(&graphics, wg_data_set.variant, WG_ScreenBuffer))
    {
        WG_ReportError("Unable to decode the Wolf3D title screen.");
        WG_GraphicsClose(&graphics);
        WG_DataClose(&wg_data_set);
        return 0;
    }
    WG_GraphicsClose(&graphics);

    memcpy(WG_Palette, WG_WolfPalette, sizeof(WG_Palette));
    title_length = snprintf(title, sizeof(title), "wolf3dgeneric - %s",
                            WG_DataVariantName(wg_data_set.variant));
    if (title_length >= 0 && (size_t)title_length < sizeof(title))
    {
        WG_SetWindowTitle(title);
    }
    wg_data_loaded = 1;
    return 1;
}

static void WG_GameSessionClose(void)
{
    WG_PCMShutdown();
    ID_SD_MusicDestroy(wg_game.music);
    WG_AudioClose(&wg_game.audio);
    WG_GraphicsClose(&wg_game.graphics);
    WG_WallCacheFree(&wg_game.walls);
    WG_PagesClose(&wg_game.pages);
    memset(&wg_game, 0, sizeof(wg_game));
}

static int WG_GameSessionPumpAudio(void)
{
    int16_t samples[1024U * 2U];
    size_t frames;

    if (!wg_game.audio_active)
    {
        return 1;
    }
    while ((frames = WG_PCMWritableFrames()) != 0U)
    {
        if (frames > 1024U)
        {
            frames = 1024U;
        }
        if (!ID_SD_MusicRender(wg_game.music, samples, frames)
            || !WG_PCMSubmit(samples, frames))
        {
            return 0;
        }
    }
    return 1;
}

static int WG_GameSessionRender(void)
{
    wl_status_t status;

    if (!wg_game.active
        || !WG_RenderStaticView(WG_ScreenBuffer, &wg_game.level,
                                &wg_game.view, &wg_game.walls, 0, 0,
                                wg_game.level.player_x,
                                wg_game.level.player_y,
                                wg_game.level.player_angle, wg_game.hits,
                                wg_game.visible_tiles)
        || !WL_DrawScaleds(WG_ScreenBuffer, &wg_game.pages, &wg_game.level,
                           &wg_game.view, wg_game.hits,
                           wg_game.visible_tiles, wg_game.level.player_x,
                           wg_game.level.player_y,
                           wg_game.level.player_angle))
    {
        return 0;
    }
    if (wg_game.death_phase == WG_DEATH_NONE
        && !WL_DrawPlayerWeapon(WG_ScreenBuffer, &wg_game.pages,
                                wg_game.level.player_weapon,
                                wg_game.level.weapon_frame))
    {
        return 0;
    }
    WL_StatusDefaults(&status);
    status.score = wg_game.level.score;
    status.health = wg_game.level.player_health;
    status.ammo = wg_game.level.player_ammo;
    status.weapon = wg_game.level.player_weapon;
    status.lives = wg_game.level.player_lives;
    status.keys = wg_game.level.player_keys;
    return WL_DrawStatusBar(WG_ScreenBuffer, &wg_game.graphics, &status);
}

static int WG_GameSessionSetPaused(int paused)
{
    wg_game.paused = paused != 0;
    memset(wg_game.keys, 0, sizeof(wg_game.keys));
    wg_game.mouse_buttons = 0U;
    wg_game.mouse_x = 0;
    wg_game.mouse_y = 0;
    ID_SD_MusicSetPaused(wg_game.music, wg_game.paused);
    if (wg_game.paused)
    {
        return WL_DrawPaused(WG_ScreenBuffer, &wg_game.graphics);
    }
    return WG_GameSessionRender();
}

static int WG_GameSessionBeginIntermission(void)
{
    const uint8_t *music_data;
    size_t music_size;

    if (!WL_IntermissionCalculate(&wg_game.level, wg_game.map_number,
                                  &wg_game.intermission_state))
    {
        return 0;
    }
    wg_game.next_map_number = WG_GameSessionNextMap();
    if (wg_game.map_number % 10U < 8U)
    {
        wg_game.level_ratios[wg_game.map_number % 10U] =
            wg_game.intermission_state;
    }
    wg_game.intermission = 1U;
    memset(wg_game.keys, 0, sizeof(wg_game.keys));
    wg_game.mouse_buttons = 0U;
    wg_game.mouse_x = 0;
    wg_game.mouse_y = 0;
    memcpy(WG_Palette, WG_WolfPalette, sizeof(WG_Palette));
    if (wg_game.audio_active
        && WG_AudioGetChunk(&wg_game.audio, 261U + 16U,
                            &music_data, &music_size))
    {
        (void)ID_SD_MusicStart(wg_game.music, music_data, music_size);
    }
    if (!WL_DrawLevelCompleted(WG_ScreenBuffer, &wg_game.graphics,
                               &wg_game.level, wg_game.map_number,
                               &wg_game.intermission_state))
    {
        return 0;
    }
    wg_game.level.score += wg_game.intermission_state.bonus;
    return 1;
}

static int WG_GameSessionBeginVictory(void)
{
    const uint8_t *music_data;
    size_t music_size;
    wl_victory_t victory;

    if (!WL_VictoryCalculate(wg_game.level_ratios, &victory)
        || !WL_DrawVictory(WG_ScreenBuffer, &wg_game.graphics,
                           &wg_game.level, &victory))
    {
        return 0;
    }
    wg_game.victory = 1U;
    memset(wg_game.keys, 0, sizeof(wg_game.keys));
    wg_game.mouse_buttons = 0U;
    wg_game.mouse_x = 0;
    wg_game.mouse_y = 0;
    memcpy(WG_Palette, WG_WolfPalette, sizeof(WG_Palette));
    if (wg_game.audio_active
        && WG_AudioGetChunk(&wg_game.audio, 261U + 24U,
                            &music_data, &music_size))
    {
        (void)ID_SD_MusicStart(wg_game.music, music_data, music_size);
    }
    return 1;
}

static int WG_GameSessionBeginHighScores(void)
{
    const uint8_t *music_data;
    size_t music_size;

    wg_game.high_score_entry = (int8_t)WL_HighScoreInsert(
        wg_high_scores, wg_game.level.score,
        (uint16_t)(wg_game.map_number % 10U + 1U),
        (uint16_t)(wg_game.map_number / 10U));
    if (!WL_DrawHighScores(WG_ScreenBuffer, &wg_game.graphics,
                           wg_high_scores))
    {
        return 0;
    }
    wg_game.game_over = 1U;
    wg_game.high_scores = 1U;
    if (wg_game.high_score_entry >= 0
        && !WL_DrawHighScoreCursor(
            WG_ScreenBuffer, &wg_game.graphics, wg_high_scores,
            (unsigned)wg_game.high_score_entry, 0U))
    {
        return 0;
    }
    memcpy(WG_Palette, WG_WolfPalette, sizeof(WG_Palette));
    if (wg_game.audio_active
        && WG_AudioGetChunk(&wg_game.audio, 261U + 23U,
                            &music_data, &music_size))
    {
        (void)ID_SD_MusicStart(wg_game.music, music_data, music_size);
    }
    return 1;
}

static int WG_GameSessionHighScoreKey(uint16_t scan_code, int pressed)
{
    wl_high_score_t *score;
    size_t length;
    int shifted;
    char character;

    if (scan_code == WG_KEY_LEFT_SHIFT
        || scan_code == WG_KEY_RIGHT_SHIFT)
    {
        wg_game.keys[scan_code] = pressed != 0;
    }
    if (!pressed || wg_game.high_score_entry < 0)
    {
        return 1;
    }
    score = &wg_high_scores[(unsigned)wg_game.high_score_entry];
    length = strlen(score->name);
    if (scan_code == WG_KEY_CAPS_LOCK)
    {
        wg_game.high_score_caps_lock ^= 1U;
    }
    else if (scan_code == WG_KEY_LEFT)
    {
        if (wg_game.high_score_cursor != 0U)
        {
            --wg_game.high_score_cursor;
        }
    }
    else if (scan_code == WG_KEY_RIGHT)
    {
        if (wg_game.high_score_cursor < length)
        {
            ++wg_game.high_score_cursor;
        }
    }
    else if (scan_code == WG_KEY_HOME)
    {
        wg_game.high_score_cursor = 0U;
    }
    else if (scan_code == WG_KEY_END)
    {
        wg_game.high_score_cursor = (uint8_t)length;
    }
    else if (scan_code == WG_KEY_BACKSPACE)
    {
        if (wg_game.high_score_cursor != 0U)
        {
            memmove(score->name + wg_game.high_score_cursor - 1U,
                    score->name + wg_game.high_score_cursor,
                    length - wg_game.high_score_cursor + 1U);
            --wg_game.high_score_cursor;
        }
    }
    else if (scan_code == WG_KEY_DELETE)
    {
        if (wg_game.high_score_cursor < length)
        {
            memmove(score->name + wg_game.high_score_cursor,
                    score->name + wg_game.high_score_cursor + 1U,
                    length - wg_game.high_score_cursor);
        }
    }
    else if (scan_code == WG_KEY_ENTER)
    {
        wg_game.high_score_entry = -1;
    }
    else if (scan_code == WG_KEY_ESCAPE)
    {
        score->name[0] = '\0';
        wg_game.high_score_entry = -1;
    }
    else
    {
        shifted = wg_game.keys[WG_KEY_LEFT_SHIFT]
                  || wg_game.keys[WG_KEY_RIGHT_SHIFT];
        character = ID_US_ScanToASCII(scan_code, shifted,
                                      wg_game.high_score_caps_lock);
        if (character >= 32 && character < 127
            && length < WL_MAX_HIGH_NAME
            && WL_HighScoreNameWidth(&wg_game.graphics, score->name) < 100U)
        {
            memmove(score->name + wg_game.high_score_cursor + 1U,
                    score->name + wg_game.high_score_cursor,
                    length - wg_game.high_score_cursor + 1U);
            score->name[wg_game.high_score_cursor++] = character;
        }
    }
    if (!WL_DrawHighScores(WG_ScreenBuffer, &wg_game.graphics,
                           wg_high_scores))
    {
        return 0;
    }
    return wg_game.high_score_entry < 0
           || WL_DrawHighScoreCursor(
               WG_ScreenBuffer, &wg_game.graphics, wg_high_scores,
               (unsigned)wg_game.high_score_entry,
               wg_game.high_score_cursor);
}

static int WG_GameSessionOpen(unsigned map_number)
{
    wg_maps_t maps;
    wg_map_t map;
    int success = 0;
    const uint8_t *music_data;
    size_t music_size;

    memset(&maps, 0, sizeof(maps));
    memset(&map, 0, sizeof(map));
    memset(&wg_game, 0, sizeof(wg_game));
    if (!WG_MapsOpen(&maps, &wg_data_set)
        || !WG_MapsLoad(&maps, map_number, &map)
        || !WG_LevelBuild(&map, &wg_game.level)
        || !WG_PagesOpen(&wg_game.pages, &wg_data_set)
        || !WG_WallCacheLoad(&wg_game.walls, &wg_game.pages)
        || !WG_GraphicsOpen(&wg_game.graphics, &wg_data_set)
        || !ID_SD_DigiBankOpen(&wg_game.digi_bank, &wg_game.pages)
        || !WG_AudioOpen(&wg_game.audio, &wg_data_set))
    {
        goto cleanup;
    }
    wg_game.level.shareware =
        wg_data_set.variant == WG_GAME_WOLF3D_SHAREWARE_14;
    wg_game.level.map_number = (uint8_t)map_number;
    wg_game.map_number = map_number;
    wg_game.level_start_score = wg_game.level.score;
    memcpy(WG_Palette, WG_WolfPalette, sizeof(WG_Palette));
    WG_ViewBuildTrigTables(&wg_game.view);
    if (!WG_ViewCalculateProjection(&wg_game.view, WG_MAX_VIEW_WIDTH,
                                    WG_FOCAL_LENGTH))
    {
        goto cleanup;
    }
    WL_PlayStateReset(&wg_game.play);
    wg_game.active = 1;
    if (!WG_GameSessionRender())
    {
        goto cleanup;
    }
    if (WG_AudioGetChunk(&wg_game.audio, WL_MusicChunkForMap(map_number),
                         &music_data, &music_size))
    {
        wg_game.music = ID_SD_MusicCreate(48000U);
        if (wg_game.music != NULL && WG_PCMInit(48000U, 2U)
            && ID_SD_MusicStart(wg_game.music, music_data, music_size))
        {
            wg_game.audio_active = 1U;
            if (!WG_GameSessionPumpAudio())
            {
                goto cleanup;
            }
        }
        else
        {
            WG_PCMShutdown();
            ID_SD_MusicDestroy(wg_game.music);
            wg_game.music = NULL;
        }
    }
    success = 1;

cleanup:
    WG_MapFree(&map);
    WG_MapsClose(&maps);
    if (!success)
    {
        WG_GameSessionClose();
        WG_ReportError("Unable to start the Wolf3D game session.");
    }
    return success;
}

static void WG_GameSessionInput(wl_input_t *input)
{
    memset(input, 0, sizeof(*input));
    input->up = wg_game.keys[WG_KEY_UP];
    input->down = wg_game.keys[WG_KEY_DOWN];
    input->left = wg_game.keys[WG_KEY_LEFT];
    input->right = wg_game.keys[WG_KEY_RIGHT];
    input->attack = (uint8_t)(wg_game.keys[WG_KEY_CONTROL]
                              || (wg_game.mouse_buttons & 1U) != 0U);
    input->use = (uint8_t)(wg_game.keys[WG_KEY_SPACE]
                           || (wg_game.mouse_buttons & 4U) != 0U);
    input->strafe = (uint8_t)(wg_game.keys[WG_KEY_ALT]
                              || (wg_game.mouse_buttons & 2U) != 0U);
    input->run = (uint8_t)(wg_game.keys[WG_KEY_LEFT_SHIFT]
                           || wg_game.keys[WG_KEY_RIGHT_SHIFT]);
    if (wg_game.keys[WG_KEY_1])
    {
        input->weapon = 1U;
    }
    else if (wg_game.keys[WG_KEY_2])
    {
        input->weapon = 2U;
    }
    else if (wg_game.keys[WG_KEY_3])
    {
        input->weapon = 3U;
    }
    else if (wg_game.keys[WG_KEY_4])
    {
        input->weapon = 4U;
    }
    input->mouse_x = (int16_t)(wg_game.mouse_x > INT16_MAX
                                   ? INT16_MAX
                               : wg_game.mouse_x < INT16_MIN
                                   ? INT16_MIN : wg_game.mouse_x);
    input->mouse_y = (int16_t)(wg_game.mouse_y > INT16_MAX
                                   ? INT16_MAX
                               : wg_game.mouse_y < INT16_MIN
                                   ? INT16_MIN : wg_game.mouse_y);
    wg_game.mouse_x = 0;
    wg_game.mouse_y = 0;
}

static unsigned WG_GameSessionNextMap(void)
{
    return WG_NextMapNumber(wg_game.map_number,
                            wg_game.level.secret_level != 0U);
}

static int WG_GameSessionReload(unsigned map_number, int died)
{
    wg_campaign_state_t state;
    wl_intermission_t level_ratios[8];
    uint32_t start_score = died ? wg_game.level_start_score
                                : wg_game.level.score;

    WG_CampaignCapture(&state, &wg_game.level);
    memcpy(level_ratios, wg_game.level_ratios, sizeof(level_ratios));
    if (died && state.lives == 0U)
    {
        return WG_GameSessionBeginHighScores();
    }
    WG_GameSessionClose();
    if (!WG_GameSessionOpen(map_number))
    {
        return 0;
    }
    if (!WG_CampaignApply(&wg_game.level, &state,
                          start_score, died))
    {
        return 0;
    }
    memcpy(wg_game.level_ratios, level_ratios, sizeof(level_ratios));
    wg_game.level_start_score = start_score;
    return WG_GameSessionRender();
}

static int WG_GameSessionTick(void)
{
    wl_input_t input;
    size_t sound;

    if (wg_game.game_over || wg_game.victory)
    {
        return 1;
    }
    if (wg_game.level.player_dead)
    {
        if (wg_game.death_phase == WG_DEATH_ROTATE)
        {
            if (WL_DeathRotateStep(&wg_game.level,
                                   wg_game.death_target_angle, 2U))
            {
                wg_game.death_phase = WG_DEATH_FIZZLE_PENDING;
            }
        }
        else if (wg_game.death_phase == WG_DEATH_FIZZLE_PENDING)
        {
            if (!WG_GameSessionRender())
            {
                return 0;
            }
            memcpy(WG_Palette, WG_WolfPalette, sizeof(WG_Palette));
            memset(wg_game.death_source, 4, sizeof(wg_game.death_source));
            WG_FizzleStart(&wg_game.death_fizzle);
            wg_game.death_phase = WG_DEATH_FIZZLE;
        }
        else if (wg_game.death_phase == WG_DEATH_FIZZLE)
        {
            if (WG_FizzleStep(&wg_game.death_fizzle,
                              wg_game.death_source, WG_ScreenBuffer,
                              WG_SCREEN_WIDTH, WG_PLAY_VIEW_HEIGHT,
                              (WG_SCREEN_WIDTH * WG_SCREEN_HEIGHT) / 70U))
            {
                wg_game.death_phase = WG_DEATH_HOLD;
                wg_game.death_hold_tics = 0U;
            }
        }
        else if (wg_game.death_phase == WG_DEATH_HOLD)
        {
            int sound_playing = wg_game.audio_active
                && (ID_SD_EffectPlaying(wg_game.music)
                    || ID_SD_DigitalPlaying(wg_game.music));

            if (wg_game.death_hold_tics < 100U)
            {
                ++wg_game.death_hold_tics;
            }
            if ((wg_game.death_acknowledged
                 || wg_game.death_hold_tics >= 100U)
                && !sound_playing)
            {
                return WG_GameSessionReload(wg_game.map_number, 1);
            }
        }
        return 1;
    }

    WG_GameSessionInput(&input);
    if (!WL_PlayTick(&wg_game.level, &wg_game.view, &wg_game.play, &input))
    {
        return 0;
    }
    WL_UpdatePaletteShifts(&wg_game.level, WG_Palette);
    if (wg_game.audio_active)
    {
        if (wg_game.sound_positioned
            && ID_SD_DigitalPlaying(wg_game.music))
        {
            uint8_t left;
            uint8_t right;

            if (WG_SoundPosition(&wg_game.level, &wg_game.view,
                                 wg_game.sound_x, wg_game.sound_y,
                                 &left, &right))
            {
                (void)ID_SD_DigitalSetPosition(wg_game.music, left, right);
            }
        }
        for (sound = 0U; sound < wg_game.level.sound_event_count; ++sound)
        {
            const uint8_t *data;
            size_t size;
            const wg_sound_event_t *event =
                &wg_game.level.sound_events[sound];
            unsigned sound_number = event->sound;
            size_t chunk = 87U + sound_number;
            int digital_number = ID_SD_DigitalNumberForSound(sound_number);

            if (WG_AudioGetChunk(&wg_game.audio, chunk, &data, &size))
            {
                int played = 0;

                if (digital_number >= 0 && size >= 6U)
                {
                    uint8_t *digital_data;
                    size_t digital_length;
                    uint8_t left = 0U;
                    uint8_t right = 0U;

                    if (ID_SD_DigiBankLoad(&wg_game.digi_bank,
                                           (size_t)digital_number,
                                           &digital_data, &digital_length))
                    {
                        if (event->positioned
                            && !WG_SoundPosition(
                                &wg_game.level, &wg_game.view,
                                event->x, event->y, &left, &right))
                        {
                            left = 0U;
                            right = 0U;
                        }
                        played = ID_SD_DigitalStart(
                            wg_game.music, digital_data, digital_length,
                            WG_ReadLE16(data + 4U), left, right);
                        free(digital_data);
                        if (played)
                        {
                            wg_game.sound_positioned = event->positioned;
                            wg_game.sound_x = event->x;
                            wg_game.sound_y = event->y;
                        }
                    }
                }
                if (!played)
                {
                    (void)ID_SD_EffectStart(wg_game.music, data, size);
                }
            }
        }
    }
    WG_ClearSoundEvents(&wg_game.level);
    if (wg_game.level.level_completed)
    {
        if (wg_game.level.victory_flag)
        {
            return WG_GameSessionBeginVictory();
        }
        return WG_GameSessionBeginIntermission();
    }
    if (wg_game.level.player_dead)
    {
        wg_game.death_target_angle = WL_DeathTargetAngle(&wg_game.level);
        wg_game.death_phase = WG_DEATH_ROTATE;
        memset(wg_game.keys, 0, sizeof(wg_game.keys));
        wg_game.mouse_buttons = 0U;
        wg_game.mouse_x = 0;
        wg_game.mouse_y = 0;
    }
    return 1;
}

static int WG_LoadInitialPlayView(unsigned map_number, int open_doors,
                                  int guard_view, int boss_view,
                                  int patrol_view, int alert_view,
                                  int chase_view, int fire_view, int bite_view,
                                  int boss_fire_view,
                                  int needle_view,
                                  int rocket_view,
                                  int flame_view,
                                  int pushwall_view,
                                  int death_view,
                                  int boss_death_view,
                                  int player_fire_view,
                                  int pickup_view,
                                  int door_use_view,
                                  int pause_view,
                                  int intermission_view,
                                  int damage_flash_view,
                                  int bonus_flash_view,
                                  int player_death_view,
                                  int victory_view,
                                  int high_score_view,
                                  int high_score_entry_view,
                                  unsigned actor_tics,
                                  unsigned forward_tics)
{
    wg_maps_t maps;
    wg_map_t map;
    wg_level_t level;
    wg_pages_t pages;
    wg_wall_cache_t walls;
    wg_view_tables_t view;
    wg_graphics_t graphics;
    wl_status_t status;
    wg_wall_hit_t hits[WG_MAX_VIEW_WIDTH];
    uint8_t visible_tiles[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    int success = 0;

    memset(&maps, 0, sizeof(maps));
    memset(&map, 0, sizeof(map));
    memset(&pages, 0, sizeof(pages));
    memset(&walls, 0, sizeof(walls));
    memset(&view, 0, sizeof(view));
    memset(&graphics, 0, sizeof(graphics));
    if (!WG_MapsOpen(&maps, &wg_data_set)
        || !WG_MapsLoad(&maps, map_number, &map)
        || !WG_LevelBuild(&map, &level)
        || !WG_PagesOpen(&pages, &wg_data_set)
        || !WG_WallCacheLoad(&walls, &pages)
        || !WG_GraphicsOpen(&graphics, &wg_data_set))
    {
        goto cleanup;
    }
    level.shareware = wg_data_set.variant == WG_GAME_WOLF3D_SHAREWARE_14;
    level.map_number = (uint8_t)map_number;
    if (open_doors)
    {
        uint8_t door;

        for (door = 0; door < level.door_count; ++door)
        {
            level.doors[door].position = 0xffffU;
            level.doors[door].action = WG_DOOR_OPEN;
        }
    }
    if (!chase_view && !fire_view && !bite_view && !boss_fire_view
        && !needle_view
        && !rocket_view
        && !flame_view
        && !pushwall_view
        && !death_view
        && !boss_death_view
        && !player_fire_view
        && !WL_TickActors(&level, actor_tics))
    {
        goto cleanup;
    }
    if (guard_view || alert_view || chase_view || fire_view || death_view
        || player_fire_view || player_death_view)
    {
        wg_actor_t *actor = WG_FindGuardViewActor(&level);
        size_t player_tile;

        if (actor == NULL)
        {
            goto cleanup;
        }
        level.player_x = actor->x - 3 * WG_FIXED_ONE;
        level.player_y = actor->y;
        level.player_angle = 0U;
        level.player_tile_x = (uint8_t)(level.player_x / WG_FIXED_ONE);
        level.player_tile_y = (uint8_t)(level.player_y / WG_FIXED_ONE);
        player_tile = (size_t)actor->tile_y * WG_LEVEL_SIZE
                      + actor->tile_x - 3U;
        if (level.tiles[player_tile] != 0U)
        {
            goto cleanup;
        }
    }
    if (boss_view || boss_fire_view || boss_death_view)
    {
        wg_actor_t *actor = WG_FindBossViewActor(&level);

        if (!WG_SetActorViewPose(&level, actor, 3))
        {
            goto cleanup;
        }
    }
    if (patrol_view)
    {
        const wg_actor_t *actor = WG_FindPatrolViewActor(&level);

        if (!WG_SetActorViewPose(&level, actor, 3))
        {
            goto cleanup;
        }
    }
    if ((alert_view || chase_view)
        && (!WL_TickAwareness(&level, 1U, 0)
            || !WL_TickAwareness(&level, 64U, 0)))
    {
        goto cleanup;
    }
    if (chase_view && !WL_TickActors(&level, actor_tics))
    {
        goto cleanup;
    }
    if (fire_view)
    {
        wg_actor_t *actor = WG_FindGuardViewActor(&level);

        if (actor == NULL || actor->attack_shape == 0U)
        {
            goto cleanup;
        }
        actor->state = WG_STATE_SHOOT2;
        actor->tic_count = 1;
        actor->shape = (uint16_t)(actor->attack_shape + 1U);
        actor->flags |= WG_ACTOR_FLAG_ATTACK_MODE | WG_ACTOR_FLAG_VISIBLE;
        if (!WL_TickActors(&level, 1U))
        {
            goto cleanup;
        }
    }
    if (bite_view)
    {
        wg_actor_t *actor = WG_FindDogViewActor(&level);

        if (!WG_SetActorViewPose(&level, actor, 2)
            || actor->attack_shape == 0U)
        {
            goto cleanup;
        }
        actor->state = WG_STATE_DOG_JUMP2;
        actor->tic_count = 1;
        actor->shape = (uint16_t)(actor->attack_shape + 1U);
        actor->flags |= WG_ACTOR_FLAG_ATTACK_MODE;
        WG_RandomSeed(&level.random, 0U);
        if (!WL_TickActors(&level, 1U))
        {
            goto cleanup;
        }
    }
    if (boss_fire_view)
    {
        wg_actor_t *actor = WG_FindBossViewActor(&level);

        if (actor == NULL || actor->attack_shape == 0U)
        {
            goto cleanup;
        }
        actor->state = WG_STATE_SHOOT2;
        actor->tic_count = 1;
        actor->shape = (uint16_t)(actor->attack_shape + 1U);
        actor->flags |= WG_ACTOR_FLAG_ATTACK_MODE | WG_ACTOR_FLAG_VISIBLE;
        WG_RandomSeed(&level.random, 0U);
        if (!WL_TickActors(&level, 1U))
        {
            goto cleanup;
        }
    }
    if (needle_view)
    {
        wg_actor_t *actor = WG_FindActorClass(&level, WG_ACTOR_SCHABBS);
        int32_t actor_x;
        int32_t actor_y;
        uint8_t actor_tile_x;
        uint8_t actor_tile_y;

        if (!WG_SetActorViewPose(&level, actor, 2)
            || actor->attack_shape == 0U)
        {
            goto cleanup;
        }
        actor_x = actor->x;
        actor_y = actor->y;
        actor_tile_x = actor->tile_x;
        actor_tile_y = actor->tile_y;
        actor->state = WG_STATE_SHOOT2;
        actor->tic_count = 1;
        actor->shape = (uint16_t)(actor->attack_shape + 1U);
        actor->flags |= WG_ACTOR_FLAG_ATTACK_MODE;
        WG_RandomSeed(&level.random, 0U);
        if (!WL_TickActors(&level, 1U))
        {
            goto cleanup;
        }
        actor->x = actor_x;
        actor->y = actor_y;
        actor->tile_x = actor_tile_x;
        actor->tile_y = actor_tile_y;
        actor->state = WG_STATE_SHOOT2;
        actor->tic_count = 100;
        actor->shape = (uint16_t)(actor->attack_shape + 1U);
        if (!WL_TickActors(&level, 8U))
        {
            goto cleanup;
        }
    }
    if (rocket_view)
    {
        wg_actor_t *actor = WG_FindActorClass(&level, WG_ACTOR_GIFT);
        int32_t actor_x;
        int32_t actor_y;
        uint8_t actor_tile_x;
        uint8_t actor_tile_y;

        if (!WG_SetActorViewPose(&level, actor, 3)
            || actor->attack_shape == 0U)
        {
            goto cleanup;
        }
        actor_x = actor->x;
        actor_y = actor->y;
        actor_tile_x = actor->tile_x;
        actor_tile_y = actor->tile_y;
        actor->state = WG_STATE_SHOOT2;
        actor->tic_count = 1;
        actor->shape = (uint16_t)(actor->attack_shape + 1U);
        actor->flags |= WG_ACTOR_FLAG_ATTACK_MODE;
        WG_RandomSeed(&level.random, 0U);
        if (!WL_TickActors(&level, 1U))
        {
            goto cleanup;
        }
        actor->x = actor_x;
        actor->y = actor_y;
        actor->tile_x = actor_tile_x;
        actor->tile_y = actor_tile_y;
        actor->state = WG_STATE_SHOOT2;
        actor->tic_count = 100;
        actor->shape = (uint16_t)(actor->attack_shape + 1U);
        if (!WL_TickActors(&level, 10U))
        {
            goto cleanup;
        }
    }
    if (flame_view)
    {
        wg_actor_t *actor = WG_FindActorClass(&level, WG_ACTOR_FAKE);
        unsigned burst;

        if (!WG_SetActorViewPose(&level, actor, 2)
            || actor->attack_shape == 0U)
        {
            goto cleanup;
        }
        actor->state = WG_STATE_SHOOT1;
        actor->tic_count = 1;
        actor->shape = actor->attack_shape;
        actor->flags |= WG_ACTOR_FLAG_ATTACK_MODE;
        if (!WL_TickActors(&level, 1U))
        {
            goto cleanup;
        }
        for (burst = 0U; burst < 3U; ++burst)
        {
            if (!WL_TickActors(&level, 8U))
            {
                goto cleanup;
            }
        }
    }
    if (pushwall_view && !WG_SetPushWallView(&level))
    {
        goto cleanup;
    }
    if (pickup_view && !WG_SetPickupView(&level))
    {
        goto cleanup;
    }
    if (door_use_view && !WG_SetDoorUseView(&level))
    {
        goto cleanup;
    }
    if (death_view)
    {
        wg_actor_t *actor = WG_FindGuardViewActor(&level);
        size_t actor_index;

        if (actor == NULL)
        {
            goto cleanup;
        }
        actor_index = (size_t)(actor - level.actors);
        if (!WL_DamageActor(&level, actor_index,
                            (unsigned)actor->hit_points)
            || !WL_TickActors(&level, 30U))
        {
            goto cleanup;
        }
    }
    if (boss_death_view)
    {
        wg_actor_t *actor = WG_FindBossViewActor(&level);
        size_t actor_index;

        if (actor == NULL)
        {
            goto cleanup;
        }
        actor_index = (size_t)(actor - level.actors);
        if (!WL_DamageActor(&level, actor_index,
                            (unsigned)actor->hit_points)
            || !WL_TickActors(&level, 30U))
        {
            goto cleanup;
        }
    }
    if (player_fire_view)
    {
        WG_ViewBuildTrigTables(&view);
        if (!WG_ViewCalculateProjection(&view, WG_MAX_VIEW_WIDTH,
                                        WG_FOCAL_LENGTH)
            || !WG_RenderStaticView(WG_ScreenBuffer, &level, &view, &walls,
                                    0, 0, level.player_x, level.player_y,
                                    level.player_angle, hits, visible_tiles)
            || !WL_DrawScaleds(WG_ScreenBuffer, &pages, &level, &view, hits,
                               visible_tiles, level.player_x, level.player_y,
                               level.player_angle))
        {
            goto cleanup;
        }
        WG_RandomSeed(&level.random, 1U);
        if (!WL_StartAttack(&level)
            || !WL_TickPlayerAttack(&level, 12U, 0))
        {
            goto cleanup;
        }
    }
    if (forward_tics != 0U)
    {
        wl_play_state_t play;
        wl_input_t input;
        unsigned tic;

        memset(&input, 0, sizeof(input));
        input.up = 1U;
        WL_PlayStateReset(&play);
        WG_ViewBuildTrigTables(&view);
        for (tic = 0U; tic < forward_tics; ++tic)
        {
            if (!WL_PlayTick(&level, &view, &play, &input))
            {
                goto cleanup;
            }
        }
    }
    if (damage_flash_view)
    {
        WL_TakeDamage(&level, 40U);
    }
    if (bonus_flash_view)
    {
        level.bonus_count = 18U;
    }
    WG_ViewBuildTrigTables(&view);
    if (!WG_ViewCalculateProjection(&view, WG_MAX_VIEW_WIDTH,
                                    WG_FOCAL_LENGTH)
        || !WG_RenderStaticView(WG_ScreenBuffer, &level, &view, &walls,
                                0, 0, level.player_x, level.player_y,
                                level.player_angle, hits, visible_tiles)
        || !WL_DrawScaleds(WG_ScreenBuffer, &pages, &level, &view, hits,
                           visible_tiles,
                           level.player_x, level.player_y,
                           level.player_angle)
        || !WL_DrawPlayerWeapon(WG_ScreenBuffer, &pages,
                                level.player_weapon, level.weapon_frame))
    {
        goto cleanup;
    }
    WL_StatusDefaults(&status);
    status.score = level.score;
    status.health = level.player_health;
    status.ammo = level.player_ammo;
    status.weapon = level.player_weapon;
    status.lives = level.player_lives;
    status.keys = level.player_keys;
    if (!WL_DrawStatusBar(WG_ScreenBuffer, &graphics, &status))
    {
        goto cleanup;
    }
    if (pause_view && !WL_DrawPaused(WG_ScreenBuffer, &graphics))
    {
        goto cleanup;
    }
    if (intermission_view)
    {
        wl_intermission_t intermission;

        level.time_count = 75U * 70U;
        level.kill_count = level.kill_total;
        level.secret_count = level.secret_total;
        level.treasure_count = level.treasure_total;
        if (!WL_IntermissionCalculate(&level, map_number, &intermission)
            || !WL_DrawLevelCompleted(WG_ScreenBuffer, &graphics, &level,
                                      map_number, &intermission))
        {
            goto cleanup;
        }
    }
    if (victory_view)
    {
        wl_intermission_t ratios[8];
        wl_victory_t victory;
        unsigned floor;

        memset(ratios, 0, sizeof(ratios));
        for (floor = 0U; floor < 8U; ++floor)
        {
            ratios[floor].seconds = 75U;
            ratios[floor].kill_ratio = 100U;
            ratios[floor].secret_ratio = 100U;
            ratios[floor].treasure_ratio = 100U;
        }
        if (!WL_VictoryCalculate(ratios, &victory)
            || !WL_DrawVictory(WG_ScreenBuffer, &graphics,
                               &level, &victory))
        {
            goto cleanup;
        }
    }
    if (high_score_view)
    {
        wl_high_score_t scores[WL_MAX_HIGH_SCORES];

        WL_HighScoresDefault(scores);
        if (!WL_DrawHighScores(WG_ScreenBuffer, &graphics, scores))
        {
            goto cleanup;
        }
    }
    if (high_score_entry_view)
    {
        wl_high_score_t scores[WL_MAX_HIGH_SCORES];
        int entry;

        WL_HighScoresDefault(scores);
        entry = WL_HighScoreInsert(scores, 20000U, 9U, 0U);
        if (entry < 0)
        {
            goto cleanup;
        }
        memcpy(scores[entry].name, "BJ", 3U);
        if (!WL_DrawHighScores(WG_ScreenBuffer, &graphics, scores)
            || !WL_DrawHighScoreCursor(WG_ScreenBuffer, &graphics, scores,
                                       (unsigned)entry, 2U))
        {
            goto cleanup;
        }
    }
    if (damage_flash_view || bonus_flash_view)
    {
        WL_UpdatePaletteShifts(&level, WG_Palette);
    }
    if (player_death_view)
    {
        const wg_actor_t *actor = WG_FindGuardViewActor(&level);
        wg_fizzle_t fizzle;
        uint8_t red[WG_SCREEN_WIDTH * WG_PLAY_VIEW_HEIGHT];
        unsigned frame;

        if (actor == NULL)
        {
            goto cleanup;
        }
        level.killer_x = actor->x;
        level.killer_y = actor->y;
        while (!WL_DeathRotateStep(&level, WL_DeathTargetAngle(&level), 2U))
        {
        }
        if (!WG_RenderStaticView(WG_ScreenBuffer, &level, &view, &walls,
                                 0, 0, level.player_x, level.player_y,
                                 level.player_angle, hits, visible_tiles)
            || !WL_DrawScaleds(WG_ScreenBuffer, &pages, &level, &view, hits,
                               visible_tiles, level.player_x, level.player_y,
                               level.player_angle))
        {
            goto cleanup;
        }
        memset(red, 4, sizeof(red));
        WG_FizzleStart(&fizzle);
        for (frame = 0U; frame < 35U; ++frame)
        {
            (void)WG_FizzleStep(&fizzle, red, WG_ScreenBuffer,
                                WG_SCREEN_WIDTH, WG_PLAY_VIEW_HEIGHT,
                                (WG_SCREEN_WIDTH * WG_SCREEN_HEIGHT) / 70U);
        }
    }
    success = 1;

cleanup:
    WG_GraphicsClose(&graphics);
    WG_WallCacheFree(&walls);
    WG_PagesClose(&pages);
    WG_MapFree(&map);
    WG_MapsClose(&maps);
    if (!success)
    {
        WG_ReportError("Unable to render the initial Wolf3D play view.");
    }
    return success;
}

wg_result_t wolf3dgeneric_Create(int argc, char **argv)
{
    size_t framebuffer_size;
    const char *data_path;
    unsigned map_number;
    unsigned actor_tics;
    unsigned forward_tics;

    if (wg_initialized || argc < 0 || (argc > 0 && argv == NULL)
        || !WG_FindUnsignedArgument(argc, argv, "--map", 0U, 99U,
                                    &map_number)
        || !WG_FindUnsignedArgument(argc, argv, "--actor-tics", 0U,
                                    10000U, &actor_tics)
        || !WG_FindUnsignedArgument(argc, argv, "--forward-tics", 0U,
                                    10000U, &forward_tics))
    {
        return WG_RESULT_INVALID_ARGUMENT;
    }

    framebuffer_size = (size_t)WG_SCREEN_WIDTH * (size_t)WG_SCREEN_HEIGHT;
    WG_ScreenBuffer = (uint8_t *)calloc(framebuffer_size, sizeof(*WG_ScreenBuffer));
    if (WG_ScreenBuffer == NULL)
    {
        WG_ReportError("Unable to allocate the Wolf3D framebuffer.");
        return WG_RESULT_PLATFORM_ERROR;
    }

    memset(WG_Palette, 0, sizeof(WG_Palette));
    if (!WG_Init())
    {
        free(WG_ScreenBuffer);
        WG_ScreenBuffer = NULL;
        return WG_RESULT_PLATFORM_ERROR;
    }

    WG_SetWindowTitle("wolf3dgeneric bootstrap");
    wg_initialized = 1;
    data_path = WG_FindDataPath(argc, argv);
    if (data_path != NULL && !WG_LoadTitleScreen(data_path))
    {
        wolf3dgeneric_Shutdown();
        return WG_RESULT_PLATFORM_ERROR;
    }
    wg_start_map = map_number;
    WL_HighScoresDefault(wg_high_scores);
    if (data_path != NULL && WG_HasArgument(argc, argv, "--play-view")
        && WG_IsInteractive())
    {
        if (!WG_GameSessionOpen(map_number))
        {
            wolf3dgeneric_Shutdown();
            return WG_RESULT_PLATFORM_ERROR;
        }
    }
    else if (data_path != NULL && WG_HasArgument(argc, argv, "--play-view")
        && !WG_LoadInitialPlayView(
            map_number,
            WG_HasArgument(argc, argv, "--open-doors"),
            WG_HasArgument(argc, argv, "--guard-view"),
            WG_HasArgument(argc, argv, "--boss-view"),
            WG_HasArgument(argc, argv, "--patrol-view"),
            WG_HasArgument(argc, argv, "--alert-view"),
            WG_HasArgument(argc, argv, "--chase-view"),
            WG_HasArgument(argc, argv, "--fire-view"),
            WG_HasArgument(argc, argv, "--bite-view"),
            WG_HasArgument(argc, argv, "--boss-fire-view"),
            WG_HasArgument(argc, argv, "--needle-view"),
            WG_HasArgument(argc, argv, "--rocket-view"),
            WG_HasArgument(argc, argv, "--flame-view"),
            WG_HasArgument(argc, argv, "--pushwall-view"),
            WG_HasArgument(argc, argv, "--death-view"),
            WG_HasArgument(argc, argv, "--boss-death-view"),
            WG_HasArgument(argc, argv, "--player-fire-view"),
            WG_HasArgument(argc, argv, "--pickup-view"),
            WG_HasArgument(argc, argv, "--door-use-view"),
            WG_HasArgument(argc, argv, "--pause-view"),
            WG_HasArgument(argc, argv, "--intermission-view"),
            WG_HasArgument(argc, argv, "--damage-flash-view"),
            WG_HasArgument(argc, argv, "--bonus-flash-view"),
            WG_HasArgument(argc, argv, "--player-death-view"),
            WG_HasArgument(argc, argv, "--victory-view"),
            WG_HasArgument(argc, argv, "--high-score-view"),
            WG_HasArgument(argc, argv, "--high-score-entry-view"), actor_tics,
            forward_tics))
    {
        wolf3dgeneric_Shutdown();
        return WG_RESULT_PLATFORM_ERROR;
    }
    return WG_RESULT_OK;
}

wg_result_t wolf3dgeneric_Run(void)
{
    wg_event_t event;
    uint32_t last_ticks;
    uint32_t accumulator = 0U;

    if (!wg_initialized)
    {
        return WG_RESULT_INVALID_ARGUMENT;
    }

    WG_Present(WG_ScreenBuffer, WG_Palette);
    if (!wg_data_loaded || !WG_IsInteractive())
    {
        return WG_RESULT_NOT_IMPLEMENTED;
    }

    last_ticks = WG_GetTicksMs();
    for (;;)
    {
        while (WG_PollEvent(&event))
        {
            if (event.type == WG_EVENT_QUIT)
            {
                return WG_RESULT_QUIT;
            }
            if (event.type == WG_EVENT_KEY)
            {
                if (wg_game.active && wg_game.level.player_dead)
                {
                    if (wg_game.death_phase == WG_DEATH_HOLD
                        && event.pressed)
                    {
                        wg_game.death_acknowledged = 1U;
                    }
                    continue;
                }
                if (wg_game.active && wg_game.victory)
                {
                    if (event.key == WG_KEY_ESCAPE && event.pressed)
                    {
                        return WG_RESULT_QUIT;
                    }
                    continue;
                }
                if (wg_game.active && wg_game.high_scores)
                {
                    if (wg_game.high_score_entry < 0
                        && event.key == WG_KEY_ESCAPE && event.pressed)
                    {
                        return WG_RESULT_QUIT;
                    }
                    if (!WG_GameSessionHighScoreKey(event.key, event.pressed))
                    {
                        return WG_RESULT_PLATFORM_ERROR;
                    }
                    continue;
                }
                if (wg_game.active && wg_game.intermission && event.pressed)
                {
                    if (!WG_GameSessionReload(wg_game.next_map_number, 0))
                    {
                        return WG_RESULT_PLATFORM_ERROR;
                    }
                    last_ticks = WG_GetTicksMs();
                    accumulator = 0U;
                    continue;
                }
                if (wg_game.active && wg_game.paused && event.pressed)
                {
                    if (!WG_GameSessionSetPaused(0))
                    {
                        return WG_RESULT_PLATFORM_ERROR;
                    }
                    last_ticks = WG_GetTicksMs();
                    accumulator = 0U;
                    continue;
                }
                if (wg_game.active && event.key == WG_KEY_PAUSE
                    && event.pressed)
                {
                    if (!WG_GameSessionSetPaused(1))
                    {
                        return WG_RESULT_PLATFORM_ERROR;
                    }
                    last_ticks = WG_GetTicksMs();
                    accumulator = 0U;
                    continue;
                }
                if (event.key == WG_KEY_ESCAPE && event.pressed)
                {
                    return WG_RESULT_QUIT;
                }
                if (!wg_game.active && event.key == WG_KEY_ENTER
                    && event.pressed)
                {
                    if (!WG_GameSessionOpen(wg_start_map))
                    {
                        return WG_RESULT_PLATFORM_ERROR;
                    }
                    last_ticks = WG_GetTicksMs();
                    accumulator = 0U;
                    continue;
                }
                if (event.key < sizeof(wg_game.keys))
                {
                    wg_game.keys[event.key] = event.pressed != 0;
                }
            }
            else if (event.type == WG_EVENT_MOUSE_MOTION
                     && wg_game.active && !wg_game.paused
                     && !wg_game.intermission)
            {
                wg_game.mouse_x += event.x;
                wg_game.mouse_y += event.y;
            }
            else if (event.type == WG_EVENT_MOUSE_BUTTON
                     && wg_game.active && event.button >= 1U
                     && event.button <= 3U)
            {
                uint8_t mask = (uint8_t)(1U << (event.button - 1U));

                if (wg_game.level.player_dead)
                {
                    if (wg_game.death_phase == WG_DEATH_HOLD
                        && event.pressed)
                    {
                        wg_game.death_acknowledged = 1U;
                    }
                    continue;
                }
                if (wg_game.victory)
                {
                    continue;
                }
                if (wg_game.high_scores)
                {
                    continue;
                }
                if (wg_game.paused && event.pressed)
                {
                    if (!WG_GameSessionSetPaused(0))
                    {
                        return WG_RESULT_PLATFORM_ERROR;
                    }
                    last_ticks = WG_GetTicksMs();
                    accumulator = 0U;
                    continue;
                }
                if (wg_game.intermission && event.pressed)
                {
                    if (!WG_GameSessionReload(wg_game.next_map_number, 0))
                    {
                        return WG_RESULT_PLATFORM_ERROR;
                    }
                    last_ticks = WG_GetTicksMs();
                    accumulator = 0U;
                    continue;
                }
                if (event.pressed)
                {
                    wg_game.mouse_buttons |= mask;
                }
                else
                {
                    wg_game.mouse_buttons =
                        (uint8_t)(wg_game.mouse_buttons & ~mask);
                }
            }
        }
        if (wg_game.active)
        {
            uint32_t now = WG_GetTicksMs();
            uint32_t elapsed = now - last_ticks;
            unsigned ticks_run = 0U;

            last_ticks = now;
            if (wg_game.paused || wg_game.intermission)
            {
                accumulator = 0U;
                elapsed = 0U;
            }
            if (elapsed > 250U)
            {
                elapsed = 250U;
            }
            accumulator += elapsed * 70U;
            while (accumulator >= 1000U && ticks_run < 18U
                   && !wg_game.intermission)
            {
                if (!WG_GameSessionTick())
                {
                    WG_ReportError("The Wolf3D game simulation failed.");
                    return WG_RESULT_PLATFORM_ERROR;
                }
                accumulator -= 1000U;
                ++ticks_run;
            }
            if (ticks_run != 0U && !wg_game.intermission && !wg_game.victory
                && !wg_game.high_scores
                && wg_game.death_phase < WG_DEATH_FIZZLE
                && !WG_GameSessionRender())
            {
                WG_ReportError("The Wolf3D game renderer failed.");
                return WG_RESULT_PLATFORM_ERROR;
            }
            if (!WG_GameSessionPumpAudio())
            {
                WG_ReportError("The Wolf3D audio stream failed.");
                return WG_RESULT_PLATFORM_ERROR;
            }
        }
        WG_Present(WG_ScreenBuffer, WG_Palette);
        WG_SleepMs(wg_game.active ? 1U : 10U);
    }
}

void wolf3dgeneric_Shutdown(void)
{
    if (!wg_initialized)
    {
        return;
    }

    WG_GameSessionClose();
    WG_Shutdown();
    WG_DataClose(&wg_data_set);
    free(WG_ScreenBuffer);
    WG_ScreenBuffer = NULL;
    wg_data_loaded = 0;
    wg_start_map = 0U;
    wg_initialized = 0;
}
