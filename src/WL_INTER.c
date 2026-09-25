/* Portable LevelCompleted presentation derived from the original WL_INTER.C. */
#include "WL_INTER.h"

#include <stdio.h>
#include <string.h>

#include "ID_VL.h"
#include "ID_VH.h"
#include "WL_AGENT.h"

#define WL_INTERMISSION_BACKGROUND 127U
#define WL_INTERMISSION_PAR_AMOUNT 500U
#define WL_INTERMISSION_PERFECT_BONUS 10000U

typedef struct wl_intermission_chunks
{
    size_t guy;
    size_t colon;
    size_t zero;
    size_t percent;
    size_t letter_a;
    size_t exclamation;
    size_t time_code;
    size_t bj_win;
} wl_intermission_chunks_t;

typedef struct wl_high_score_chunks
{
    size_t time_code;
    size_t level;
    size_t name;
    size_t score;
    size_t title;
} wl_high_score_chunks_t;

static const wl_high_score_t wl_default_high_scores[WL_MAX_HIGH_SCORES] =
{
    { "id software-'92", 10000U, 1U, 0U },
    { "Adrian Carmack",   10000U, 1U, 0U },
    { "John Carmack",     10000U, 1U, 0U },
    { "Kevin Cloud",      10000U, 1U, 0U },
    { "Tom Hall",         10000U, 1U, 0U },
    { "John Romero",      10000U, 1U, 0U },
    { "Jay Wilbur",       10000U, 1U, 0U }
};

static const uint16_t wl_par_seconds[60] =
{
     90U, 120U, 120U, 210U, 180U, 180U, 150U, 150U,   0U,   0U,
     90U, 210U, 180U, 120U, 240U, 360U,  60U, 180U,   0U,   0U,
     90U,  90U, 150U, 150U, 210U, 150U, 120U, 360U,   0U,   0U,
    120U, 120U,  90U,  60U, 270U, 210U, 120U, 270U,   0U,   0U,
    150U,  90U, 150U, 150U, 240U, 180U, 270U, 210U,   0U,   0U,
    390U, 240U, 270U, 360U, 300U, 330U, 330U, 510U,   0U,   0U
};

static int WL_IntermissionChunks(wg_game_variant_t variant,
                                 wl_intermission_chunks_t *chunks)
{
    if (chunks == NULL)
    {
        return 0;
    }
    if (variant == WG_GAME_WOLF3D_FULL_GT_14)
    {
        chunks->guy = 43U;
    }
    else if (variant == WG_GAME_WOLF3D_SHAREWARE_14)
    {
        /* This late Apogee graph places the level-end lump at chunk 55. */
        chunks->guy = 55U;
    }
    else
    {
        return 0;
    }
    chunks->colon = chunks->guy + 1U;
    chunks->zero = chunks->guy + 2U;
    chunks->percent = chunks->zero + 10U;
    chunks->letter_a = chunks->percent + 1U;
    chunks->exclamation = chunks->letter_a + 26U;
    chunks->time_code = chunks->guy - 6U;
    chunks->bj_win = chunks->guy + 42U;
    return 1;
}

static int WL_HighScoreChunks(wg_game_variant_t variant,
                              wl_high_score_chunks_t *chunks)
{
    if (chunks == NULL)
    {
        return 0;
    }
    if (variant == WG_GAME_WOLF3D_FULL_GT_14)
    {
        chunks->time_code = 37U;
        chunks->title = 90U;
    }
    else if (variant == WG_GAME_WOLF3D_SHAREWARE_14)
    {
        chunks->time_code = 49U;
        chunks->title = 102U;
    }
    else
    {
        return 0;
    }
    chunks->level = chunks->time_code + 1U;
    chunks->name = chunks->time_code + 2U;
    chunks->score = chunks->time_code + 3U;
    return 1;
}

static int WL_IntermissionWritePixels(uint8_t *framebuffer,
                                     const wg_graphics_t *graphics,
                                     const wl_intermission_chunks_t *chunks,
                                     int x, int y, const char *text)
{
    int origin_x = x;
    int draw_x = origin_x;
    int draw_y = y;

    while (*text != '\0')
    {
        unsigned char character = (unsigned char)*text++;
        size_t chunk;

        if (character == '\n')
        {
            draw_x = origin_x;
            draw_y += 16;
            continue;
        }
        if (character == ' ')
        {
            draw_x += 16;
            continue;
        }
        if (character >= 'a' && character <= 'z')
        {
            character = (unsigned char)(character - ('a' - 'A'));
        }
        if (character >= '0' && character <= '9')
        {
            chunk = chunks->zero + (size_t)(character - '0');
        }
        else if (character >= 'A' && character <= 'Z')
        {
            chunk = chunks->letter_a + (size_t)(character - 'A');
        }
        else if (character == ':')
        {
            chunk = chunks->colon;
        }
        else if (character == '%')
        {
            chunk = chunks->percent;
        }
        else if (character == '!')
        {
            chunk = chunks->exclamation;
        }
        else
        {
            return 0;
        }
        if (!WG_VideoDrawPicture(framebuffer, graphics, chunk,
                                 draw_x, draw_y))
        {
            return 0;
        }
        draw_x += character == ':' || character == '!' ? 8 : 16;
    }
    return 1;
}

