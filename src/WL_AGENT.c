/* Portable status-bar drawing from the original WL_AGENT.C/WL_GAME.C. */
#include "WL_AGENT.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
    WL_STATUS_HEIGHT = 40,
    WL_STATUS_Y = WG_VIDEO_HEIGHT - WL_STATUS_HEIGHT
};

typedef struct wl_status_chunks
{
    size_t status_bar;
    size_t knife;
    size_t no_key;
    size_t blank_digit;
    size_t zero_digit;
    size_t face_1a;
} wl_status_chunks_t;

static int WL_StatusChunks(wg_game_variant_t variant,
                           wl_status_chunks_t *chunks)
{
    size_t status_bar;

    if (chunks == NULL)
    {
        return 0;
    }
    if (variant == WG_GAME_WOLF3D_FULL_GT_14)
    {
        status_bar = 86U;
    }
    else if (variant == WG_GAME_WOLF3D_SHAREWARE_14)
    {
        /* The supplied Apogee v1.4 WL1 graph is four chunks later than the
           early generated GFXE_WL1.H retained in the source release. */
        status_bar = 98U;
    }
    else
    {
        return 0;
    }
    chunks->status_bar = status_bar;
    chunks->knife = status_bar + 5U;
    chunks->no_key = status_bar + 9U;
    chunks->blank_digit = status_bar + 12U;
    chunks->zero_digit = status_bar + 13U;
    chunks->face_1a = status_bar + 23U;
    return 1;
}

static int WL_DrawPicture(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    const wg_graphics_t *graphics, size_t chunk, int x, int y)
{
    uint8_t *pixels = NULL;
    uint16_t width;
    uint16_t height;
    int result;

    if (!WG_GraphicsDecodePicture(graphics, chunk, &pixels, &width, &height))
    {
        return 0;
    }
    result = x >= 0 && y >= 0
             && x + width <= WG_VIDEO_WIDTH
             && y + height <= WG_VIDEO_HEIGHT;
    if (result)
    {
        WG_VideoBlit(framebuffer, x, y, pixels, width, height);
    }
    free(pixels);
    return result;
}

static int WL_StatusDrawPicture(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    const wg_graphics_t *graphics, size_t chunk, int x, int y)
{
    return WL_DrawPicture(framebuffer, graphics, chunk, x * 8, WL_STATUS_Y + y);
}

static int WL_DrawNumber(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    const wg_graphics_t *graphics, const wl_status_chunks_t *chunks,
    int x, int y, unsigned width, uint32_t number)
{
    char digits[16];
    int length;
    unsigned first;
    unsigned index;

    length = snprintf(digits, sizeof(digits), "%lu", (unsigned long)number);
    if (length < 0 || (size_t)length >= sizeof(digits))
    {
        return 0;
    }
    while ((unsigned)length < width)
    {
        if (!WL_StatusDrawPicture(framebuffer, graphics,
                                  chunks->blank_digit, x++, y))
        {
            return 0;
        }
        --width;
    }
    first = (unsigned)length <= width ? 0U : (unsigned)length - width;
    for (index = first; index < (unsigned)length; ++index)
    {
        if (!WL_StatusDrawPicture(framebuffer, graphics,
                                  chunks->zero_digit
                                  + (unsigned)(digits[index] - '0'),
                                  x++, y))
        {
            return 0;
        }
    }
    return 1;
}

void WL_StatusDefaults(wl_status_t *status)
{
    if (status == NULL)
    {
        return;
    }
    memset(status, 0, sizeof(*status));
    status->health = 100;
    status->ammo = 8;
    status->lives = 3;
    status->weapon = 1;
}

int WL_DrawStatusBar(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    const wg_graphics_t *graphics, const wl_status_t *status)
{
    wl_status_chunks_t chunks;
    size_t face;

    if (framebuffer == NULL || graphics == NULL || status == NULL
        || status->health > 100U || status->weapon > 3U
        || status->face_frame > 2U
        || !WL_StatusChunks(graphics->variant, &chunks)
        || !WL_DrawPicture(framebuffer, graphics, chunks.status_bar,
                           0, WL_STATUS_Y))
    {
        return 0;
    }

    face = chunks.face_1a
           + 3U * ((100U - status->health) / 16U)
           + status->face_frame;
    return WL_StatusDrawPicture(framebuffer, graphics, face, 17, 4)
           && WL_DrawNumber(framebuffer, graphics, &chunks,
                            21, 16, 3, status->health)
           && WL_DrawNumber(framebuffer, graphics, &chunks,
                            14, 16, 1, status->lives)
           && WL_DrawNumber(framebuffer, graphics, &chunks,
                            2, 16, 2, (uint32_t)status->map + 1U)
           && WL_DrawNumber(framebuffer, graphics, &chunks,
                            27, 16, 2, status->ammo)
           && WL_StatusDrawPicture(framebuffer, graphics,
                                   (status->keys & 1U) != 0U
                                   ? chunks.no_key + 1U : chunks.no_key,
                                   30, 4)
           && WL_StatusDrawPicture(framebuffer, graphics,
                                   (status->keys & 2U) != 0U
                                   ? chunks.no_key + 2U : chunks.no_key,
                                   30, 20)
           && WL_StatusDrawPicture(framebuffer, graphics,
                                   chunks.knife + status->weapon, 32, 8)
           && WL_DrawNumber(framebuffer, graphics, &chunks,
                            6, 16, 6, status->score);
}
