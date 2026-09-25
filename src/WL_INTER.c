/* Portable LevelCompleted presentation derived from the original WL_INTER.C. */
#include "WL_INTER.h"

#include <stdio.h>
#include <string.h>

#include "ID_VL.h"
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
} wl_intermission_chunks_t;

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
    return 1;
}

static int WL_IntermissionWrite(uint8_t *framebuffer,
                                const wg_graphics_t *graphics,
                                const wl_intermission_chunks_t *chunks,
                                int x, int y, const char *text)
{
    int origin_x = x * 8;
    int draw_x = origin_x;
    int draw_y = y * 8;

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