static int WL_IntermissionWrite(uint8_t *framebuffer,
                                const wg_graphics_t *graphics,
                                const wl_intermission_chunks_t *chunks,
                                int x, int y, const char *text)
{
    return WL_IntermissionWritePixels(framebuffer, graphics, chunks,
                                      x * 8, y * 8, text);
}

static uint8_t WL_Ratio(uint16_t count, uint16_t total)
{
    unsigned ratio = total == 0U ? 0U : (unsigned)count * 100U / total;

    return (uint8_t)(ratio > 100U ? 100U : ratio);
}

int WL_IntermissionCalculate(const wg_level_t *level,
                             unsigned map_number,
                             wl_intermission_t *intermission)
{
    unsigned floor;
    uint32_t time_left;

    if (level == NULL || intermission == NULL || map_number >= 60U)
    {
        return 0;
    }
    memset(intermission, 0, sizeof(*intermission));
    floor = map_number % 10U;
    intermission->seconds = level->time_count / 70U;
    if (intermission->seconds > 99U * 60U)
    {
        intermission->seconds = 99U * 60U;
    }
    intermission->par_seconds = wl_par_seconds[map_number];
    intermission->special_floor = floor >= 8U;
    if (intermission->special_floor)
    {
        intermission->bonus = 15000U;
        return 1;
    }

    intermission->kill_ratio = WL_Ratio(level->kill_count,
                                        level->kill_total);
    intermission->secret_ratio = WL_Ratio(level->secret_count,
                                          level->secret_total);
    intermission->treasure_ratio = WL_Ratio(level->treasure_count,
                                            level->treasure_total);
    time_left = intermission->seconds < intermission->par_seconds
                    ? intermission->par_seconds - intermission->seconds : 0U;
    intermission->bonus = time_left * WL_INTERMISSION_PAR_AMOUNT;
    if (intermission->kill_ratio == 100U)
    {
        intermission->bonus += WL_INTERMISSION_PERFECT_BONUS;
    }
    if (intermission->secret_ratio == 100U)
    {
        intermission->bonus += WL_INTERMISSION_PERFECT_BONUS;
    }
    if (intermission->treasure_ratio == 100U)
    {
        intermission->bonus += WL_INTERMISSION_PERFECT_BONUS;
    }
    return 1;
}

