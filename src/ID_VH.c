/* Portable proportional-font drawing corresponding to the original ID_VH.C. */
#include "ID_VH.h"

#include <stdlib.h>
#include <string.h>

#include "WG_ENDIAN.h"
#include "ID_VL.h"

#define WG_FONT_HEADER_SIZE 770U

int WG_FontOpen(wg_font_t *font, const wg_graphics_t *graphics,
                unsigned font_number)
{
    size_t index;

    if (font == NULL || graphics == NULL || font_number >= 2U)
    {
        return 0;
    }
    memset(font, 0, sizeof(*font));
    if (!WG_GraphicsDecodeChunk(graphics, 1U + font_number,
                                &font->data, &font->data_size)
        || font->data_size < WG_FONT_HEADER_SIZE)
    {
        WG_FontClose(font);
        return 0;
    }
    font->height = WG_ReadLE16(font->data);
    if (font->height == 0)
    {
        WG_FontClose(font);
        return 0;
    }
    for (index = 0; index < 256U; ++index)
    {
        size_t glyph_size;

        font->locations[index] = WG_ReadLE16(font->data + 2U + index * 2U);
        font->widths[index] = font->data[514U + index];
        glyph_size = (size_t)font->widths[index] * font->height;
        if (glyph_size != 0
            && ((size_t)font->locations[index] > font->data_size
                || glyph_size > font->data_size - font->locations[index]))
        {
            WG_FontClose(font);
            return 0;
        }
    }
    return 1;
}

void WG_FontClose(wg_font_t *font)
{
    if (font == NULL)
    {
        return;
    }
    free(font->data);
    memset(font, 0, sizeof(*font));
}

size_t WG_FontMeasure(const wg_font_t *font, const char *text)
{
    size_t width = 0;

    if (font == NULL || text == NULL)
    {
        return 0;
    }
    while (*text != '\0')
    {
        width += font->widths[(uint8_t)*text++];
    }
    return width;
}

void WG_FontDraw(const wg_font_t *font, uint8_t *framebuffer,
                 int x, int y, const char *text, uint8_t color)
{
    if (font == NULL || framebuffer == NULL || text == NULL)
    {
        return;
    }
    while (*text != '\0')
    {
        uint8_t character = (uint8_t)*text++;
        size_t width = font->widths[character];
        const uint8_t *glyph = font->data + font->locations[character];
        size_t row;
        size_t column;

        for (row = 0; row < font->height; ++row)
        {
            for (column = 0; column < width; ++column)
            {
                if (glyph[row * width + column] != 0)
                {
                    WG_VideoPlot(framebuffer, x + (int)column,
                                 y + (int)row, color);
                }
            }
        }
        x += (int)width;
    }
}
