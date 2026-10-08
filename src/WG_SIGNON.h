#ifndef WG_SIGNON_H
#define WG_SIGNON_H

#include <stdint.h>

#include "WG_DATA.h"

#define WG_SIGNON_SIZE (320U * 200U)

typedef enum wg_sound_hardware
{
    WG_SOUND_HARDWARE_NONE,
    WG_SOUND_HARDWARE_ADLIB,
    WG_SOUND_HARDWARE_SOUND_BLASTER
} wg_sound_hardware_t;

int WG_SignonIsEmbeddedName(const char *name);
int WG_SignonDraw(uint8_t framebuffer[WG_SIGNON_SIZE],
                  wg_game_variant_t game_variant,
                  wg_data_edition_t data_edition, const char *name,
                  int mouse_present, int joystick_present,
                  wg_sound_hardware_t sound_hardware,
                  wg_game_family_t *palette_family);
void WG_SignonDrawIndicators(uint8_t framebuffer[WG_SIGNON_SIZE],
                             wg_game_family_t family,
                             int mouse_present, int joystick_present,
                             wg_sound_hardware_t sound_hardware);

#endif
