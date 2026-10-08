#ifndef ID_VL_H
#define ID_VL_H

#include <stddef.h>
#include <stdint.h>

#include "WG_GRAPHICS.h"

#define WG_VIDEO_WIDTH 320
#define WG_VIDEO_HEIGHT 200

typedef struct wg_fizzle
{
    uint32_t value;
} wg_fizzle_t;

void WG_VideoClear(uint8_t *framebuffer, uint8_t color);
void WG_VideoPlot(uint8_t *framebuffer, int x, int y, uint8_t color);
void WG_VideoBar(uint8_t *framebuffer, int x, int y,
                 int width, int height, uint8_t color);
void WG_VideoBlit(uint8_t *framebuffer, int x, int y,
                  const uint8_t *pixels, size_t width, size_t height);
int WG_VideoDrawPicture(uint8_t *framebuffer, const wg_graphics_t *graphics,
                        size_t chunk, int x, int y);

void WG_PaletteFromVGA(const uint8_t source[256 * 3],
                       uint8_t destination[256 * 3]);
void WG_PaletteFade(const uint8_t from[256 * 3],
                    const uint8_t to[256 * 3], unsigned step,
                    unsigned steps, uint8_t destination[256 * 3]);

void WG_FizzleStart(wg_fizzle_t *fizzle);
int WG_FizzleStep(wg_fizzle_t *fizzle, const uint8_t *source,
                  uint8_t *destination, size_t width, size_t height,
                  size_t pixels);
int WG_FizzleStepRegion(wg_fizzle_t *fizzle, const uint8_t *source,
                        size_t source_stride, uint8_t *destination,
                        size_t destination_stride, size_t destination_x,
                        size_t destination_y, size_t width, size_t height,
                        size_t pixels);

#endif
