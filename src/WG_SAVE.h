#ifndef WG_SAVE_H
#define WG_SAVE_H

#include <stddef.h>
#include <stdint.h>

#include "WG_DATA.h"
#include "WL_GAME.h"
#include "WL_INTER.h"

#define WG_SAVE_NAME_BYTES 32U
#define WG_SAVE_BUFFER_SIZE 65536U

typedef struct wg_save_state
{
    wg_level_t level;
    unsigned map_number;
    uint32_t level_start_score;
    wl_intermission_t level_ratios[WL_MAX_LEVEL_RATIOS];
} wg_save_state_t;

int WG_SaveEncode(uint8_t *data, size_t capacity, size_t *size,
                  const char name[WG_SAVE_NAME_BYTES],
                  wg_game_variant_t variant,
                  const wg_save_state_t *state);
int WG_SaveDecode(const uint8_t *data, size_t size,
                  wg_game_variant_t variant,
                  char name[WG_SAVE_NAME_BYTES],
                  wg_save_state_t *state);
int WG_SaveReadName(const uint8_t *data, size_t size,
                    wg_game_variant_t variant,
                    char name[WG_SAVE_NAME_BYTES]);
int WG_SaveWriteFile(const char *path,
                     const char name[WG_SAVE_NAME_BYTES],
                     wg_game_variant_t variant,
                     const wg_save_state_t *state);
int WG_SaveReadFile(const char *path, wg_game_variant_t variant,
                    char name[WG_SAVE_NAME_BYTES],
                    wg_save_state_t *state);

#endif
