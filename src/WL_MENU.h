#ifndef WL_MENU_H
#define WL_MENU_H

#include <stdint.h>

#include "WG_GRAPHICS.h"

#define WL_MAIN_MENU_ITEMS 10U
#define WL_MAIN_MENU_DEFAULT_ITEM 6U
#define WL_SOUND_MENU_ITEMS 12U
#define WL_CONTROL_MENU_ITEMS 6U
#define WL_CUSTOM_MENU_ITEMS 9U
#define WL_CUSTOM_BINDINGS 4U
#define WL_VIEW_SIZE_MIN 4U
#define WL_VIEW_SIZE_MAX 19U
#define WL_VIEW_SIZE_DEFAULT 15U
#define WL_SAVE_SLOTS 10U
#define WL_SAVE_NAME_LENGTH 31U

int WL_DrawMainMenu(uint8_t framebuffer[320 * 200],
                    const wg_graphics_t *graphics, unsigned selected,
                    int in_game);
unsigned WL_MainMenuMove(unsigned selected, int direction, int in_game);
unsigned WL_MainMenuMoveForVariant(unsigned selected, int direction,
                                   int in_game, wg_game_variant_t variant);
int WL_DrawConfirm(uint8_t framebuffer[320 * 200],
                   const wg_graphics_t *graphics, const char *message);
int WL_DrawLoadSaveMenu(
    uint8_t framebuffer[320 * 200], const wg_graphics_t *graphics,
    int saving, unsigned selected,
    const uint8_t available[WL_SAVE_SLOTS],
    const char names[WL_SAVE_SLOTS][WL_SAVE_NAME_LENGTH + 1U],
    int editing, int confirm_overwrite);
unsigned WL_LoadSaveMenuMove(unsigned selected, int direction);
int WL_DrawSoundMenu(uint8_t framebuffer[320 * 200],
                     const wg_graphics_t *graphics, unsigned selected,
                     unsigned sound_mode, int digitized, int music);
unsigned WL_SoundMenuMove(unsigned selected, int direction);
int WL_DrawControlMenu(uint8_t framebuffer[320 * 200],
                       const wg_graphics_t *graphics, unsigned selected,
                       int mouse_enabled, int joystick_present,
                       int joystick_enabled, unsigned joystick_port,
                       int gamepad_enabled);
unsigned WL_ControlMenuMove(unsigned selected, int direction,
                            int mouse_enabled, int joystick_present,
                            int joystick_enabled);
int WL_DrawMouseSensitivity(uint8_t framebuffer[320 * 200],
                            const wg_graphics_t *graphics,
                            unsigned adjustment);
int WL_DrawCustomizeMenu(
    uint8_t framebuffer[320 * 200], const wg_graphics_t *graphics,
    unsigned selected, int mouse_enabled, int joystick_enabled,
    const uint8_t mouse_bindings[WL_CUSTOM_BINDINGS],
    const uint8_t joystick_bindings[WL_CUSTOM_BINDINGS],
    const uint16_t action_keys[WL_CUSTOM_BINDINGS],
    const uint16_t movement_keys[WL_CUSTOM_BINDINGS],
    int edit_column, int capture);
unsigned WL_CustomMenuMove(unsigned selected, int direction,
                           int mouse_enabled, int joystick_enabled);
int WL_DrawChangeView(uint8_t framebuffer[320 * 200],
                      const wg_graphics_t *graphics, unsigned view_size);
int WL_DrawEpisodeMenu(uint8_t framebuffer[320 * 200],
                       const wg_graphics_t *graphics, unsigned episode,
                       int shareware);
unsigned WL_EpisodeMenuMove(unsigned episode, int direction);
int WL_DrawDifficultyMenu(uint8_t framebuffer[320 * 200],
                          const wg_graphics_t *graphics, unsigned difficulty);
unsigned WL_DifficultyMenuMove(unsigned difficulty, int direction);

#endif
