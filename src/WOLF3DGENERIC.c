#include "WOLF3DGENERIC.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "WG_DATA.h"
#include "WG_AUDIO.h"
#include "WG_CONFIG.h"
#include "WG_FIXED.h"
#include "WG_FILE.h"
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
#include "WG_SAVE.h"
#include "WL_DRAW.h"
#include "WL_INTER.h"
#include "WL_TEXT.h"
#include "WL_MAIN.h"
#include "WL_MENU.h"
#include "WL_PLAY.h"
#include "WL_STATE.h"

uint8_t *WG_ScreenBuffer;
uint8_t WG_Palette[WG_PALETTE_COLORS * 3];

static int wg_initialized;
static int wg_data_loaded;
static unsigned wg_start_map;
static wg_data_set_t wg_data_set;
static wl_high_score_t wg_high_scores[WL_MAX_HIGH_SCORES];
static uint8_t wg_menu_active;
static uint8_t wg_front_scores;
static uint8_t wg_new_game_screen;
static uint8_t wg_sound_screen;
static uint8_t wg_control_screen;
static uint8_t wg_mouse_sensitivity_screen;
static uint8_t wg_customize_screen;
static uint8_t wg_change_view_screen;
static uint8_t wg_load_save_screen;
static uint8_t wg_help_active;
static size_t wg_help_page;
static wl_article_t wg_help_article;
static unsigned wg_menu_selection = WL_MAIN_MENU_DEFAULT_ITEM;
static unsigned wg_episode_selection;
static wg_difficulty_t wg_difficulty_selection = WG_DIFFICULTY_MEDIUM;
static unsigned wg_sound_selection;
static unsigned wg_control_selection;
static unsigned wg_mouse_adjustment = 5U;
static unsigned wg_saved_mouse_adjustment = 5U;
static uint8_t wg_mouse_enabled = 1U;
static unsigned wg_customize_selection;
static int wg_customize_edit_column = -1;
static uint8_t wg_customize_capture;
static uint8_t wg_mouse_bindings[WL_CUSTOM_BINDINGS] =
    { UINT8_MAX, 2U, 0U, 1U };
static uint16_t wg_action_keys[WL_CUSTOM_BINDINGS] =
    { WG_KEY_RIGHT_SHIFT, WG_KEY_SPACE, WG_KEY_CONTROL, WG_KEY_ALT };
static uint16_t wg_movement_keys[WL_CUSTOM_BINDINGS] =
    { WG_KEY_LEFT, WG_KEY_RIGHT, WG_KEY_UP, WG_KEY_DOWN };
static unsigned wg_view_size = WL_VIEW_SIZE_DEFAULT;
static unsigned wg_change_view_size = WL_VIEW_SIZE_DEFAULT;
static unsigned wg_saved_view_size = WL_VIEW_SIZE_DEFAULT;
static unsigned wg_save_selection;
static uint8_t wg_save_available[WL_SAVE_SLOTS];
static char wg_save_names[WL_SAVE_SLOTS][WL_SAVE_NAME_LENGTH + 1U];
static uint8_t wg_save_editing;
static uint8_t wg_save_confirm;
static uint8_t wg_save_caps_lock;
static char wg_save_original_name[WL_SAVE_NAME_LENGTH + 1U];
static uint8_t wg_adlib_effects = 1U;
static uint8_t wg_digitized_effects = 1U;
static uint8_t wg_music_enabled = 1U;
static uint8_t wg_config_ready;
static unsigned wg_next_demo;

typedef enum wg_confirm_action
{
    WG_CONFIRM_NONE = 0,
    WG_CONFIRM_NEW_GAME,
    WG_CONFIRM_END_GAME,
    WG_CONFIRM_QUICK_LOAD,
    WG_CONFIRM_QUIT
} wg_confirm_action_t;

static wg_confirm_action_t wg_confirm_action;
static uint8_t wg_quick_slot_valid;

typedef enum wg_attract_phase
{
    WG_ATTRACT_NONE = 0,
    WG_ATTRACT_TITLE,
    WG_ATTRACT_CREDITS,
    WG_ATTRACT_SCORES
} wg_attract_phase_t;

static wg_attract_phase_t wg_attract_phase;
static uint32_t wg_attract_deadline;
static wg_audio_t wg_front_audio;
static id_sd_music_t *wg_front_music;
static uint8_t wg_front_audio_active;
static unsigned wg_front_song = UINT_MAX;

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
    uint8_t control_panel;
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
    uint8_t end_text;
    size_t end_text_page;
    wl_article_t article;
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
    uint8_t demo_playback;
    uint8_t demo_tick_phase;
    uint8_t demo_finished;
    uint8_t *demo_data;
    wl_demo_t demo;
} wg_game_session_t;

static wg_game_session_t wg_game;

static unsigned WG_GameSessionNextMap(void);
static int WG_GameSessionRender(void);
static int WG_GameSessionOpen(unsigned map_number,
                              wg_difficulty_t difficulty);

static void WG_CloseFrontHelp(void)
{
    WL_ArticleClose(&wg_help_article);
    wg_help_active = 0U;
    wg_help_page = 0U;
}

static void WG_AttractSet(wg_attract_phase_t phase, uint32_t duration_ms)
{
    wg_attract_phase = phase;
    wg_attract_deadline = WG_GetTicksMs() + duration_ms;
}

static void WG_FrontMusicClose(void)
{
    WG_PCMShutdown();
    ID_SD_MusicDestroy(wg_front_music);
    WG_AudioClose(&wg_front_audio);
    memset(&wg_front_audio, 0, sizeof(wg_front_audio));
    wg_front_music = NULL;
    wg_front_audio_active = 0U;
    wg_front_song = UINT_MAX;
}

static void WG_FrontMusicStart(unsigned song)
{
    const uint8_t *data;
    size_t size;

    if (wg_game.active)
    {
        return;
    }
    if (wg_front_audio_active && wg_front_song == song)
    {
        return;
    }
    WG_FrontMusicClose();
    if (!WG_AudioOpen(&wg_front_audio, &wg_data_set))
    {
        WG_FrontMusicClose();
        return;
    }
    wg_front_music = ID_SD_MusicCreate(48000U);
    if (wg_front_music == NULL || !WG_PCMInit(48000U, 2U)
        || (wg_music_enabled
            && (!WG_AudioGetChunk(&wg_front_audio, 261U + song,
                                  &data, &size)
                || !ID_SD_MusicStart(wg_front_music, data, size))))
    {
        WG_FrontMusicClose();
        return;
    }
    wg_front_audio_active = 1U;
    wg_front_song = song;
}

static int WG_FrontMusicPump(void)
{
    int16_t samples[1024U * 2U];
    size_t frames;

    if (!wg_front_audio_active)
    {
        return 1;
    }
    while ((frames = WG_PCMWritableFrames()) != 0U)
    {
        if (frames > 1024U)
        {
            frames = 1024U;
        }
        if (!ID_SD_MusicRender(wg_front_music, samples, frames)
            || !WG_PCMSubmit(samples, frames))
        {
            return 0;
        }
    }
    return 1;
}

static void WG_FrontSoundPreviewNumber(unsigned sound_number)
{
    const wg_audio_t *audio;
    id_sd_music_t *mixer;
    const uint8_t *effect_data;
    size_t effect_size;
    int digital_number = ID_SD_DigitalNumberForSound(sound_number);

    if (wg_game.active)
    {
        if (!wg_game.audio_active || wg_game.music == NULL)
        {
            return;
        }
        audio = &wg_game.audio;
        mixer = wg_game.music;
    }
    else
    {
        if (!wg_front_audio_active || wg_front_music == NULL)
        {
            return;
        }
        audio = &wg_front_audio;
        mixer = wg_front_music;
    }
    if (!WG_AudioGetChunk(audio, 87U + sound_number,
                          &effect_data, &effect_size))
    {
        return;
    }
    if (wg_digitized_effects && digital_number >= 0)
    {
        wg_pages_t pages;
        id_sd_digi_bank_t bank;
        uint8_t *sample = NULL;
        size_t sample_size = 0U;
        int played = 0;

        memset(&pages, 0, sizeof(pages));
        memset(&bank, 0, sizeof(bank));
        if (WG_PagesOpen(&pages, &wg_data_set)
            && ID_SD_DigiBankOpen(&bank, &pages)
            && ID_SD_DigiBankLoad(&bank, (size_t)digital_number,
                                  &sample, &sample_size)
            && effect_size >= 6U)
        {
            played = ID_SD_DigitalStart(
                mixer, sample, sample_size,
                WG_ReadLE16(effect_data + 4U), 0U, 0U);
        }
        free(sample);
        WG_PagesClose(&pages);
        if (played)
        {
            return;
        }
    }
    if (wg_adlib_effects)
    {
        (void)ID_SD_EffectStart(mixer, effect_data, effect_size);
    }
}

static void WG_FrontSoundPreview(void)
{
    WG_FrontSoundPreviewNumber(WG_SOUND_ATTACK_PISTOL);
}

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

static int WG_DrawTitleScreen(void)
{
    wg_graphics_t graphics;

    memset(&graphics, 0, sizeof(graphics));
    if (!WG_GraphicsOpen(&graphics, &wg_data_set))
    {
        return 0;
    }
    if (!WG_GraphicsDecodeTitle(&graphics, wg_data_set.variant, WG_ScreenBuffer))
    {
        WG_ReportError("Unable to decode the Wolf3D title screen.");
        WG_GraphicsClose(&graphics);
        return 0;
    }
    WG_GraphicsClose(&graphics);
    memcpy(WG_Palette, WG_WolfPalette, sizeof(WG_Palette));
    wg_menu_active = 0U;
    wg_front_scores = 0U;
    wg_new_game_screen = 0U;
    wg_sound_screen = 0U;
    wg_control_screen = 0U;
    wg_mouse_sensitivity_screen = 0U;
    wg_customize_screen = 0U;
    WG_CloseFrontHelp();
    WG_AttractSet(WG_ATTRACT_TITLE, 15000U);
    WG_FrontMusicStart(7U);
    return 1;
}

static int WG_DrawCreditsScreen(void)
{
    wg_graphics_t graphics;
    uint8_t *pixels = NULL;
    uint16_t width;
    uint16_t height;
    size_t chunk;
    int result = 0;

    memset(&graphics, 0, sizeof(graphics));
    chunk = wg_data_set.variant == WG_GAME_WOLF3D_SHAREWARE_14 ? 101U : 89U;
    if (WG_GraphicsOpen(&graphics, &wg_data_set)
        && WG_GraphicsDecodePicture(&graphics, chunk, &pixels,
                                    &width, &height)
        && width == WG_SCREEN_WIDTH && height == WG_SCREEN_HEIGHT)
    {
        memcpy(WG_ScreenBuffer, pixels,
               (size_t)WG_SCREEN_WIDTH * WG_SCREEN_HEIGHT);
        memcpy(WG_Palette, WG_WolfPalette, sizeof(WG_Palette));
        WG_CloseFrontHelp();
        wg_menu_active = 0U;
        wg_front_scores = 0U;
        wg_new_game_screen = 0U;
        wg_sound_screen = 0U;
        wg_control_screen = 0U;
        wg_mouse_sensitivity_screen = 0U;
        wg_customize_screen = 0U;
        WG_AttractSet(WG_ATTRACT_CREDITS, 10000U);
        result = 1;
    }
    free(pixels);
    WG_GraphicsClose(&graphics);
    return result;
}

