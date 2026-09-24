#include "wg_palette.h"

#define RGB(red, green, blue) \
    (uint8_t)((red) * 255 / 63), \
    (uint8_t)((green) * 255 / 63), \
    (uint8_t)((blue) * 255 / 63)

const uint8_t WG_WolfPalette[256 * 3] =
{
#include "wolfpal.inc"
};

#undef RGB

