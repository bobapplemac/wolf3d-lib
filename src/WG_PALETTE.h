#ifndef WG_PALETTE_H
#define WG_PALETTE_H

#include <stdint.h>

#include "WG_DATA.h"

extern const uint8_t WG_WolfPalette[256 * 3];
extern const uint8_t WG_WolfPaletteVGA[256 * 3];

void WG_GamePaletteVGA(wg_game_variant_t variant,
                       uint8_t palette[256 * 3]);
void WG_GamePalette(wg_game_variant_t variant,
                    uint8_t palette[256 * 3]);

#endif