int WL_DrawLevelCompleted(uint8_t *framebuffer,
                          const wg_graphics_t *graphics,
                          const wg_level_t *level,
                          unsigned map_number,
                          const wl_intermission_t *intermission)
{
    wl_intermission_chunks_t chunks;
    wl_status_t status;
    char text[32];
    unsigned floor;
    unsigned minutes;
    unsigned seconds;
    size_t length;

    if (framebuffer == NULL || graphics == NULL || level == NULL
        || intermission == NULL || map_number >= 60U
        || !WL_IntermissionChunks(graphics->variant, &chunks))
    {
        return 0;
    }

    WL_StatusDefaults(&status);
    status.score = level->score + intermission->bonus;
    status.health = level->player_health;
    status.ammo = level->player_ammo;
    status.weapon = level->player_weapon;
    status.lives = level->player_lives;
    status.keys = level->player_keys;
    if (!WL_DrawStatusBar(framebuffer, graphics, &status))
    {
        return 0;
    }
    WG_VideoBar(framebuffer, 0, 0, 320, 160,
                WL_INTERMISSION_BACKGROUND);
    if (!WG_VideoDrawPicture(framebuffer, graphics, chunks.guy, 0, 16))
    {
        return 0;
    }

    floor = map_number % 10U;
    if (intermission->special_floor)
    {
        return WL_IntermissionWrite(framebuffer, graphics, &chunks,
                                    14, 4, "secret floor\n completed!")
               && WL_IntermissionWrite(framebuffer, graphics, &chunks,
                                       10, 16, "15000 bonus!");
    }

    if (!WL_IntermissionWrite(framebuffer, graphics, &chunks,
                              14, 2, "floor\ncompleted")
        || snprintf(text, sizeof(text), "%u", floor + 1U) < 0
        || !WL_IntermissionWrite(framebuffer, graphics, &chunks, 26, 2, text)
        || !WL_IntermissionWrite(framebuffer, graphics, &chunks,
                                 14, 7, "bonus")
        || !WL_IntermissionWrite(framebuffer, graphics, &chunks,
                                 16, 10, "time")
        || !WL_IntermissionWrite(framebuffer, graphics, &chunks,
                                 16, 12, " par")
        || !WL_IntermissionWrite(framebuffer, graphics, &chunks,
                                 9, 14, "kill ratio    %")
        || !WL_IntermissionWrite(framebuffer, graphics, &chunks,
                                 5, 16, "secret ratio    %")
        || !WL_IntermissionWrite(framebuffer, graphics, &chunks,
                                 1, 18, "treasure ratio    %"))
    {
        return 0;
    }

    minutes = (unsigned)(intermission->seconds / 60U);
    seconds = (unsigned)(intermission->seconds % 60U);
    if (snprintf(text, sizeof(text), "%02u:%02u", minutes, seconds) < 0
        || !WL_IntermissionWrite(framebuffer, graphics, &chunks,
                                 26, 10, text))
    {
        return 0;
    }
    if (intermission->par_seconds == 0U)
    {
        memcpy(text, "??:??", 6U);
    }
    else
    {
        (void)snprintf(text, sizeof(text), "%02u:%02u",
                       intermission->par_seconds / 60U,
                       intermission->par_seconds % 60U);
    }
    if (!WL_IntermissionWrite(framebuffer, graphics, &chunks,
                              26, 12, text))
    {
        return 0;
    }

    (void)snprintf(text, sizeof(text), "%u", intermission->kill_ratio);
    length = strlen(text);
    if (!WL_IntermissionWrite(framebuffer, graphics, &chunks,
                              37 - (int)length * 2, 14, text))
    {
        return 0;
    }
    (void)snprintf(text, sizeof(text), "%u", intermission->secret_ratio);
    length = strlen(text);
    if (!WL_IntermissionWrite(framebuffer, graphics, &chunks,
                              37 - (int)length * 2, 16, text))
    {
        return 0;
    }
    (void)snprintf(text, sizeof(text), "%u", intermission->treasure_ratio);
    length = strlen(text);
    if (!WL_IntermissionWrite(framebuffer, graphics, &chunks,
                              37 - (int)length * 2, 18, text))
    {
        return 0;
    }
    (void)snprintf(text, sizeof(text), "%lu",
                   (unsigned long)intermission->bonus);
    length = strlen(text);
    return WL_IntermissionWrite(framebuffer, graphics, &chunks,
                                36 - (int)length * 2, 7, text);
}

int WL_VictoryCalculate(const wl_intermission_t ratios[8],
                        wl_victory_t *victory)
{
    unsigned index;
    unsigned kills = 0U;
    unsigned secrets = 0U;
    unsigned treasures = 0U;

    if (ratios == NULL || victory == NULL)
    {
        return 0;
    }
    memset(victory, 0, sizeof(*victory));
    for (index = 0U; index < 8U; ++index)
    {
        victory->seconds += ratios[index].seconds;
        kills += ratios[index].kill_ratio;
        secrets += ratios[index].secret_ratio;
        treasures += ratios[index].treasure_ratio;
    }
    if (victory->seconds > 99U * 60U)
    {
        victory->seconds = 99U * 60U;
    }
    victory->kill_ratio = (uint8_t)(kills / 8U);
    victory->secret_ratio = (uint8_t)(secrets / 8U);
    victory->treasure_ratio = (uint8_t)(treasures / 8U);
    return 1;
}

