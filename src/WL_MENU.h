#ifndef WL_MENU_H
#define WL_MENU_H

#include <stdint.h>

#include "WG_GRAPHICS.h"

#define WL_MAIN_MENU_ITEMS 10U
#define WL_MAIN_MENU_DEFAULT_ITEM 6U
#define WL_SOUND_MENU_ITEMS 12U
#define WL_CONTROL_MENU_ITEMS 6U

int WL_DrawMainMenu(uint8_t framebuffer[320 * 200],
                    const wg_graphics_t *graphics, unsigned selected,
                    int in_game);
unsigned WL_MainMenuMove(unsigned selected, int direction, int in_game);
int WL_DrawSoundMenu(uint8_t framebuffer[320 * 200],
                     const wg_graphics_t *graphics, unsigned selected,
                     int adlib_effects, int digitized, int music);
unsigned WL_SoundMenuMove(unsigned selected, int direction);
int WL_DrawControlMenu(uint8_t framebuffer[320 * 200],
                       const wg_graphics_t *graphics, unsigned selected,
                       int mouse_enabled);
unsigned WL_ControlMenuMove(unsigned selected, int direction,
                            int mouse_enabled);
int WL_DrawMouseSensitivity(uint8_t framebuffer[320 * 200],
                            const wg_graphics_t *graphics,
                            unsigned adjustment);
int WL_DrawEpisodeMenu(uint8_t framebuffer[320 * 200],
                       const wg_graphics_t *graphics, unsigned episode,
                       int shareware);
unsigned WL_EpisodeMenuMove(unsigned episode, int direction);
int WL_DrawDifficultyMenu(uint8_t framebuffer[320 * 200],
                          const wg_graphics_t *graphics, unsigned difficulty);
unsigned WL_DifficultyMenuMove(unsigned difficulty, int direction);

#endif
