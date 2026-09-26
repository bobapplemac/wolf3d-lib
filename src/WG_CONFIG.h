#ifndef WG_CONFIG_H
#define WG_CONFIG_H

#include <stddef.h>
#include <stdint.h>

#include "WG_DATA.h"
#include "WL_INTER.h"
#include "WL_MENU.h"

#define WG_CONFIG_BUFFER_SIZE 1024U

typedef struct wg_config
{
    wl_high_score_t high_scores[WL_MAX_HIGH_SCORES];
    uint16_t action_keys[WL_CUSTOM_BINDINGS];
    uint16_t movement_keys[WL_CUSTOM_BINDINGS];
    uint8_t mouse_bindings[WL_CUSTOM_BINDINGS];
    uint8_t adlib_effects;
    uint8_t digitized_effects;
    uint8_t music_enabled;
    uint8_t mouse_enabled;
    uint8_t mouse_adjustment;
    uint8_t view_size;
} wg_config_t;

void WG_ConfigDefaults(wg_config_t *config);
int WG_ConfigEncode(uint8_t *data, size_t capacity, size_t *size,
                    wg_game_variant_t variant,
                    const wg_config_t *config);
int WG_ConfigDecode(const uint8_t *data, size_t size,
                    wg_game_variant_t variant,
                    wg_config_t *config);
int WG_ConfigReadFile(const char *path, wg_game_variant_t variant,
                      wg_config_t *config);
int WG_ConfigWriteFile(const char *path, wg_game_variant_t variant,
                       const wg_config_t *config);

#endif
