/* Portable indexed-video primitives corresponding to the original ID_VL.C. */
#include "ID_VL.h"

#include <stdlib.h>
#include <string.h>

void WG_VideoClear(uint8_t *framebuffer, uint8_t color)
{
    if (framebuffer != NULL)
    {
        memset(framebuffer, color, WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT);
    }
}

void WG_VideoPlot(uint8_t *framebuffer, int x, int y, uint8_t color)
{
    if (framebuffer != NULL && x >= 0 && x < WG_VIDEO_WIDTH
        && y >= 0 && y < WG_VIDEO_HEIGHT)
    {
        framebuffer[(size_t)y * WG_VIDEO_WIDTH + (size_t)x] = color;
    }
}

void WG_VideoBar(uint8_t *framebuffer, int x, int y,
                 int width, int height, uint8_t color)
{
    int row;

    if (framebuffer == NULL || width <= 0 || height <= 0
        || x >= WG_VIDEO_WIDTH || y >= WG_VIDEO_HEIGHT
        || x + width <= 0 || y + height <= 0)
    {
        return;
    }
    if (x < 0)
    {
        width += x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        y = 0;
    }
    if (width > WG_VIDEO_WIDTH - x)
    {
        width = WG_VIDEO_WIDTH - x;
    }
    if (height > WG_VIDEO_HEIGHT - y)
    {
        height = WG_VIDEO_HEIGHT - y;
    }
    for (row = 0; row < height; ++row)
    {
        memset(framebuffer + (size_t)(y + row) * WG_VIDEO_WIDTH + (size_t)x,
               color, (size_t)width);
    }
}

void WG_VideoBlit(uint8_t *framebuffer, int x, int y,
                  const uint8_t *pixels, size_t width, size_t height)
{
    size_t source_x = 0;
    size_t source_y = 0;
    size_t copy_width = width;
    size_t copy_height = height;
    size_t row;

    if (framebuffer == NULL || pixels == NULL || width == 0 || height == 0
        || x >= WG_VIDEO_WIDTH || y >= WG_VIDEO_HEIGHT)
    {
        return;
    }
    if (x < 0)
    {
        source_x = (size_t)(-x);
        if (source_x >= width)
        {
            return;
        }
        copy_width -= source_x;
        x = 0;
    }
    if (y < 0)
    {
        source_y = (size_t)(-y);
        if (source_y >= height)
        {
            return;
        }
        copy_height -= source_y;
        y = 0;
    }
    if (copy_width > (size_t)(WG_VIDEO_WIDTH - x))
    {
        copy_width = (size_t)(WG_VIDEO_WIDTH - x);
    }
    if (copy_height > (size_t)(WG_VIDEO_HEIGHT - y))
    {
        copy_height = (size_t)(WG_VIDEO_HEIGHT - y);
    }
    for (row = 0; row < copy_height; ++row)
    {
        memcpy(framebuffer + (size_t)(y + (int)row) * WG_VIDEO_WIDTH
               + (size_t)x,
               pixels + (source_y + row) * width + source_x, copy_width);
    }
}

int WG_VideoDrawPicture(uint8_t *framebuffer, const wg_graphics_t *graphics,
                        size_t chunk, int x, int y)
{
    uint8_t *pixels;
    uint16_t width;
    uint16_t height;

    if (!WG_GraphicsDecodePicture(graphics, chunk, &pixels, &width, &height))
    {
        return 0;
    }
    WG_VideoBlit(framebuffer, x, y, pixels, width, height);
    free(pixels);
    return 1;
}

void WG_PaletteFromVGA(const uint8_t source[256 * 3],
                       uint8_t destination[256 * 3])
{
    size_t index;

    if (source == NULL || destination == NULL)
    {
        return;
    }
    for (index = 0; index < 256U * 3U; ++index)
    {
        destination[index] = (uint8_t)((unsigned)source[index] * 255U / 63U);
    }
}

void WG_PaletteFade(const uint8_t from[256 * 3],
                    const uint8_t to[256 * 3], unsigned step,
                    unsigned steps, uint8_t destination[256 * 3])
{
    size_t index;

    if (from == NULL || to == NULL || destination == NULL || steps == 0)
    {
        return;
    }
    if (step > steps)
    {
        step = steps;
    }
    for (index = 0; index < 256U * 3U; ++index)
    {
        int start = from[index];
        int delta = (int)to[index] - start;
        destination[index] = (uint8_t)(start + delta * (int)step / (int)steps);
    }
}

void WG_FizzleStart(wg_fizzle_t *fizzle)
{
    if (fizzle != NULL)
    {
        /* Original FizzleFade's 17-bit LFSR starts at one.  This generator is
           intentionally independent of the table-driven gameplay RNG. */
        fizzle->value = 1U;
    }
}

int WG_FizzleStep(wg_fizzle_t *fizzle, const uint8_t *source,
                  uint8_t *destination, size_t width, size_t height,
                  size_t pixels)
{
    return WG_FizzleStepRegion(fizzle, source, width, destination, width,
                               0U, 0U, width, height, pixels);
}

int WG_FizzleStepRegion(wg_fizzle_t *fizzle, const uint8_t *source,
                        size_t source_stride, uint8_t *destination,
                        size_t destination_stride, size_t destination_x,
                        size_t destination_y, size_t width, size_t height,
                        size_t pixels)
{
    size_t pixel;

    if (fizzle == NULL || source == NULL || destination == NULL
        || width == 0 || height == 0 || pixels == 0
        || source_stride < width
        || destination_x > destination_stride
        || width > destination_stride - destination_x)
    {
        return 0;
    }
    for (pixel = 0; pixel < pixels; ++pixel)
    {
        size_t y = (uint8_t)fizzle->value;
        size_t x = (fizzle->value >> 8U) & 0xffffU;
        uint32_t carry = fizzle->value & 1U;

        y = (y - 1U) & 0xffU;
        fizzle->value >>= 1U;
        if (carry != 0)
        {
            fizzle->value ^= 0x00012000U;
        }
        if (x < width && y < height)
        {
            destination[(destination_y + y) * destination_stride
                        + destination_x + x] = source[y * source_stride + x];
        }
        if (fizzle->value == 1U)
        {
            return 1;
        }
    }
    return 0;
}
