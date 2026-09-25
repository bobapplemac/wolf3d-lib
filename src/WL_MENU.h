#ifndef WL_MENU_H
#define WL_MENU_H

#include <stdint.h>

#include "WG_GRAPHICS.h"

#define WL_MAIN_MENU_ITEMS 10U
#define WL_MAIN_MENU_DEFAULT_ITEM 6U

int WL_DrawMainMenu(uint8_t framebuffer[320 * 200],
                    const wg_graphics_t *graphics, unsigned selected,
                    int in_game);
unsigned WL_MainMenuMove(unsigned selected, int direction, int in_game);

#endif
