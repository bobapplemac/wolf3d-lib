#ifndef WG_FONT_H
#define WG_FONT_H

#include <stddef.h>
#include <stdint.h>

#include "wg_graphics.h"

typedef struct wg_font
{
    uint16_t height;
    uint16_t locations[256];
    uint8_t widths[256];
    uint8_t *data;
    size_t data_size;
} wg_font_t;

int WG_FontOpen(wg_font_t *font, const wg_graphics_t *graphics,
                unsigned font_number);
void WG_FontClose(wg_font_t *font);
size_t WG_FontMeasure(const wg_font_t *font, const char *text);
void WG_FontDraw(const wg_font_t *font, uint8_t *framebuffer,
                 int x, int y, const char *text, uint8_t color);

#endif
