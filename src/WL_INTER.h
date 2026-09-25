#ifndef WL_INTER_H
#define WL_INTER_H

#include <stdint.h>

#include "WG_GRAPHICS.h"
#include "WL_GAME.h"

typedef struct wl_intermission
{
    uint32_t seconds;
    uint32_t bonus;
    uint16_t par_seconds;
    uint8_t kill_ratio;
    uint8_t secret_ratio;
    uint8_t treasure_ratio;
    uint8_t special_floor;
} wl_intermission_t;

typedef struct wl_victory
{
    uint32_t seconds;
    uint8_t kill_ratio;
    uint8_t secret_ratio;
    uint8_t treasure_ratio;
} wl_victory_t;

int WL_IntermissionCalculate(const wg_level_t *level,
                             unsigned map_number,
                             wl_intermission_t *intermission);
int WL_DrawLevelCompleted(uint8_t *framebuffer,
                          const wg_graphics_t *graphics,
                          const wg_level_t *level,
                          unsigned map_number,
                          const wl_intermission_t *intermission);
int WL_VictoryCalculate(const wl_intermission_t ratios[8],
                        wl_victory_t *victory);
int WL_DrawVictory(uint8_t *framebuffer,
                   const wg_graphics_t *graphics,
                   const wg_level_t *level,
                   const wl_victory_t *victory);

#endif