int WL_DrawVictory(uint8_t *framebuffer,
                   const wg_graphics_t *graphics,
                   const wg_level_t *level,
                   const wl_victory_t *victory)
{
    wl_intermission_chunks_t chunks;
    wl_status_t status;
    wg_font_t font;
    char text[16];
    char code[4];
    unsigned minutes;
    unsigned seconds;
    size_t length;
    int result = 0;

    memset(&font, 0, sizeof(font));
    if (framebuffer == NULL || graphics == NULL || level == NULL
        || victory == NULL
        || !WL_IntermissionChunks(graphics->variant, &chunks))
    {
        return 0;
    }
    WL_StatusDefaults(&status);
    status.score = level->score;
    status.health = level->player_health;
    status.ammo = level->player_ammo;
    status.weapon = level->player_weapon;
    status.lives = level->player_lives;
    status.keys = level->player_keys;
    if (!WL_DrawStatusBar(framebuffer, graphics, &status))
    {
        return 0;
    }
    WG_VideoBar(framebuffer, 0, 0, 320, 160,
                WL_INTERMISSION_BACKGROUND);
    if (!WG_VideoDrawPicture(framebuffer, graphics, chunks.bj_win, 8, 4)
        || !WL_IntermissionWrite(framebuffer, graphics, &chunks,
                                 18, 2, "you win!")
        || !WL_IntermissionWrite(framebuffer, graphics, &chunks,
                                 14, 6, "total time")
        || !WL_IntermissionWrite(framebuffer, graphics, &chunks,
                                 12, 12, "averages")
        || !WL_IntermissionWrite(framebuffer, graphics, &chunks,
                                 14, 14, "kill ratio    %")
        || !WL_IntermissionWrite(framebuffer, graphics, &chunks,
                                 10, 16, "secret ratio    %")
        || !WL_IntermissionWrite(framebuffer, graphics, &chunks,
                                 6, 18, "treasure ratio    %"))
    {
        return 0;
    }
    minutes = (unsigned)(victory->seconds / 60U);
    seconds = (unsigned)(victory->seconds % 60U);
    (void)snprintf(text, sizeof(text), "%02u:%02u", minutes, seconds);
    if (!WL_IntermissionWritePixels(framebuffer, graphics, &chunks,
                                    113, 64, text))
    {
        return 0;
    }
    (void)snprintf(text, sizeof(text), "%u", victory->kill_ratio);
    length = strlen(text);
    if (!WL_IntermissionWrite(framebuffer, graphics, &chunks,
                              30 - (int)length * 2, 14, text))
    {
        return 0;
    }
    (void)snprintf(text, sizeof(text), "%u", victory->secret_ratio);
    length = strlen(text);
    if (!WL_IntermissionWrite(framebuffer, graphics, &chunks,
                              30 - (int)length * 2, 16, text))
    {
        return 0;
    }
    (void)snprintf(text, sizeof(text), "%u", victory->treasure_ratio);
    length = strlen(text);
    if (!WL_IntermissionWrite(framebuffer, graphics, &chunks,
                              30 - (int)length * 2, 18, text))
    {
        return 0;
    }
    if (level->difficulty < WG_DIFFICULTY_MEDIUM)
    {
        return 1;
    }
    code[0] = (char)((((minutes / 10U) ^ (minutes % 10U)) ^ 0x0aU) + 'A');
    code[1] = (char)((((seconds / 10U) ^ (seconds % 10U)) ^ 0x0aU) + 'A');
    code[2] = (char)((code[0] ^ code[1]) + 'A');
    code[3] = '\0';
    if (WG_VideoDrawPicture(framebuffer, graphics, chunks.time_code, 240, 64)
        && WG_FontOpen(&font, graphics, 0U))
    {
        WG_FontDraw(&font, framebuffer, 241, 72, code, 0x47U);
        result = 1;
    }
    WG_FontClose(&font);
    return result;
}

void WL_HighScoresDefault(wl_high_score_t scores[WL_MAX_HIGH_SCORES])
{
    if (scores != NULL)
    {
        memcpy(scores, wl_default_high_scores,
               sizeof(wl_default_high_scores));
    }
}

int WL_HighScoreInsert(wl_high_score_t scores[WL_MAX_HIGH_SCORES],
                       uint32_t score, uint16_t completed,
                       uint16_t episode)
{
    unsigned index;

    if (scores == NULL)
    {
        return -1;
    }
    for (index = 0U; index < WL_MAX_HIGH_SCORES; ++index)
    {
        if (score > scores[index].score
            || (score == scores[index].score
                && completed > scores[index].completed))
        {
            unsigned move;

            for (move = WL_MAX_HIGH_SCORES - 1U; move > index; --move)
            {
                scores[move] = scores[move - 1U];
            }
            memset(&scores[index], 0, sizeof(scores[index]));
            scores[index].score = score;
            scores[index].completed = completed;
            scores[index].episode = episode;
            return (int)index;
        }
    }
    return -1;
}

static int WL_HighScoreDrawText(const wg_font_t *font,
                                uint8_t *framebuffer, int x, int y,
                                const char *text)
{
    if (font == NULL || framebuffer == NULL || text == NULL)
    {
        return x;
    }
    WG_FontDraw(font, framebuffer, x, y, text, 15U);
    return x + (int)WG_FontMeasure(font, text);
}