static int WG_LoadTitleScreen(const char *data_path)
{
    char title[160];
    int title_length;

    if (!WG_DataOpen(&wg_data_set, data_path))
    {
        WG_ReportError("The selected directory is not a supported Wolf3D v1.4 data set.");
        return 0;
    }
    if (!WG_DrawTitleScreen())
    {
        WG_ReportError("Unable to open the Wolf3D graphics resources.");
        WG_DataClose(&wg_data_set);
        return 0;
    }
    title_length = snprintf(title, sizeof(title), "wolf3dgeneric - %s",
                            WG_DataVariantName(wg_data_set.variant));
    if (title_length >= 0 && (size_t)title_length < sizeof(title))
    {
        WG_SetWindowTitle(title);
    }
    wg_data_loaded = 1;
    return 1;
}

static int WG_DrawMainMenuScreen(void)
{
    wg_graphics_t graphics;
    int result;

    wg_change_view_screen = 0U;
    wg_load_save_screen = 0U;
    memset(&graphics, 0, sizeof(graphics));
    if (!WG_GraphicsOpen(&graphics, &wg_data_set))
    {
        return 0;
    }
    result = WL_DrawMainMenu(WG_ScreenBuffer, &graphics,
                             wg_menu_selection, wg_game.control_panel);
    WG_GraphicsClose(&graphics);
    if (result)
    {
        WG_CloseFrontHelp();
        memcpy(WG_Palette, WG_WolfPalette, sizeof(WG_Palette));
        wg_menu_active = 1U;
        wg_front_scores = 0U;
        wg_new_game_screen = 0U;
        wg_sound_screen = 0U;
        wg_control_screen = 0U;
        wg_mouse_sensitivity_screen = 0U;
        wg_customize_screen = 0U;
        wg_attract_phase = WG_ATTRACT_NONE;
        WG_FrontMusicStart(14U);
    }
    return result;
}

static int WG_DrawConfirmScreen(wg_confirm_action_t action)
{
    static const char current_game[] =
        "You are currently in\n"
        "a game. Continuing will\n"
        "erase old game. Ok?";
    static const char end_game[] =
        "Are you sure you want\n"
        "to end the game you\n"
        "are playing? (Y or N):";
    static const char quit_game[] =
        "Are you sure you want\n"
        "to quit this great game?";
    char quick_load[80];
    const char *message;
    wg_graphics_t graphics;
    int result;

    if (action == WG_CONFIRM_NEW_GAME)
    {
        message = current_game;
    }
    else if (action == WG_CONFIRM_END_GAME)
    {
        message = end_game;
    }
    else if (action == WG_CONFIRM_QUICK_LOAD)
    {
        if (snprintf(quick_load, sizeof(quick_load),
                     "Load Game called\n\"%s\"?",
                     wg_save_names[wg_save_selection]) < 0)
        {
            return 0;
        }
        message = quick_load;
    }
    else if (action == WG_CONFIRM_QUIT)
    {
        message = quit_game;
    }
    else
    {
        return 0;
    }
    memset(&graphics, 0, sizeof(graphics));
    if (!WG_GraphicsOpen(&graphics, &wg_data_set))
    {
        return 0;
    }
    result = WL_DrawConfirm(WG_ScreenBuffer, &graphics, message);
    WG_GraphicsClose(&graphics);
    if (result)
    {
        wg_confirm_action = action;
    }
    return result;
}

static int WG_SavePath(char *path, size_t path_size, unsigned slot)
{
    int length;

    if (path == NULL || slot >= WL_SAVE_SLOTS)
    {
        return 0;
    }
    length = snprintf(path, path_size, "SAVEGAM%u%s",
                      slot, wg_data_set.extension);
    return length >= 0 && (size_t)length < path_size;
}

static int WG_ConfigPath(char *path, size_t path_size)
{
    int length;

    if (path == NULL)
    {
        return 0;
    }
    length = snprintf(path, path_size, "CONFIG%s", wg_data_set.extension);
    return length >= 0 && (size_t)length < path_size;
}

static void WG_ApplyConfig(const wg_config_t *config)
{
    memcpy(wg_high_scores, config->high_scores, sizeof(wg_high_scores));
    memcpy(wg_action_keys, config->action_keys, sizeof(wg_action_keys));
    memcpy(wg_movement_keys, config->movement_keys, sizeof(wg_movement_keys));
    memcpy(wg_mouse_bindings, config->mouse_bindings,
           sizeof(wg_mouse_bindings));
    wg_adlib_effects = config->adlib_effects;
    wg_digitized_effects = config->digitized_effects;
    wg_music_enabled = config->music_enabled;
    wg_mouse_enabled = config->mouse_enabled;
    wg_mouse_adjustment = config->mouse_adjustment;
    wg_saved_mouse_adjustment = config->mouse_adjustment;
    wg_view_size = config->view_size;
    wg_change_view_size = config->view_size;
    wg_saved_view_size = config->view_size;
}

static void WG_CaptureConfig(wg_config_t *config)
{
    memset(config, 0, sizeof(*config));
    memcpy(config->high_scores, wg_high_scores, sizeof(config->high_scores));
    memcpy(config->action_keys, wg_action_keys, sizeof(config->action_keys));
    memcpy(config->movement_keys, wg_movement_keys,
           sizeof(config->movement_keys));
    memcpy(config->mouse_bindings, wg_mouse_bindings,
           sizeof(config->mouse_bindings));
    config->adlib_effects = wg_adlib_effects;
    config->digitized_effects = wg_digitized_effects;
    config->music_enabled = wg_music_enabled;
    config->mouse_enabled = wg_mouse_enabled;
    config->mouse_adjustment = (uint8_t)wg_mouse_adjustment;
    config->view_size = (uint8_t)wg_view_size;
}

static void WG_LoadConfig(void)
{
    wg_config_t config;
    char path[32];

    WG_ConfigDefaults(&config);
    if (wg_data_loaded && WG_IsInteractive()
        && WG_ConfigPath(path, sizeof(path)))
    {
        (void)WG_ConfigReadFile(path, wg_data_set.variant, &config);
    }
    WG_ApplyConfig(&config);
    wg_config_ready = 1U;
}

static void WG_WriteConfig(void)
{
    wg_config_t config;
    char path[32];

    if (!wg_config_ready || !wg_data_loaded || !WG_IsInteractive()
        || !WG_ConfigPath(path, sizeof(path)))
    {
        return;
    }
    WG_CaptureConfig(&config);
    if (!WG_ConfigWriteFile(path, wg_data_set.variant, &config))
    {
        WG_ReportError("Unable to write the Wolf3D configuration file.");
    }
}

static void WG_RefreshSaveSlots(void)
{
    unsigned slot;

    memset(wg_save_available, 0, sizeof(wg_save_available));
    memset(wg_save_names, 0, sizeof(wg_save_names));
    for (slot = 0U; slot < WL_SAVE_SLOTS; ++slot)
    {
        char path[32];
        wg_file_buffer_t file;
        char name[WG_SAVE_NAME_BYTES];

        memset(&file, 0, sizeof(file));
        if (WG_SavePath(path, sizeof(path), slot)
            && WG_LoadFile(path, &file)
            && WG_SaveReadName(file.data, file.size, wg_data_set.variant,
                               name))
        {
            wg_save_available[slot] = 1U;
            memcpy(wg_save_names[slot], name,
                   sizeof(wg_save_names[slot]));
            wg_save_names[slot][WL_SAVE_NAME_LENGTH] = '\0';
        }
        WG_FreeFile(&file);
    }
}

static int WG_DrawLoadSaveScreen(unsigned mode, int refresh)
{
    wg_graphics_t graphics;
    int result;

    if ((mode != 1U && mode != 2U) || (mode == 2U && !wg_game.active))
    {
        return 0;
    }
    if (refresh)
    {
        WG_RefreshSaveSlots();
        wg_save_editing = 0U;
        wg_save_confirm = 0U;
    }
    memset(&graphics, 0, sizeof(graphics));
    if (!WG_GraphicsOpen(&graphics, &wg_data_set))
    {
        return 0;
    }
    result = WL_DrawLoadSaveMenu(
        WG_ScreenBuffer, &graphics, mode == 2U, wg_save_selection,
        wg_save_available, wg_save_names, wg_save_editing,
        wg_save_confirm);
    WG_GraphicsClose(&graphics);
    if (result)
    {
        WG_CloseFrontHelp();
        memcpy(WG_Palette, WG_WolfPalette, sizeof(WG_Palette));
        wg_menu_active = 0U;
        wg_front_scores = 0U;
        wg_new_game_screen = 0U;
        wg_sound_screen = 0U;
        wg_control_screen = 0U;
        wg_mouse_sensitivity_screen = 0U;
        wg_customize_screen = 0U;
        wg_change_view_screen = 0U;
        wg_load_save_screen = (uint8_t)mode;
        wg_attract_phase = WG_ATTRACT_NONE;
        WG_FrontMusicStart(14U);
    }
    return result;
}

static int WG_DrawSoundMenuScreen(void)
{
    wg_graphics_t graphics;
    int result;

    memset(&graphics, 0, sizeof(graphics));
    if (!WG_GraphicsOpen(&graphics, &wg_data_set))
    {
        return 0;
    }
    result = WL_DrawSoundMenu(WG_ScreenBuffer, &graphics,
                              wg_sound_selection, wg_adlib_effects,
                              wg_digitized_effects, wg_music_enabled);
    WG_GraphicsClose(&graphics);
    if (result)
    {
        WG_CloseFrontHelp();
        memcpy(WG_Palette, WG_WolfPalette, sizeof(WG_Palette));
        wg_menu_active = 0U;
        wg_front_scores = 0U;
        wg_new_game_screen = 0U;
        wg_sound_screen = 1U;
        wg_control_screen = 0U;
        wg_mouse_sensitivity_screen = 0U;
        wg_customize_screen = 0U;
        wg_attract_phase = WG_ATTRACT_NONE;
        WG_FrontMusicStart(14U);
    }
    return result;
}

static int WG_DrawControlMenuScreen(void)
{
    wg_graphics_t graphics;
    int result;

    memset(&graphics, 0, sizeof(graphics));
    if (!WG_GraphicsOpen(&graphics, &wg_data_set))
    {
        return 0;
    }
    result = WL_DrawControlMenu(WG_ScreenBuffer, &graphics,
                                wg_control_selection, wg_mouse_enabled);
    WG_GraphicsClose(&graphics);
    if (result)
    {
        WG_CloseFrontHelp();
        memcpy(WG_Palette, WG_WolfPalette, sizeof(WG_Palette));
        wg_menu_active = 0U;
        wg_front_scores = 0U;
        wg_new_game_screen = 0U;
        wg_sound_screen = 0U;
        wg_control_screen = 1U;
        wg_mouse_sensitivity_screen = 0U;
        wg_customize_screen = 0U;
        wg_attract_phase = WG_ATTRACT_NONE;
        WG_FrontMusicStart(14U);
    }
    return result;
}

