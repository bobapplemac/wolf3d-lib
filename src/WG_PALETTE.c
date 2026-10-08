#include "WG_PALETTE.h"

#include <string.h>

const uint8_t WG_WolfPaletteVGA[256 * 3] =
{
#define RGB(red, green, blue) red, green, blue
#include "WOLFPAL.inc"
#undef RGB
};

#define RGB(red, green, blue) \
    (uint8_t)((red) * 255 / 63), \
    (uint8_t)((green) * 255 / 63), \
    (uint8_t)((blue) * 255 / 63)

const uint8_t WG_WolfPalette[256 * 3] =
{
#include "WOLFPAL.inc"
};

#undef RGB

void WG_GamePaletteVGA(wg_game_variant_t variant,
                       uint8_t palette[256 * 3])
{
    memcpy(palette, WG_WolfPaletteVGA, 256U * 3U);
    if (WG_DataVariantFamily(variant) == WG_GAME_FAMILY_SPEAR)
    {
        /* GAMEPAL_SPEAR.OBJ differs from GAMEPAL_WOLF.OBJ only in the
         * two greens reserved at palette indices 166 and 167. */
        palette[166U * 3U] = 0U;
        palette[166U * 3U + 1U] = 14U;
        palette[166U * 3U + 2U] = 0U;
        palette[167U * 3U] = 0U;
        palette[167U * 3U + 1U] = 10U;
        palette[167U * 3U + 2U] = 0U;
    }
}

void WG_GamePalette(wg_game_variant_t variant,
                    uint8_t palette[256 * 3])
{
    uint8_t vga_palette[256U * 3U];
    size_t index;

    WG_GamePaletteVGA(variant, vga_palette);
    for (index = 0U; index < sizeof(vga_palette); ++index)
    {
        palette[index] = (uint8_t)((unsigned)vga_palette[index] * 255U / 63U);
    }
}