int WL_DrawHighScores(uint8_t *framebuffer,
                      const wg_graphics_t *graphics,
                      const wl_high_score_t scores[WL_MAX_HIGH_SCORES])
{
    wl_high_score_chunks_t chunks;
    wg_font_t font;
    unsigned index;
    int result = 0;

    memset(&font, 0, sizeof(font));
    if (framebuffer == NULL || graphics == NULL || scores == NULL
        || !WL_HighScoreChunks(graphics->variant, &chunks)
        || !WG_FontOpen(&font, graphics, 0U))
    {
        return 0;
    }
    WG_VideoBar(framebuffer, 0, 0, 320, 200, 0x29U);
    WG_VideoBar(framebuffer, 0, 10, 320, 24, 0U);
    WG_VideoBar(framebuffer, 0, 32, 320, 1, 0x2cU);
    if (!WG_VideoDrawPicture(framebuffer, graphics, chunks.title, 48, 0)
        || !WG_VideoDrawPicture(framebuffer, graphics, chunks.name, 32, 68)
        || !WG_VideoDrawPicture(framebuffer, graphics, chunks.level, 160, 68)
        || !WG_VideoDrawPicture(framebuffer, graphics, chunks.score, 224, 68))
    {
        goto cleanup;
    }
    for (index = 0U; index < WL_MAX_HIGH_SCORES; ++index)
    {
        char number[16];
        char fixed[16];
        char episode[8];
        size_t digit;
        size_t width;
        int x;
        int y = 76 + (int)index * 16;

        (void)WL_HighScoreDrawText(&font, framebuffer, 32, y,
                                   scores[index].name);
        (void)snprintf(number, sizeof(number), "%u",
                       (unsigned)scores[index].completed);
        for (digit = 0U; number[digit] != '\0'; ++digit)
        {
            fixed[digit] = (char)((unsigned char)number[digit]
                                  + (129U - (unsigned)'0'));
        }
        fixed[digit] = '\0';
        width = WG_FontMeasure(&font, fixed);
        x = 176 - (int)width - 6;
        x = WL_HighScoreDrawText(&font, framebuffer, x, y, "E");
        (void)snprintf(episode, sizeof(episode), "%u",
                       (unsigned)scores[index].episode + 1U);
        x = WL_HighScoreDrawText(&font, framebuffer, x, y, episode);
        x = WL_HighScoreDrawText(&font, framebuffer, x, y, "/L");
        (void)WL_HighScoreDrawText(&font, framebuffer, x, y, fixed);

        (void)snprintf(number, sizeof(number), "%lu",
                       (unsigned long)scores[index].score);
        for (digit = 0U; number[digit] != '\0'; ++digit)
        {
            fixed[digit] = (char)((unsigned char)number[digit]
                                  + (129U - (unsigned)'0'));
        }
        fixed[digit] = '\0';
        width = WG_FontMeasure(&font, fixed);
        (void)WL_HighScoreDrawText(&font, framebuffer,
                                   264 - (int)width, y, fixed);
    }
    result = 1;

cleanup:
    WG_FontClose(&font);
    return result;
}

size_t WL_HighScoreNameWidth(const wg_graphics_t *graphics,
                             const char *name)
{
    wg_font_t font;
    size_t width;

    memset(&font, 0, sizeof(font));
    if (graphics == NULL || name == NULL
        || !WG_FontOpen(&font, graphics, 0U))
    {
        return (size_t)-1;
    }
    width = WG_FontMeasure(&font, name);
    WG_FontClose(&font);
    return width;
}

int WL_DrawHighScoreCursor(uint8_t *framebuffer,
                           const wg_graphics_t *graphics,
                           const wl_high_score_t scores[WL_MAX_HIGH_SCORES],
                           unsigned score_index, unsigned cursor)
{
    wg_font_t font;
    char prefix[WL_MAX_HIGH_NAME + 1];
    static const char cursor_text[] = "\x80";
    size_t length;
    int x;

    memset(&font, 0, sizeof(font));
    if (framebuffer == NULL || graphics == NULL || scores == NULL
        || score_index >= WL_MAX_HIGH_SCORES
        || !WG_FontOpen(&font, graphics, 0U))
    {
        return 0;
    }
    length = strlen(scores[score_index].name);
    if (cursor > length)
    {
        cursor = (unsigned)length;
    }
    memcpy(prefix, scores[score_index].name, cursor);
    prefix[cursor] = '\0';
    x = 32 + (int)WG_FontMeasure(&font, prefix) - 1;
    WG_FontDraw(&font, framebuffer, x,
                76 + (int)score_index * 16, cursor_text, 15U);
    WG_FontClose(&font);
    return 1;
}