static int WG_DrawMouseSensitivityScreen(void)
{
    wg_graphics_t graphics;
    int result;

    memset(&graphics, 0, sizeof(graphics));
    if (!WG_GraphicsOpen(&graphics, &wg_data_set))
    {
        return 0;
    }
    result = WL_DrawMouseSensitivity(WG_ScreenBuffer, &graphics,
                                     wg_mouse_adjustment);
    WG_GraphicsClose(&graphics);
    if (result)
    {
        WG_CloseFrontHelp();
        memcpy(WG_Palette, WG_WolfPalette, sizeof(WG_Palette));
        wg_menu_active = 0U;
        wg_front_scores = 0U;
        wg_new_game_screen = 0U;
        wg_sound_screen = 0U;
        wg_control_screen = 0U;
        wg_mouse_sensitivity_screen = 1U;
        wg_customize_screen = 0U;
        wg_attract_phase = WG_ATTRACT_NONE;
        WG_FrontMusicStart(14U);
    }
    return result;
}

static int WG_DrawCustomizeScreen(void)
{
    wg_graphics_t graphics;
    int result;

    memset(&graphics, 0, sizeof(graphics));
    if (!WG_GraphicsOpen(&graphics, &wg_data_set))
    {
        return 0;
    }
    result = WL_DrawCustomizeMenu(
        WG_ScreenBuffer, &graphics, wg_customize_selection,
        wg_mouse_enabled, wg_mouse_bindings, wg_action_keys,
        wg_movement_keys, wg_customize_edit_column,
        wg_customize_capture);
    WG_GraphicsClose(&graphics);
    if (result)
    {
        WG_CloseFrontHelp();
        memcpy(WG_Palette, WG_WolfPalette, sizeof(WG_Palette));
        wg_menu_active = 0U;
        wg_front_scores = 0U;
        wg_new_game_screen = 0U;
        wg_sound_screen = 0U;
        wg_control_screen = 0U;
        wg_mouse_sensitivity_screen = 0U;
        wg_customize_screen = 1U;
        wg_attract_phase = WG_ATTRACT_NONE;
        WG_FrontMusicStart(14U);
    }
    return result;
}

static int WG_DrawChangeViewScreen(void)
{
    wg_graphics_t graphics;
    int result;

    memset(&graphics, 0, sizeof(graphics));
    if (!WG_GraphicsOpen(&graphics, &wg_data_set))
    {
        return 0;
    }
    result = WL_DrawChangeView(WG_ScreenBuffer, &graphics,
                               wg_change_view_size);
    WG_GraphicsClose(&graphics);
    if (result)
    {
        WG_CloseFrontHelp();
        memcpy(WG_Palette, WG_WolfPalette, sizeof(WG_Palette));
        wg_menu_active = 0U;
        wg_front_scores = 0U;
        wg_new_game_screen = 0U;
        wg_sound_screen = 0U;
        wg_control_screen = 0U;
        wg_mouse_sensitivity_screen = 0U;
        wg_customize_screen = 0U;
        wg_change_view_screen = 1U;
        wg_attract_phase = WG_ATTRACT_NONE;
        WG_FrontMusicStart(14U);
    }
    return result;
}

static int WG_DrawFrontHighScores(int attract)
{
    wg_graphics_t graphics;
    int result;

    memset(&graphics, 0, sizeof(graphics));
    if (!WG_GraphicsOpen(&graphics, &wg_data_set))
    {
        return 0;
    }
    result = WL_DrawHighScores(WG_ScreenBuffer, &graphics, wg_high_scores);
    WG_GraphicsClose(&graphics);
    if (result)
    {
        WG_CloseFrontHelp();
        memcpy(WG_Palette, WG_WolfPalette, sizeof(WG_Palette));
        wg_menu_active = 0U;
        wg_front_scores = 1U;
        wg_new_game_screen = 0U;
        if (!attract)
        {
            wg_attract_phase = WG_ATTRACT_NONE;
            WG_FrontMusicStart(23U);
        }
    }
    return result;
}

static int WG_DrawAttractHighScores(void)
{
    if (!WG_DrawFrontHighScores(1))
    {
        return 0;
    }
    WG_AttractSet(WG_ATTRACT_SCORES, 10000U);
    return 1;
}

static int WG_DrawEpisodeMenuScreen(void)
{
    wg_graphics_t graphics;
    int result;

    memset(&graphics, 0, sizeof(graphics));
    if (!WG_GraphicsOpen(&graphics, &wg_data_set))
    {
        return 0;
    }
    result = WL_DrawEpisodeMenu(
        WG_ScreenBuffer, &graphics, wg_episode_selection,
        wg_data_set.variant == WG_GAME_WOLF3D_SHAREWARE_14);
    WG_GraphicsClose(&graphics);
    if (result)
    {
        WG_CloseFrontHelp();
        memcpy(WG_Palette, WG_WolfPalette, sizeof(WG_Palette));
        wg_menu_active = 0U;
        wg_front_scores = 0U;
        wg_new_game_screen = 1U;
        wg_sound_screen = 0U;
        wg_control_screen = 0U;
        wg_mouse_sensitivity_screen = 0U;
        wg_attract_phase = WG_ATTRACT_NONE;
        WG_FrontMusicStart(14U);
    }
    return result;
}

static int WG_DrawDifficultyMenuScreen(void)
{
    wg_graphics_t graphics;
    int result;

    memset(&graphics, 0, sizeof(graphics));
    if (!WG_GraphicsOpen(&graphics, &wg_data_set))
    {
        return 0;
    }
    result = WL_DrawDifficultyMenu(WG_ScreenBuffer, &graphics,
                                   wg_difficulty_selection);
    WG_GraphicsClose(&graphics);
    if (result)
    {
        WG_CloseFrontHelp();
        memcpy(WG_Palette, WG_WolfPalette, sizeof(WG_Palette));
        wg_menu_active = 0U;
        wg_front_scores = 0U;
        wg_new_game_screen = 2U;
        wg_sound_screen = 0U;
        wg_control_screen = 0U;
        wg_mouse_sensitivity_screen = 0U;
        wg_attract_phase = WG_ATTRACT_NONE;
        WG_FrontMusicStart(14U);
    }
    return result;
}

static int WG_DrawHelpScreen(void)
{
    wg_graphics_t graphics;
    int result;

    memset(&graphics, 0, sizeof(graphics));
    if (!WG_GraphicsOpen(&graphics, &wg_data_set))
    {
        return 0;
    }
    if (wg_help_article.text == NULL)
    {
        WG_CloseFrontHelp();
        if (!WL_ArticleOpenHelp(&wg_help_article, &graphics))
        {
            WG_GraphicsClose(&graphics);
            return 0;
        }
    }
    result = WL_ArticleRender(&wg_help_article, &graphics, wg_help_page,
                              WG_ScreenBuffer);
    WG_GraphicsClose(&graphics);
    if (result)
    {
        memcpy(WG_Palette, WG_WolfPalette, sizeof(WG_Palette));
        wg_menu_active = 0U;
        wg_front_scores = 0U;
        wg_new_game_screen = 0U;
        wg_sound_screen = 0U;
        wg_control_screen = 0U;
        wg_mouse_sensitivity_screen = 0U;
        wg_help_active = 1U;
        wg_attract_phase = WG_ATTRACT_NONE;
        WG_FrontMusicStart(0U);
    }
    return result;
}

static void WG_GameSessionClose(void)
{
    free(wg_game.demo_data);
    WL_ArticleClose(&wg_game.article);
    WG_PCMShutdown();
    ID_SD_MusicDestroy(wg_game.music);
    WG_AudioClose(&wg_game.audio);
    WG_GraphicsClose(&wg_game.graphics);
    WG_WallCacheFree(&wg_game.walls);
    WG_PagesClose(&wg_game.pages);
    memset(&wg_game, 0, sizeof(wg_game));
}

static int WG_GameSessionReturnToMenu(void)
{
    WG_GameSessionClose();
    wg_menu_selection = WL_MAIN_MENU_DEFAULT_ITEM;
    return WG_DrawMainMenuScreen();
}

static int WG_GameSessionLeaveControlPanel(void)
{
    wg_game.control_panel = 0U;
    wg_menu_active = 0U;
    wg_sound_screen = 0U;
    wg_control_screen = 0U;
    wg_mouse_sensitivity_screen = 0U;
    wg_customize_screen = 0U;
    wg_change_view_screen = 0U;
    wg_load_save_screen = 0U;
    wg_save_confirm = 0U;
    memset(wg_game.keys, 0, sizeof(wg_game.keys));
    wg_game.mouse_buttons = 0U;
    wg_game.mouse_x = 0;
    wg_game.mouse_y = 0;
    ID_SD_MusicSetPaused(wg_game.music, !wg_music_enabled);
    return WG_GameSessionRender();
}

static int WG_GameSessionOpenControlPanel(void)
{
    wg_game.control_panel = 1U;
    wg_menu_selection = 8U;
    memset(wg_game.keys, 0, sizeof(wg_game.keys));
    wg_game.mouse_buttons = 0U;
    wg_game.mouse_x = 0;
    wg_game.mouse_y = 0;
    ID_SD_MusicSetPaused(wg_game.music, 1);
    return WG_DrawMainMenuScreen();
}

static int WG_GameSessionSave(unsigned slot)
{
    wg_save_state_t state;
    char path[32];
    char name[WG_SAVE_NAME_BYTES];

    if (!wg_game.active || slot >= WL_SAVE_SLOTS
        || wg_save_names[slot][0] == '\0'
        || !WG_SavePath(path, sizeof(path), slot))
    {
        return 0;
    }
    memset(&state, 0, sizeof(state));
    memset(name, 0, sizeof(name));
    state.level = wg_game.level;
    state.map_number = wg_game.map_number;
    state.level_start_score = wg_game.level_start_score;
    memcpy(state.level_ratios, wg_game.level_ratios,
           sizeof(state.level_ratios));
    memcpy(name, wg_save_names[slot], strlen(wg_save_names[slot]));
    if (!WG_SaveWriteFile(path, name, wg_data_set.variant, &state))
    {
        return 0;
    }
    wg_save_available[slot] = 1U;
    wg_save_selection = slot;
    wg_quick_slot_valid = 1U;
    wg_save_editing = 0U;
    return WG_GameSessionLeaveControlPanel();
}

static int WG_GameSessionLoad(unsigned slot)
{
    wg_save_state_t state;
    char path[32];
    char name[WG_SAVE_NAME_BYTES];

    if (slot >= WL_SAVE_SLOTS || !wg_save_available[slot]
        || !WG_SavePath(path, sizeof(path), slot)
        || !WG_SaveReadFile(path, wg_data_set.variant, name, &state)
        || !WG_GameSessionOpen(state.map_number, state.level.difficulty))
    {
        return 0;
    }
    wg_game.level = state.level;
    wg_game.level.view_width = wg_game.view.view_width;
    wg_game.map_number = state.map_number;
    wg_game.level_start_score = state.level_start_score;
    memcpy(wg_game.level_ratios, state.level_ratios,
           sizeof(wg_game.level_ratios));
    WL_PlayStateReset(&wg_game.play);
    memset(wg_game.keys, 0, sizeof(wg_game.keys));
    wg_game.mouse_buttons = 0U;
    wg_game.mouse_x = 0;
    wg_game.mouse_y = 0;
    wg_save_selection = slot;
    wg_quick_slot_valid = 1U;
    return WG_GameSessionRender();
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
                                wg_game.level.weapon_frame,
                                wg_game.view.view_width))
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

static int WG_GameSessionBeginEndText(void)
{
    if (!WL_ArticleOpen(&wg_game.article, &wg_game.graphics,
                        wg_game.map_number / 10U)
        || !WL_ArticleRender(&wg_game.article, &wg_game.graphics, 0U,
                             WG_ScreenBuffer))
    {
        WL_ArticleClose(&wg_game.article);
        return 0;
    }
    wg_game.end_text = 1U;
    wg_game.end_text_page = 0U;
    memset(wg_game.keys, 0, sizeof(wg_game.keys));
    return 1;
}

