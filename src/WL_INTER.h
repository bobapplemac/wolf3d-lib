#ifndef WL_INTER_H
#define WL_INTER_H

#include <stdint.h>

#include "WG_GRAPHICS.h"
#include "WL_GAME.h"

#define WL_MAX_LEVEL_RATIOS 20U

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

#define WL_MAX_HIGH_SCORES 7
#define WL_MAX_HIGH_NAME 57

typedef struct wl_high_score
{
    char name[WL_MAX_HIGH_NAME + 1];
    uint32_t score;
    uint16_t completed;
    uint16_t episode;
} wl_high_score_t;

int WL_IntermissionCalculate(const wg_level_t *level,
                             unsigned map_number,
                             wl_intermission_t *intermission);
int WL_IntermissionIsSpecial(wg_game_variant_t variant,
                             unsigned map_number);
int WL_DrawLevelCompleted(uint8_t *framebuffer,
                          const wg_graphics_t *graphics,
                          const wg_level_t *level,
                          unsigned map_number,
                          const wl_intermission_t *intermission);
int WL_VictoryCalculate(const wl_intermission_t ratios[8],
                        wl_victory_t *victory);
int WL_VictoryCalculateForVariant(
    wg_game_variant_t variant,
    const wl_intermission_t ratios[WL_MAX_LEVEL_RATIOS],
    wl_victory_t *victory);
int WL_DrawVictory(uint8_t *framebuffer,
                   const wg_graphics_t *graphics,
                   const wg_level_t *level,
                   const wl_victory_t *victory);
int WL_DrawSpearCollapse(uint8_t *framebuffer,
                         const wg_graphics_t *graphics,
                         unsigned frame);
#define WL_SPEAR_END_PAGE_COUNT 10U
int WL_DrawSpearEndPage(uint8_t *framebuffer,
                        uint8_t palette[256 * 3],
                        const wg_graphics_t *graphics,
                        unsigned page);
void WL_HighScoresDefault(wl_high_score_t scores[WL_MAX_HIGH_SCORES]);
int WL_HighScoreInsert(wl_high_score_t scores[WL_MAX_HIGH_SCORES],
                       uint32_t score, uint16_t completed,
                       uint16_t episode);
int WL_DrawHighScores(uint8_t *framebuffer,
                      const wg_graphics_t *graphics,
                      const wl_high_score_t scores[WL_MAX_HIGH_SCORES]);
size_t WL_HighScoreNameWidth(const wg_graphics_t *graphics,
                             const char *name);
int WL_DrawHighScoreCursor(uint8_t *framebuffer,
                           const wg_graphics_t *graphics,
                           const wl_high_score_t scores[WL_MAX_HIGH_SCORES],
                           unsigned score_index, unsigned cursor);

#endif
