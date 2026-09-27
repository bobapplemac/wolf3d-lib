#ifndef WG_SIGNON_H
#define WG_SIGNON_H

#include <stdint.h>

#include "WG_DATA.h"

#define WG_SIGNON_SIZE (320U * 200U)

int WG_SignonIsEmbeddedName(const char *name);
int WG_SignonDraw(uint8_t framebuffer[WG_SIGNON_SIZE],
                  wg_game_variant_t game_variant, const char *name,
                  int joystick_present, int sound_blaster_present,
                  wg_game_family_t *palette_family);
void WG_SignonDrawIndicators(uint8_t framebuffer[WG_SIGNON_SIZE],
                             wg_game_family_t family,
                             int joystick_present,
                             int sound_blaster_present);

#endif