static int WG_GameSessionBeginHighScores(void)
{
    const uint8_t *music_data;
    size_t music_size;

    WL_ArticleClose(&wg_game.article);
    wg_game.victory = 0U;
    wg_game.end_text = 0U;
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

static int WG_GameSessionOpen(unsigned map_number, wg_difficulty_t difficulty)
{
    wg_maps_t maps;
    wg_map_t map;
    int success = 0;
    const uint8_t *music_data;
    size_t music_size;

    if (wg_game.active)
    {
        WG_GameSessionClose();
    }
    else
    {
        WG_FrontMusicClose();
    }
    memset(&maps, 0, sizeof(maps));
    memset(&map, 0, sizeof(map));
    memset(&wg_game, 0, sizeof(wg_game));
    if (!WG_MapsOpen(&maps, &wg_data_set)
        || !WG_MapsLoad(&maps, map_number, &map)
        || !WG_LevelBuildForDifficulty(&map, difficulty, &wg_game.level)
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
    if (!WG_ViewCalculateProjection(&wg_game.view,
                                    (uint16_t)(wg_view_size * 16U),
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
    wg_game.music = ID_SD_MusicCreate(48000U);
    if (wg_game.music != NULL && WG_PCMInit(48000U, 2U))
    {
        wg_game.audio_active = 1U;
        if (wg_music_enabled
            && WG_AudioGetChunk(&wg_game.audio,
                                WL_MusicChunkForMap(map_number),
                                &music_data, &music_size)
            && !ID_SD_MusicStart(wg_game.music, music_data, music_size))
        {
            goto cleanup;
        }
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
    wg_menu_active = 0U;
    wg_front_scores = 0U;
    wg_new_game_screen = 0U;
    wg_sound_screen = 0U;
    wg_control_screen = 0U;
    wg_mouse_sensitivity_screen = 0U;
    wg_load_save_screen = 0U;
    wg_attract_phase = WG_ATTRACT_NONE;
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

static int WG_GameSessionOpenDemo(unsigned demo_number)
{
    wg_graphics_t graphics;
    wl_demo_t demo;
    uint8_t *data = NULL;
    size_t size = 0U;
    size_t first_chunk;

    if (demo_number >= 4U)
    {
        return 0;
    }
    memset(&graphics, 0, sizeof(graphics));
    memset(&demo, 0, sizeof(demo));
    first_chunk = wg_data_set.variant == WG_GAME_WOLF3D_SHAREWARE_14
                      ? 151U : 139U;
    if (!WG_GraphicsOpen(&graphics, &wg_data_set)
        || !WG_GraphicsDecodeChunk(&graphics, first_chunk + demo_number,
                                   &data, &size)
        || !WL_DemoOpen(&demo, data, size))
    {
        free(data);
        WG_GraphicsClose(&graphics);
        return 0;
    }
    WG_GraphicsClose(&graphics);
    if (!WG_GameSessionOpen(demo.map_number, WG_DIFFICULTY_HARD))
    {
        free(data);
        return 0;
    }
    wg_game.demo_data = data;
    if (!WL_DemoOpen(&wg_game.demo, data, size))
    {
        WG_GameSessionClose();
        return 0;
    }
    wg_game.demo_playback = 1U;
    wg_game.demo_tick_phase = 0U;
    wg_game.demo_finished = 0U;
    return 1;
}

static int WG_ActionPressed(unsigned action)
{
    uint16_t key;
    uint8_t button;

    if (action >= WL_CUSTOM_BINDINGS)
    {
        return 0;
    }
    key = wg_action_keys[action];
    button = wg_mouse_bindings[action];
    return (key < sizeof(wg_game.keys) && wg_game.keys[key])
        || (wg_mouse_enabled && button < 3U
            && (wg_game.mouse_buttons & (uint8_t)(1U << button)) != 0U);
}

static void WG_GameSessionInput(wl_input_t *input)
{
    memset(input, 0, sizeof(*input));
    input->up = wg_game.keys[wg_movement_keys[2U]];
    input->down = wg_game.keys[wg_movement_keys[3U]];
    input->left = wg_game.keys[wg_movement_keys[0U]];
    input->right = wg_game.keys[wg_movement_keys[1U]];
    input->run = (uint8_t)WG_ActionPressed(0U);
    input->use = (uint8_t)WG_ActionPressed(1U);
    input->attack = (uint8_t)WG_ActionPressed(2U);
    input->strafe = (uint8_t)WG_ActionPressed(3U);
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
    input->mouse_adjustment = (uint8_t)wg_mouse_adjustment;
    if (wg_mouse_enabled)
    {
        input->mouse_x = (int16_t)(wg_game.mouse_x > INT16_MAX
                                       ? INT16_MAX
                                   : wg_game.mouse_x < INT16_MIN
                                       ? INT16_MIN : wg_game.mouse_x);
        input->mouse_y = (int16_t)(wg_game.mouse_y > INT16_MAX
                                       ? INT16_MAX
                                   : wg_game.mouse_y < INT16_MIN
                                       ? INT16_MIN : wg_game.mouse_y);
    }
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
    wg_difficulty_t difficulty = wg_game.level.difficulty;
    uint32_t start_score = died ? wg_game.level_start_score
                                : wg_game.level.score;

    WG_CampaignCapture(&state, &wg_game.level);
    memcpy(level_ratios, wg_game.level_ratios, sizeof(level_ratios));
    if (died && state.lives == 0U)
    {
        return WG_GameSessionBeginHighScores();
    }
    WG_GameSessionClose();
    if (!WG_GameSessionOpen(map_number, difficulty))
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
    wl_demo_command_t demo_command;
    size_t sound;
    int demo_last_command = 0;

    if (wg_game.game_over || wg_game.victory)
    {
        return 1;
    }
    if (!wg_game.demo_playback && wg_game.level.player_dead)
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

    if (wg_game.demo_playback)
    {
        ++wg_game.demo_tick_phase;
        if (wg_game.demo_tick_phase < WL_DEMO_TICS)
        {
            return 1;
        }
        wg_game.demo_tick_phase = 0U;
        if (!WL_DemoNext(&wg_game.demo, &demo_command))
        {
            wg_game.demo_finished = 1U;
            return 1;
        }
        demo_last_command =
            wg_game.demo.position == wg_game.demo.command_count;
        if (!WL_PlayDemoCommand(&wg_game.level, &wg_game.view,
                                &wg_game.play, &demo_command))
        {
            return 0;
        }
    }
    else
    {
        WG_GameSessionInput(&input);
        if (!WL_PlayTick(&wg_game.level, &wg_game.view,
                         &wg_game.play, &input))
        {
            return 0;
        }
    }
    WL_UpdatePaletteShiftsForTics(
        &wg_game.level, WG_Palette,
        wg_game.demo_playback ? WL_DEMO_TICS : 1U);
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

                if (wg_digitized_effects && digital_number >= 0 && size >= 6U)
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
                if (!played && wg_adlib_effects)
                {
                    (void)ID_SD_EffectStart(wg_game.music, data, size);
                }
            }
        }
    }
    WG_ClearSoundEvents(&wg_game.level);
    if (wg_game.demo_playback)
    {
        if (demo_last_command || wg_game.level.level_completed
            || wg_game.level.player_dead)
        {
            wg_game.demo_finished = 1U;
        }
        return 1;
    }
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
                                  int end_text_view,
                                  int main_menu_view,
                                  int load_save_view,
                                  int sound_menu_view,
                                  int control_menu_view,
                                  int mouse_sensitivity_view,
                                  int customize_controls_view,
                                  int change_view,
                                  int credits_view,
                                  int help_view,
                                  int demo_view,
                                  unsigned demo_commands,
                                  int episode_menu_view,
                                  int difficulty_menu_view,
                                  int high_score_view,
                                  int high_score_entry_view,
                                  unsigned actor_tics,
                                  unsigned forward_tics,
                                  unsigned view_size)
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
    uint8_t *demo_data = NULL;
    size_t demo_size = 0U;
    wl_demo_t demo;
    int success = 0;

    memset(&maps, 0, sizeof(maps));
    memset(&map, 0, sizeof(map));
    memset(&pages, 0, sizeof(pages));
    memset(&walls, 0, sizeof(walls));
    memset(&view, 0, sizeof(view));
    memset(&graphics, 0, sizeof(graphics));
    memset(&demo, 0, sizeof(demo));
    if (!WG_GraphicsOpen(&graphics, &wg_data_set))
    {
        goto cleanup;
    }
    if (demo_view)
    {
        if (!WG_GraphicsDecodeChunk(
                &graphics,
                wg_data_set.variant == WG_GAME_WOLF3D_SHAREWARE_14
                    ? 151U : 139U,
                &demo_data, &demo_size)
            || !WL_DemoOpen(&demo, demo_data, demo_size))
        {
            goto cleanup;
        }
        map_number = demo.map_number;
    }
    if (map_number >= 60U
        || !WG_MapsOpen(&maps, &wg_data_set)
        || !WG_MapsLoad(&maps, map_number, &map)
        || !(demo_view
             ? WG_LevelBuildForDifficulty(&map, WG_DIFFICULTY_HARD, &level)
             : WG_LevelBuild(&map, &level))
        || !WG_PagesOpen(&pages, &wg_data_set)
        || !WG_WallCacheLoad(&walls, &pages))
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
        if (!WG_ViewCalculateProjection(&view,
                                        (uint16_t)(view_size * 16U),
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
    if (demo_view)
    {
        wl_play_state_t play;
        wl_demo_command_t command;
        unsigned command_number;

        WL_PlayStateReset(&play);
        WG_ViewBuildTrigTables(&view);
        for (command_number = 0U;
             command_number < demo_commands && WL_DemoNext(&demo, &command);
             ++command_number)
        {
            if (!WL_PlayDemoCommand(&level, &view, &play, &command))
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
    if (!WG_ViewCalculateProjection(&view, (uint16_t)(view_size * 16U),
                                    WG_FOCAL_LENGTH)
        || !WG_RenderStaticView(WG_ScreenBuffer, &level, &view, &walls,
                                0, 0, level.player_x, level.player_y,
                                level.player_angle, hits, visible_tiles)
        || !WL_DrawScaleds(WG_ScreenBuffer, &pages, &level, &view, hits,
                           visible_tiles,
                           level.player_x, level.player_y,
                           level.player_angle)
        || !WL_DrawPlayerWeapon(WG_ScreenBuffer, &pages,
                                level.player_weapon, level.weapon_frame,
                                view.view_width))
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
    if (end_text_view)
    {
        wl_article_t article;

        memset(&article, 0, sizeof(article));
        if (!WL_ArticleOpen(&article, &graphics, map_number / 10U)
            || !WL_ArticleRender(&article, &graphics, 0U,
                                 WG_ScreenBuffer))
        {
            WL_ArticleClose(&article);
            goto cleanup;
        }
        WL_ArticleClose(&article);
    }
    if (main_menu_view
        && !WL_DrawMainMenu(WG_ScreenBuffer, &graphics,
                            main_menu_view == 2U ? 8U
                                                 : WL_MAIN_MENU_DEFAULT_ITEM,
                            main_menu_view == 2U))
    {
        goto cleanup;
    }
    if (load_save_view != 0)
    {
        uint8_t available[WL_SAVE_SLOTS] = { 1U, 1U };
        char names[WL_SAVE_SLOTS][WL_SAVE_NAME_LENGTH + 1U] =
        {
            "E1L1 - CELL BLOCK",
            "SECRET FLOOR"
        };

        if (!WL_DrawLoadSaveMenu(WG_ScreenBuffer, &graphics,
                                 load_save_view == 2, 0U,
                                 available, names, 0, 0))
        {
            goto cleanup;
        }
    }
    if (sound_menu_view
        && !WL_DrawSoundMenu(WG_ScreenBuffer, &graphics, 0U, 1, 1, 1))
    {
        goto cleanup;
    }
    if (control_menu_view
        && !WL_DrawControlMenu(WG_ScreenBuffer, &graphics, 0U, 1))
    {
        goto cleanup;
    }
    if (mouse_sensitivity_view
        && !WL_DrawMouseSensitivity(WG_ScreenBuffer, &graphics, 5U))
    {
        goto cleanup;
    }
    if (customize_controls_view
        && !WL_DrawCustomizeMenu(
            WG_ScreenBuffer, &graphics, 0U, 1,
            wg_mouse_bindings, wg_action_keys, wg_movement_keys, -1, 0))
    {
        goto cleanup;
    }
    if (change_view
        && !WL_DrawChangeView(WG_ScreenBuffer, &graphics,
                              WL_VIEW_SIZE_DEFAULT))
    {
        goto cleanup;
    }
    if (credits_view)
    {
        uint8_t *pixels = NULL;
        uint16_t width;
        uint16_t height;
        size_t chunk = graphics.variant == WG_GAME_WOLF3D_SHAREWARE_14
                           ? 101U : 89U;

        if (!WG_GraphicsDecodePicture(&graphics, chunk, &pixels,
                                      &width, &height)
            || width != WG_SCREEN_WIDTH || height != WG_SCREEN_HEIGHT)
        {
            free(pixels);
            goto cleanup;
        }
        memcpy(WG_ScreenBuffer, pixels,
               (size_t)WG_SCREEN_WIDTH * WG_SCREEN_HEIGHT);
        free(pixels);
    }
    if (help_view)
    {
        wl_article_t article;

        memset(&article, 0, sizeof(article));
        if (!WL_ArticleOpenHelp(&article, &graphics)
            || !WL_ArticleRender(&article, &graphics, 0U,
                                 WG_ScreenBuffer))
        {
            WL_ArticleClose(&article);
            goto cleanup;
        }
        WL_ArticleClose(&article);
    }
    if (episode_menu_view
        && !WL_DrawEpisodeMenu(
            WG_ScreenBuffer, &graphics, map_number / 10U,
            graphics.variant == WG_GAME_WOLF3D_SHAREWARE_14))
    {
        goto cleanup;
    }
    if (difficulty_menu_view
        && !WL_DrawDifficultyMenu(WG_ScreenBuffer, &graphics,
                                  WG_DIFFICULTY_MEDIUM))
    {
        goto cleanup;
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
    free(demo_data);
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
    unsigned demo_commands;
    unsigned view_size;

    if (wg_initialized || argc < 0 || (argc > 0 && argv == NULL)
        || !WG_FindUnsignedArgument(argc, argv, "--map", 0U, 99U,
                                    &map_number)
        || !WG_FindUnsignedArgument(argc, argv, "--actor-tics", 0U,
                                    10000U, &actor_tics)
        || !WG_FindUnsignedArgument(argc, argv, "--forward-tics", 0U,
                                    10000U, &forward_tics)
        || !WG_FindUnsignedArgument(argc, argv, "--demo-commands", 70U,
                                    100000U, &demo_commands)
        || !WG_FindUnsignedArgument(argc, argv, "--view-size", 20U,
                                    20U, &view_size)
        || view_size < WL_VIEW_SIZE_MIN)
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
    wg_next_demo = 0U;
    data_path = WG_FindDataPath(argc, argv);
    if (data_path != NULL && !WG_LoadTitleScreen(data_path))
    {
        wolf3dgeneric_Shutdown();
        return WG_RESULT_PLATFORM_ERROR;
    }
    wg_start_map = map_number;
    WG_LoadConfig();
    if (WG_HasArgument(argc, argv, "--view-size"))
    {
        wg_view_size = view_size;
    }
    if (data_path != NULL && WG_HasArgument(argc, argv, "--play-view")
        && WG_IsInteractive())
    {
        if (!WG_GameSessionOpen(map_number, WG_DIFFICULTY_MEDIUM))
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
            WG_HasArgument(argc, argv, "--end-text-view"),
            WG_HasArgument(argc, argv, "--main-menu-view")
                ? 1 : WG_HasArgument(argc, argv, "--in-game-menu-view")
                          ? 2 : 0,
            WG_HasArgument(argc, argv, "--load-game-view")
                ? 1 : WG_HasArgument(argc, argv, "--save-game-view") ? 2 : 0,
            WG_HasArgument(argc, argv, "--sound-menu-view"),
            WG_HasArgument(argc, argv, "--control-menu-view"),
            WG_HasArgument(argc, argv, "--mouse-sensitivity-view"),
            WG_HasArgument(argc, argv, "--customize-controls-view"),
            WG_HasArgument(argc, argv, "--change-view"),
            WG_HasArgument(argc, argv, "--credits-view"),
            WG_HasArgument(argc, argv, "--help-view"),
            WG_HasArgument(argc, argv, "--demo-view"), demo_commands,
            WG_HasArgument(argc, argv, "--episode-menu-view"),
            WG_HasArgument(argc, argv, "--difficulty-menu-view"),
            WG_HasArgument(argc, argv, "--high-score-view"),
            WG_HasArgument(argc, argv, "--high-score-entry-view"), actor_tics,
            forward_tics, view_size))
    {
        wolf3dgeneric_Shutdown();
        return WG_RESULT_PLATFORM_ERROR;
    }
    return WG_RESULT_OK;
}

static wg_result_t WG_MainMenuActivate(void)
{
    switch (wg_menu_selection)
    {
        case 0U:
            if (wg_game.control_panel)
            {
                return WG_DrawConfirmScreen(WG_CONFIRM_NEW_GAME)
                           ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
            }
            wg_episode_selection = wg_start_map / 10U;
            if (wg_episode_selection >= 6U)
            {
                wg_episode_selection = 0U;
            }
            return WG_DrawEpisodeMenuScreen()
                       ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
        case 1U:
            wg_sound_selection = 0U;
            return WG_DrawSoundMenuScreen()
                       ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
        case 2U:
            wg_control_selection = 0U;
            return WG_DrawControlMenuScreen()
                       ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
        case 3U:
            wg_save_selection = 0U;
            return WG_DrawLoadSaveScreen(1U, 1)
                       ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
        case 4U:
            wg_save_selection = 0U;
            return WG_DrawLoadSaveScreen(2U, 1)
                       ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
        case 5U:
            wg_saved_view_size = wg_view_size;
            wg_change_view_size = wg_view_size > WL_VIEW_SIZE_MAX
                                      ? WL_VIEW_SIZE_MAX : wg_view_size;
            return WG_DrawChangeViewScreen()
                       ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
        case 7U:
            if (wg_game.control_panel)
            {
                return WG_DrawConfirmScreen(WG_CONFIRM_END_GAME)
                           ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
            }
            return WG_DrawFrontHighScores(0)
                       ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
        case 6U:
            wg_help_page = 0U;
            return WG_DrawHelpScreen()
                       ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
        case 8U:
            if (wg_game.control_panel)
            {
                return WG_GameSessionLeaveControlPanel()
                           ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
            }
            return WG_DrawTitleScreen()
                       ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
        case 9U:
            return WG_DrawConfirmScreen(WG_CONFIRM_QUIT)
                       ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
        default:
            return WG_RESULT_OK;
    }
}

static wg_result_t WG_ConfirmKey(uint16_t key)
{
    wg_confirm_action_t action = wg_confirm_action;
    int accepted = key == WG_KEY_Y;

    if (!accepted && key != WG_KEY_N && key != WG_KEY_ESCAPE)
    {
        return WG_RESULT_OK;
    }
    wg_confirm_action = WG_CONFIRM_NONE;
    if (!accepted)
    {
        if (wg_game.active && !wg_game.control_panel)
        {
            return WG_GameSessionRender()
                       ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
        }
        return WG_DrawMainMenuScreen()
                   ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
    }
    switch (action)
    {
        case WG_CONFIRM_NEW_GAME:
            wg_episode_selection = wg_start_map / 10U;
            if (wg_episode_selection >= 6U)
            {
                wg_episode_selection = 0U;
            }
            return WG_DrawEpisodeMenuScreen()
                       ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
        case WG_CONFIRM_END_GAME:
            return WG_GameSessionReturnToMenu()
                       ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
        case WG_CONFIRM_QUICK_LOAD:
            return WG_GameSessionLoad(wg_save_selection)
                       ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
        case WG_CONFIRM_QUIT:
            return WG_RESULT_QUIT;
        default:
            return WG_RESULT_PLATFORM_ERROR;
    }
}

static wg_result_t WG_GameQuickKey(uint16_t key)
{
    if (key == WG_KEY_F7)
    {
        return WG_DrawConfirmScreen(WG_CONFIRM_END_GAME)
                   ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
    }
    if (key == WG_KEY_F8 && wg_quick_slot_valid
        && wg_save_available[wg_save_selection])
    {
        return WG_GameSessionSave(wg_save_selection)
                   ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
    }
    if (key == WG_KEY_F9 && wg_quick_slot_valid
        && wg_save_available[wg_save_selection])
    {
        return WG_DrawConfirmScreen(WG_CONFIRM_QUICK_LOAD)
                   ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
    }
    if (key == WG_KEY_F10)
    {
        return WG_DrawConfirmScreen(WG_CONFIRM_QUIT)
                   ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
    }
    if (!WG_GameSessionOpenControlPanel())
    {
        return WG_RESULT_PLATFORM_ERROR;
    }
    switch (key)
    {
        case WG_KEY_F1:
            wg_help_page = 0U;
            return WG_DrawHelpScreen()
                       ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
        case WG_KEY_F2:
        case WG_KEY_F8:
            return WG_DrawLoadSaveScreen(2U, 1)
                       ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
        case WG_KEY_F3:
        case WG_KEY_F9:
            return WG_DrawLoadSaveScreen(1U, 1)
                       ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
        case WG_KEY_F4:
            wg_sound_selection = 0U;
            return WG_DrawSoundMenuScreen()
                       ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
        case WG_KEY_F5:
            wg_saved_view_size = wg_view_size;
            wg_change_view_size = wg_view_size > WL_VIEW_SIZE_MAX
                                      ? WL_VIEW_SIZE_MAX : wg_view_size;
            return WG_DrawChangeViewScreen()
                       ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
        case WG_KEY_F6:
            wg_control_selection = 0U;
            return WG_DrawControlMenuScreen()
                       ? WG_RESULT_OK : WG_RESULT_PLATFORM_ERROR;
        default:
            return WG_RESULT_PLATFORM_ERROR;
    }
}

static int WG_SoundMenuActivate(void)
{
    int preview = 0;

    switch (wg_sound_selection)
    {
        case 0U:
            wg_adlib_effects = 0U;
            break;
        case 2U:
            wg_adlib_effects = 1U;
            preview = 1;
            break;
        case 5U:
            wg_digitized_effects = 0U;
            break;
        case 7U:
            wg_digitized_effects = 1U;
            preview = 2;
            break;
        case 10U:
            wg_music_enabled = 0U;
            if (!wg_game.active)
            {
                WG_FrontMusicClose();
            }
            preview = 1;
            break;
        case 11U:
            wg_music_enabled = 1U;
            if (!wg_game.active)
            {
                WG_FrontMusicClose();
            }
            preview = 1;
            break;
        default:
            return 0;
    }
    if (!WG_DrawSoundMenuScreen())
    {
        return 0;
    }
    if (preview != 0)
    {
        WG_FrontSoundPreview();
    }
    return 1;
}

static int WG_ControlMenuActivate(void)
{
    switch (wg_control_selection)
    {
        case 0U:
            wg_mouse_enabled = (uint8_t)!wg_mouse_enabled;
            if (!WG_DrawControlMenuScreen())
            {
                return 0;
            }
            WG_FrontSoundPreview();
            return 1;
        case 4U:
            wg_saved_mouse_adjustment = wg_mouse_adjustment;
            return WG_DrawMouseSensitivityScreen();
        case 5U:
            wg_customize_selection = wg_mouse_enabled ? 0U : 6U;
            wg_customize_edit_column = -1;
            wg_customize_capture = 0U;
            return WG_DrawCustomizeScreen();
        default:
            return 0;
    }
}

static int WG_CustomizeAssignKey(uint16_t key)
{
    if (key >= sizeof(wg_game.keys) || wg_customize_edit_column < 0
        || wg_customize_edit_column >= (int)WL_CUSTOM_BINDINGS)
    {
        return 0;
    }
    if (wg_customize_selection == 6U)
    {
        wg_action_keys[wg_customize_edit_column] = key;
    }
    else if (wg_customize_selection == 8U)
    {
        wg_movement_keys[wg_customize_edit_column] = key;
    }
    else
    {
        return 0;
    }
    wg_customize_capture = 0U;
    WG_FrontSoundPreview();
    return WG_DrawCustomizeScreen();
}

static int WG_CustomizeAssignMouse(uint8_t button)
{
    unsigned action;

    if (button < 1U || button > 3U || wg_customize_selection != 0U
        || wg_customize_edit_column < 1
        || wg_customize_edit_column >= (int)WL_CUSTOM_BINDINGS)
    {
        return 0;
    }
    --button;
    for (action = 0U; action < WL_CUSTOM_BINDINGS; ++action)
    {
        if (wg_mouse_bindings[action] == button)
        {
            wg_mouse_bindings[action] = UINT8_MAX;
        }
    }
    wg_mouse_bindings[wg_customize_edit_column] = button;
    wg_customize_capture = 0U;
    WG_FrontSoundPreviewNumber(WG_SOUND_SHOOT_DOOR);
    return WG_DrawCustomizeScreen();
}

static int WG_AttractAdvance(void)
{
    switch (wg_attract_phase)
    {
        case WG_ATTRACT_TITLE:
            return WG_DrawCreditsScreen();
        case WG_ATTRACT_CREDITS:
            return WG_DrawAttractHighScores();
        case WG_ATTRACT_SCORES:
            if (!WG_GameSessionOpenDemo(wg_next_demo))
            {
                return 0;
            }
            wg_next_demo = (wg_next_demo + 1U) % 4U;
            return 1;
        default:
            return 1;
    }
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
            if (wg_confirm_action != WG_CONFIRM_NONE
                && event.type == WG_EVENT_MOUSE_BUTTON)
            {
                if (event.pressed)
                {
                    wg_result_t confirm_result = WG_ConfirmKey(
                        event.button == 1U ? WG_KEY_Y : WG_KEY_ESCAPE);

                    if (confirm_result != WG_RESULT_OK)
                    {
                        return confirm_result;
                    }
                    last_ticks = WG_GetTicksMs();
                    accumulator = 0U;
                }
                continue;
            }
            if (wg_confirm_action != WG_CONFIRM_NONE
                && event.type != WG_EVENT_KEY)
            {
                continue;
            }
            if (event.type == WG_EVENT_KEY)
            {
                if (wg_confirm_action != WG_CONFIRM_NONE)
                {
                    if (event.pressed)
                    {
                        wg_result_t confirm_result = WG_ConfirmKey(event.key);

                        if (confirm_result != WG_RESULT_OK)
                        {
                            return confirm_result;
                        }
                        last_ticks = WG_GetTicksMs();
                        accumulator = 0U;
                    }
                    continue;
                }
                if (wg_game.active && wg_game.demo_playback)
                {
                    if (event.pressed)
                    {
                        if (!WG_GameSessionReturnToMenu())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                        last_ticks = WG_GetTicksMs();
                        accumulator = 0U;
                    }
                    continue;
                }
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
                    if (!event.pressed)
                    {
                        continue;
                    }
                    if (!wg_game.end_text)
                    {
                        if (!WG_GameSessionBeginEndText())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                    else if (event.key == WG_KEY_ESCAPE)
                    {
                        if (!WG_GameSessionBeginHighScores())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                    else if (event.key == WG_KEY_LEFT
                             || event.key == WG_KEY_UP
                             || event.key == WG_KEY_HOME)
                    {
                        if (wg_game.end_text_page != 0U)
                        {
                            --wg_game.end_text_page;
                            if (!WL_ArticleRender(
                                    &wg_game.article, &wg_game.graphics,
                                    wg_game.end_text_page, WG_ScreenBuffer))
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                    }
                    else if (event.key == WG_KEY_RIGHT
                             || event.key == WG_KEY_DOWN
                             || event.key == WG_KEY_END
                             || event.key == WG_KEY_ENTER
                             || event.key == WG_KEY_SPACE)
                    {
                        if (wg_game.end_text_page + 1U
                            < wg_game.article.page_count)
                        {
                            ++wg_game.end_text_page;
                            if (!WL_ArticleRender(
                                    &wg_game.article, &wg_game.graphics,
                                    wg_game.end_text_page, WG_ScreenBuffer))
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                    }
                    continue;
                }
                if (wg_game.active && wg_game.high_scores)
                {
                    if (wg_game.high_score_entry < 0 && event.pressed)
                    {
                        if (!WG_GameSessionReturnToMenu())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                        last_ticks = WG_GetTicksMs();
                        accumulator = 0U;
                        continue;
                    }
                    if (!WG_GameSessionHighScoreKey(event.key, event.pressed))
                    {
                        return WG_RESULT_PLATFORM_ERROR;
                    }
                    if (wg_game.high_score_entry < 0)
                    {
                        if (!WG_GameSessionReturnToMenu())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                        last_ticks = WG_GetTicksMs();
                        accumulator = 0U;
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
                if (wg_game.active && !wg_game.control_panel
                    && event.key == WG_KEY_PAUSE
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
                if (!wg_game.active || wg_game.control_panel)
                {
                    wg_result_t menu_result = WG_RESULT_OK;

                    if (wg_load_save_screen && wg_save_editing
                        && (event.key == WG_KEY_LEFT_SHIFT
                            || event.key == WG_KEY_RIGHT_SHIFT))
                    {
                        wg_game.keys[event.key] = event.pressed != 0;
                    }
                    if (!event.pressed)
                    {
                        continue;
                    }
                    if (wg_load_save_screen)
                    {
                        if (wg_save_confirm)
                        {
                            if (event.key == WG_KEY_Y)
                            {
                                wg_save_confirm = 0U;
                                wg_save_editing = 1U;
                                if (!WG_DrawLoadSaveScreen(2U, 0))
                                {
                                    return WG_RESULT_PLATFORM_ERROR;
                                }
                            }
                            else if (event.key == WG_KEY_N
                                     || event.key == WG_KEY_ESCAPE)
                            {
                                wg_save_confirm = 0U;
                                if (!WG_DrawLoadSaveScreen(2U, 0))
                                {
                                    return WG_RESULT_PLATFORM_ERROR;
                                }
                            }
                        }
                        else if (event.key == WG_KEY_ESCAPE)
                        {
                            if (wg_save_editing)
                            {
                                memcpy(wg_save_names[wg_save_selection],
                                       wg_save_original_name,
                                       sizeof(wg_save_original_name));
                                wg_save_available[wg_save_selection] =
                                    wg_save_original_name[0] != '\0';
                                wg_save_editing = 0U;
                                if (!WG_DrawLoadSaveScreen(
                                        wg_load_save_screen, 0))
                                {
                                    return WG_RESULT_PLATFORM_ERROR;
                                }
                            }
                            else if (!WG_DrawMainMenuScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (!wg_save_editing
                                 && (event.key == WG_KEY_UP
                                     || event.key == WG_KEY_DOWN))
                        {
                            wg_save_selection = WL_LoadSaveMenuMove(
                                wg_save_selection,
                                event.key == WG_KEY_UP ? -1 : 1);
                            if (!WG_DrawLoadSaveScreen(
                                    wg_load_save_screen, 0))
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (event.key == WG_KEY_ENTER
                                 || event.key == WG_KEY_SPACE)
                        {
                            if (wg_load_save_screen == 1U)
                            {
                                if (wg_save_available[wg_save_selection]
                                    && !WG_GameSessionLoad(
                                        wg_save_selection))
                                {
                                    WG_ReportError(
                                        "Unable to load the selected game.");
                                    if (!WG_DrawLoadSaveScreen(1U, 1))
                                    {
                                        return WG_RESULT_PLATFORM_ERROR;
                                    }
                                }
                            }
                            else if (!wg_save_editing)
                            {
                                memcpy(wg_save_original_name,
                                       wg_save_names[wg_save_selection],
                                       sizeof(wg_save_original_name));
                                if (wg_save_available[wg_save_selection])
                                {
                                    wg_save_confirm = 1U;
                                }
                                else
                                {
                                    wg_save_names[wg_save_selection][0] = '\0';
                                    wg_save_available[wg_save_selection] = 1U;
                                    wg_save_editing = 1U;
                                }
                                if (!WG_DrawLoadSaveScreen(2U, 0))
                                {
                                    return WG_RESULT_PLATFORM_ERROR;
                                }
                            }
                            else if (wg_save_names[wg_save_selection][0]
                                     != '\0'
                                     && !WG_GameSessionSave(
                                         wg_save_selection))
                            {
                                WG_ReportError(
                                    "Unable to save the current game.");
                                if (!WG_DrawLoadSaveScreen(2U, 0))
                                {
                                    return WG_RESULT_PLATFORM_ERROR;
                                }
                            }
                        }
                        else if (wg_save_editing
                                 && event.key == WG_KEY_BACKSPACE)
                        {
                            size_t length = strlen(
                                wg_save_names[wg_save_selection]);
                            if (length != 0U)
                            {
                                wg_save_names[wg_save_selection]
                                                  [length - 1U] = '\0';
                                if (!WG_DrawLoadSaveScreen(2U, 0))
                                {
                                    return WG_RESULT_PLATFORM_ERROR;
                                }
                            }
                        }
                        else if (wg_save_editing
                                 && event.key == WG_KEY_CAPS_LOCK)
                        {
                            wg_save_caps_lock = (uint8_t)!wg_save_caps_lock;
                        }
                        else if (wg_save_editing)
                        {
                            char character = ID_US_ScanToASCII(
                                event.key,
                                wg_game.keys[WG_KEY_LEFT_SHIFT]
                                    || wg_game.keys[WG_KEY_RIGHT_SHIFT],
                                wg_save_caps_lock);
                            size_t length = strlen(
                                wg_save_names[wg_save_selection]);

                            if (character >= 32 && character < 127
                                && length < WL_SAVE_NAME_LENGTH)
                            {
                                wg_save_names[wg_save_selection][length] =
                                    character;
                                wg_save_names[wg_save_selection][length + 1U]
                                    = '\0';
                                if (!WG_DrawLoadSaveScreen(2U, 0))
                                {
                                    return WG_RESULT_PLATFORM_ERROR;
                                }
                            }
                        }
                    }
                    else if (wg_help_active)
                    {
                        if (event.key == WG_KEY_ESCAPE)
                        {
                            if (!WG_DrawMainMenuScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (event.key == WG_KEY_LEFT
                                 || event.key == WG_KEY_UP
                                 || event.key == WG_KEY_HOME)
                        {
                            if (wg_help_page != 0U)
                            {
                                --wg_help_page;
                                if (!WG_DrawHelpScreen())
                                {
                                    return WG_RESULT_PLATFORM_ERROR;
                                }
                            }
                        }
                        else if (event.key == WG_KEY_RIGHT
                                 || event.key == WG_KEY_DOWN
                                 || event.key == WG_KEY_END
                                 || event.key == WG_KEY_ENTER
                                 || event.key == WG_KEY_SPACE)
                        {
                            if (wg_help_page + 1U
                                < wg_help_article.page_count)
                            {
                                ++wg_help_page;
                                if (!WG_DrawHelpScreen())
                                {
                                    return WG_RESULT_PLATFORM_ERROR;
                                }
                            }
                        }
                    }
                    else if (wg_change_view_screen)
                    {
                        if (event.key == WG_KEY_LEFT
                            || event.key == WG_KEY_DOWN)
                        {
                            if (wg_change_view_size > WL_VIEW_SIZE_MIN)
                            {
                                --wg_change_view_size;
                            }
                            if (!WG_DrawChangeViewScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                            WG_FrontSoundPreviewNumber(0U);
                        }
                        else if (event.key == WG_KEY_RIGHT
                                 || event.key == WG_KEY_UP)
                        {
                            if (wg_change_view_size < WL_VIEW_SIZE_MAX)
                            {
                                ++wg_change_view_size;
                            }
                            if (!WG_DrawChangeViewScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                            WG_FrontSoundPreviewNumber(0U);
                        }
                        else if (event.key == WG_KEY_ESCAPE)
                        {
                            wg_view_size = wg_saved_view_size;
                            if (!WG_DrawMainMenuScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (event.key == WG_KEY_ENTER
                                 || event.key == WG_KEY_SPACE)
                        {
                            wg_view_size = wg_change_view_size;
                            if (!WG_DrawMainMenuScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                            WG_FrontSoundPreview();
                        }
                    }
                    else if (wg_customize_screen)
                    {
                        if (wg_customize_capture)
                        {
                            if (event.key == WG_KEY_ESCAPE)
                            {
                                wg_customize_capture = 0U;
                                if (!WG_DrawCustomizeScreen())
                                {
                                    return WG_RESULT_PLATFORM_ERROR;
                                }
                            }
                            else if (wg_customize_selection != 0U
                                     && event.key < sizeof(wg_game.keys)
                                     && !WG_CustomizeAssignKey(event.key))
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (wg_customize_edit_column >= 0)
                        {
                            int first = wg_customize_selection == 0U ? 1 : 0;

                            if (event.key == WG_KEY_LEFT)
                            {
                                --wg_customize_edit_column;
                                if (wg_customize_edit_column < first)
                                {
                                    wg_customize_edit_column = 3;
                                }
                                if (!WG_DrawCustomizeScreen())
                                {
                                    return WG_RESULT_PLATFORM_ERROR;
                                }
                            }
                            else if (event.key == WG_KEY_RIGHT)
                            {
                                ++wg_customize_edit_column;
                                if (wg_customize_edit_column > 3)
                                {
                                    wg_customize_edit_column = first;
                                }
                                if (!WG_DrawCustomizeScreen())
                                {
                                    return WG_RESULT_PLATFORM_ERROR;
                                }
                            }
                            else if (event.key == WG_KEY_ENTER
                                     || event.key == WG_KEY_SPACE)
                            {
                                wg_customize_capture = 1U;
                                if (!WG_DrawCustomizeScreen())
                                {
                                    return WG_RESULT_PLATFORM_ERROR;
                                }
                            }
                            else if (event.key == WG_KEY_ESCAPE
                                     || event.key == WG_KEY_UP
                                     || event.key == WG_KEY_DOWN)
                            {
                                wg_customize_edit_column = -1;
                                if (!WG_DrawCustomizeScreen())
                                {
                                    return WG_RESULT_PLATFORM_ERROR;
                                }
                            }
                        }
                        else if (event.key == WG_KEY_UP)
                        {
                            wg_customize_selection = WL_CustomMenuMove(
                                wg_customize_selection, -1,
                                wg_mouse_enabled);
                            if (!WG_DrawCustomizeScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (event.key == WG_KEY_DOWN)
                        {
                            wg_customize_selection = WL_CustomMenuMove(
                                wg_customize_selection, 1,
                                wg_mouse_enabled);
                            if (!WG_DrawCustomizeScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (event.key == WG_KEY_ESCAPE)
                        {
                            if (!WG_DrawControlMenuScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (event.key == WG_KEY_ENTER
                                 || event.key == WG_KEY_SPACE)
                        {
                            wg_customize_edit_column =
                                wg_customize_selection == 0U ? 1 : 0;
                            if (!WG_DrawCustomizeScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                    }
                    else if (wg_mouse_sensitivity_screen)
                    {
                        if ((event.key == WG_KEY_LEFT
                             || event.key == WG_KEY_UP)
                            && wg_mouse_adjustment != 0U)
                        {
                            --wg_mouse_adjustment;
                            if (!WG_DrawMouseSensitivityScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if ((event.key == WG_KEY_RIGHT
                                  || event.key == WG_KEY_DOWN)
                                 && wg_mouse_adjustment < 9U)
                        {
                            ++wg_mouse_adjustment;
                            if (!WG_DrawMouseSensitivityScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (event.key == WG_KEY_ESCAPE)
                        {
                            wg_mouse_adjustment =
                                wg_saved_mouse_adjustment;
                            if (!WG_DrawControlMenuScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (event.key == WG_KEY_ENTER
                                 || event.key == WG_KEY_SPACE)
                        {
                            if (!WG_DrawControlMenuScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                            WG_FrontSoundPreview();
                        }
                    }
                    else if (wg_control_screen)
                    {
                        if (event.key == WG_KEY_UP)
                        {
                            wg_control_selection = WL_ControlMenuMove(
                                wg_control_selection, -1,
                                wg_mouse_enabled);
                            if (!WG_DrawControlMenuScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (event.key == WG_KEY_DOWN)
                        {
                            wg_control_selection = WL_ControlMenuMove(
                                wg_control_selection, 1,
                                wg_mouse_enabled);
                            if (!WG_DrawControlMenuScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (event.key == WG_KEY_ESCAPE)
                        {
                            if (!WG_DrawMainMenuScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (event.key == WG_KEY_ENTER
                                 || event.key == WG_KEY_SPACE)
                        {
                            if (!WG_ControlMenuActivate())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                    }
                    else if (wg_sound_screen)
                    {
                        if (event.key == WG_KEY_UP)
                        {
                            wg_sound_selection = WL_SoundMenuMove(
                                wg_sound_selection, -1);
                            if (!WG_DrawSoundMenuScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (event.key == WG_KEY_DOWN)
                        {
                            wg_sound_selection = WL_SoundMenuMove(
                                wg_sound_selection, 1);
                            if (!WG_DrawSoundMenuScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (event.key == WG_KEY_ESCAPE)
                        {
                            if (!WG_DrawMainMenuScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (event.key == WG_KEY_ENTER
                                 || event.key == WG_KEY_SPACE)
                        {
                            if (!WG_SoundMenuActivate())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                    }
                    else if (wg_new_game_screen == 1U)
                    {
                        if (event.key == WG_KEY_UP)
                        {
                            wg_episode_selection = WL_EpisodeMenuMove(
                                wg_episode_selection, -1);
                            if (!WG_DrawEpisodeMenuScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (event.key == WG_KEY_DOWN)
                        {
                            wg_episode_selection = WL_EpisodeMenuMove(
                                wg_episode_selection, 1);
                            if (!WG_DrawEpisodeMenuScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (event.key == WG_KEY_ESCAPE)
                        {
                            if (!WG_DrawMainMenuScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if ((event.key == WG_KEY_ENTER
                                  || event.key == WG_KEY_SPACE)
                                 && (wg_data_set.variant
                                     != WG_GAME_WOLF3D_SHAREWARE_14
                                     || wg_episode_selection == 0U))
                        {
                            if (!WG_DrawDifficultyMenuScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                    }
                    else if (wg_new_game_screen == 2U)
                    {
                        if (event.key == WG_KEY_UP)
                        {
                            wg_difficulty_selection = (wg_difficulty_t)
                                WL_DifficultyMenuMove(
                                    wg_difficulty_selection, -1);
                            if (!WG_DrawDifficultyMenuScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (event.key == WG_KEY_DOWN)
                        {
                            wg_difficulty_selection = (wg_difficulty_t)
                                WL_DifficultyMenuMove(
                                    wg_difficulty_selection, 1);
                            if (!WG_DrawDifficultyMenuScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (event.key == WG_KEY_ESCAPE)
                        {
                            if (!WG_DrawEpisodeMenuScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (event.key == WG_KEY_ENTER
                                 || event.key == WG_KEY_SPACE)
                        {
                            if (!WG_GameSessionOpen(
                                    wg_episode_selection * 10U,
                                    wg_difficulty_selection))
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                    }
                    else if (wg_front_scores || !wg_menu_active)
                    {
                        if (!WG_DrawMainMenuScreen())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                    else if (event.key == WG_KEY_UP)
                    {
                        wg_menu_selection = WL_MainMenuMove(
                            wg_menu_selection, -1,
                            wg_game.control_panel);
                        if (!WG_DrawMainMenuScreen())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                    else if (event.key == WG_KEY_DOWN)
                    {
                        wg_menu_selection = WL_MainMenuMove(
                            wg_menu_selection, 1,
                            wg_game.control_panel);
                        if (!WG_DrawMainMenuScreen())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                    else if (event.key == WG_KEY_ESCAPE)
                    {
                        if (wg_game.control_panel)
                        {
                            if (!WG_GameSessionLeaveControlPanel())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (!WG_DrawTitleScreen())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                    else if (event.key == WG_KEY_ENTER
                             || event.key == WG_KEY_SPACE)
                    {
                        menu_result = WG_MainMenuActivate();
                        if (menu_result != WG_RESULT_OK)
                        {
                            return menu_result;
                        }
                    }
                    if (wg_game.active)
                    {
                        last_ticks = WG_GetTicksMs();
                        accumulator = 0U;
                    }
                    continue;
                }
                if (event.key == WG_KEY_ESCAPE && event.pressed)
                {
                    if (!WG_GameSessionOpenControlPanel())
                    {
                        return WG_RESULT_PLATFORM_ERROR;
                    }
                    last_ticks = WG_GetTicksMs();
                    accumulator = 0U;
                    continue;
                }
                if (event.pressed && event.key >= WG_KEY_F1
                    && event.key <= WG_KEY_F10)
                {
                    wg_result_t quick_result = WG_GameQuickKey(event.key);

                    if (quick_result != WG_RESULT_OK)
                    {
                        return quick_result;
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
                     && !wg_game.intermission && !wg_game.control_panel)
            {
                wg_game.mouse_x += event.x;
                wg_game.mouse_y += event.y;
            }
            else if (event.type == WG_EVENT_MOUSE_BUTTON
                     && (!wg_game.active || wg_game.control_panel)
                     && event.pressed)
            {
                if (wg_load_save_screen)
                {
                    if (event.button == 2U)
                    {
                        if (wg_save_confirm)
                        {
                            wg_save_confirm = 0U;
                            if (!WG_DrawLoadSaveScreen(2U, 0))
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (wg_save_editing)
                        {
                            memcpy(wg_save_names[wg_save_selection],
                                   wg_save_original_name,
                                   sizeof(wg_save_original_name));
                            wg_save_available[wg_save_selection] =
                                wg_save_original_name[0] != '\0';
                            wg_save_editing = 0U;
                            if (!WG_DrawLoadSaveScreen(
                                    wg_load_save_screen, 0))
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (!WG_DrawMainMenuScreen())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                    else if (event.button == 1U
                             && wg_load_save_screen == 1U
                             && wg_save_available[wg_save_selection]
                             && !WG_GameSessionLoad(wg_save_selection))
                    {
                        WG_ReportError("Unable to load the selected game.");
                        if (!WG_DrawLoadSaveScreen(1U, 1))
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                    else if (event.button == 1U
                             && wg_load_save_screen == 2U
                             && !wg_save_confirm
                             && wg_save_editing
                             && wg_save_names[wg_save_selection][0] != '\0'
                             && !WG_GameSessionSave(wg_save_selection))
                    {
                        WG_ReportError("Unable to save the current game.");
                        if (!WG_DrawLoadSaveScreen(2U, 0))
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                    else if (event.button == 1U
                             && wg_load_save_screen == 2U
                             && !wg_save_confirm
                             && !wg_save_editing)
                    {
                        memcpy(wg_save_original_name,
                               wg_save_names[wg_save_selection],
                               sizeof(wg_save_original_name));
                        if (wg_save_available[wg_save_selection])
                        {
                            wg_save_confirm = 1U;
                        }
                        else
                        {
                            wg_save_names[wg_save_selection][0] = '\0';
                            wg_save_available[wg_save_selection] = 1U;
                            wg_save_editing = 1U;
                        }
                        if (!WG_DrawLoadSaveScreen(2U, 0))
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                }
                else if (wg_help_active)
                {
                    if (event.button == 2U)
                    {
                        if (!WG_DrawMainMenuScreen())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                    else if (event.button == 1U
                             && wg_help_page + 1U
                                < wg_help_article.page_count)
                    {
                        ++wg_help_page;
                        if (!WG_DrawHelpScreen())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                }
                else if (wg_change_view_screen)
                {
                    if (event.button == 2U)
                    {
                        wg_view_size = wg_saved_view_size;
                        if (!WG_DrawMainMenuScreen())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                    else if (event.button == 1U)
                    {
                        wg_view_size = wg_change_view_size;
                        if (!WG_DrawMainMenuScreen())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                        WG_FrontSoundPreview();
                    }
                }
                else if (wg_customize_screen)
                {
                    if (wg_customize_capture
                        && wg_customize_selection == 0U)
                    {
                        if (!WG_CustomizeAssignMouse(event.button))
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                    else if (wg_customize_capture)
                    {
                        if (event.button == 2U)
                        {
                            wg_customize_capture = 0U;
                            if (!WG_DrawCustomizeScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                    }
                    else if (wg_customize_edit_column >= 0)
                    {
                        if (event.button == 2U)
                        {
                            wg_customize_edit_column = -1;
                            if (!WG_DrawCustomizeScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                        else if (event.button == 1U)
                        {
                            wg_customize_capture = 1U;
                            if (!WG_DrawCustomizeScreen())
                            {
                                return WG_RESULT_PLATFORM_ERROR;
                            }
                        }
                    }
                    else if (event.button == 2U)
                    {
                        if (!WG_DrawControlMenuScreen())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                    else if (event.button == 1U)
                    {
                        wg_customize_edit_column =
                            wg_customize_selection == 0U ? 1 : 0;
                        if (!WG_DrawCustomizeScreen())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                }
                else if (wg_mouse_sensitivity_screen)
                {
                    if (event.button == 2U)
                    {
                        wg_mouse_adjustment = wg_saved_mouse_adjustment;
                        if (!WG_DrawControlMenuScreen())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                    else if (event.button == 1U)
                    {
                        if (!WG_DrawControlMenuScreen())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                        WG_FrontSoundPreview();
                    }
                }
                else if (wg_control_screen)
                {
                    if (event.button == 2U)
                    {
                        if (!WG_DrawMainMenuScreen())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                    else if (event.button == 1U
                             && !WG_ControlMenuActivate())
                    {
                        return WG_RESULT_PLATFORM_ERROR;
                    }
                }
                else if (wg_sound_screen)
                {
                    if (event.button == 2U)
                    {
                        if (!WG_DrawMainMenuScreen())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                    else if (event.button == 1U
                             && !WG_SoundMenuActivate())
                    {
                        return WG_RESULT_PLATFORM_ERROR;
                    }
                }
                else if (wg_new_game_screen == 1U)
                {
                    if (event.button == 2U)
                    {
                        if (!WG_DrawMainMenuScreen())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                    else if (event.button == 1U
                             && (wg_data_set.variant
                                 != WG_GAME_WOLF3D_SHAREWARE_14
                                 || wg_episode_selection == 0U)
                             && !WG_DrawDifficultyMenuScreen())
                    {
                        return WG_RESULT_PLATFORM_ERROR;
                    }
                }
                else if (wg_new_game_screen == 2U)
                {
                    if (event.button == 2U)
                    {
                        if (!WG_DrawEpisodeMenuScreen())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                    else if (event.button == 1U
                             && !WG_GameSessionOpen(
                                 wg_episode_selection * 10U,
                                 wg_difficulty_selection))
                    {
                        return WG_RESULT_PLATFORM_ERROR;
                    }
                }
                else if (wg_front_scores || !wg_menu_active)
                {
                    if (!WG_DrawMainMenuScreen())
                    {
                        return WG_RESULT_PLATFORM_ERROR;
                    }
                }
                else if (event.button == 2U)
                {
                    if (wg_game.control_panel)
                    {
                        if (!WG_GameSessionLeaveControlPanel())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                    else if (!WG_DrawTitleScreen())
                    {
                        return WG_RESULT_PLATFORM_ERROR;
                    }
                }
                else if (event.button == 1U)
                {
                    wg_result_t menu_result = WG_MainMenuActivate();

                    if (menu_result != WG_RESULT_OK)
                    {
                        return menu_result;
                    }
                    if (wg_game.active)
                    {
                        last_ticks = WG_GetTicksMs();
                        accumulator = 0U;
                    }
                }
            }
            else if (event.type == WG_EVENT_MOUSE_BUTTON
                     && wg_game.active && !wg_game.control_panel
                     && event.button >= 1U
                     && event.button <= 3U)
            {
                uint8_t mask = (uint8_t)(1U << (event.button - 1U));

                if (wg_game.demo_playback)
                {
                    if (event.pressed)
                    {
                        if (!WG_GameSessionReturnToMenu())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                        last_ticks = WG_GetTicksMs();
                        accumulator = 0U;
                    }
                    continue;
                }
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
                    if (event.pressed && !wg_game.end_text)
                    {
                        if (!WG_GameSessionBeginEndText())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                    else if (event.pressed && event.button == 2U)
                    {
                        if (!WG_GameSessionBeginHighScores())
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
                    else if (event.pressed && event.button == 1U
                             && wg_game.end_text_page + 1U
                                < wg_game.article.page_count)
                    {
                        ++wg_game.end_text_page;
                        if (!WL_ArticleRender(
                                &wg_game.article, &wg_game.graphics,
                                wg_game.end_text_page, WG_ScreenBuffer))
                        {
                            return WG_RESULT_PLATFORM_ERROR;
                        }
                    }
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
        if (!wg_game.active && wg_attract_phase != WG_ATTRACT_NONE
            && (int32_t)(WG_GetTicksMs() - wg_attract_deadline) >= 0)
        {
            if (!WG_AttractAdvance())
            {
                WG_ReportError("The Wolf3D attract loop failed.");
                return WG_RESULT_PLATFORM_ERROR;
            }
            last_ticks = WG_GetTicksMs();
            accumulator = 0U;
        }
        if (wg_game.active)
        {
            uint32_t now = WG_GetTicksMs();
            uint32_t elapsed = now - last_ticks;
            unsigned ticks_run = 0U;

            last_ticks = now;
            if (wg_game.paused || wg_game.intermission
                || wg_game.control_panel
                || wg_confirm_action != WG_CONFIRM_NONE)
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
                if (wg_game.demo_finished)
                {
                    WG_GameSessionClose();
                    if (!WG_DrawTitleScreen())
                    {
                        return WG_RESULT_PLATFORM_ERROR;
                    }
                    last_ticks = WG_GetTicksMs();
                    accumulator = 0U;
                    ticks_run = 0U;
                    break;
                }
            }
            if (wg_game.active && ticks_run != 0U
                && !wg_game.intermission && !wg_game.victory
                && !wg_game.high_scores
                && wg_game.death_phase < WG_DEATH_FIZZLE
                && !WG_GameSessionRender())
            {
                WG_ReportError("The Wolf3D game renderer failed.");
                return WG_RESULT_PLATFORM_ERROR;
            }
            if (wg_game.active && !WG_GameSessionPumpAudio())
            {
                WG_ReportError("The Wolf3D audio stream failed.");
                return WG_RESULT_PLATFORM_ERROR;
            }
        }
        else if (!WG_FrontMusicPump())
        {
            WG_ReportError("The Wolf3D front-end audio stream failed.");
            return WG_RESULT_PLATFORM_ERROR;
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

    WG_CloseFrontHelp();
    WG_WriteConfig();
    WG_GameSessionClose();
    WG_FrontMusicClose();
    WG_Shutdown();
    WG_DataClose(&wg_data_set);
    free(WG_ScreenBuffer);
    WG_ScreenBuffer = NULL;
    wg_data_loaded = 0;
    wg_start_map = 0U;
    wg_next_demo = 0U;
    wg_change_view_screen = 0U;
    wg_load_save_screen = 0U;
    wg_save_selection = 0U;
    wg_save_editing = 0U;
    wg_save_confirm = 0U;
    wg_save_caps_lock = 0U;
    memset(wg_save_available, 0, sizeof(wg_save_available));
    memset(wg_save_names, 0, sizeof(wg_save_names));
    memset(wg_save_original_name, 0, sizeof(wg_save_original_name));
    wg_view_size = WL_VIEW_SIZE_DEFAULT;
    wg_change_view_size = WL_VIEW_SIZE_DEFAULT;
    wg_saved_view_size = WL_VIEW_SIZE_DEFAULT;
    wg_attract_phase = WG_ATTRACT_NONE;
    wg_attract_deadline = 0U;
    wg_config_ready = 0U;
    wg_confirm_action = WG_CONFIRM_NONE;
    wg_quick_slot_valid = 0U;
    wg_initialized = 0;
}
