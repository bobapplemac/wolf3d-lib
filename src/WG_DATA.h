#ifndef WG_DATA_H
#define WG_DATA_H

#include <stddef.h>
#include <stdint.h>

typedef enum wg_game_variant
{
    WG_GAME_UNKNOWN = 0,
    WG_GAME_WOLF3D_SHAREWARE_14,
    WG_GAME_WOLF3D_FULL_GT_14
} wg_game_variant_t;

typedef struct wg_page_entry
{
    uint32_t offset;
    uint16_t length;
} wg_page_entry_t;

typedef struct wg_data_set
{
    wg_game_variant_t variant;
    char root[1024];
    char extension[5];
    uint16_t page_count;
    uint16_t sprite_start;
    uint16_t sound_start;
    uint16_t rlew_tag;
    size_t graphics_offset_count;
    size_t audio_offset_count;
    wg_page_entry_t *pages;
} wg_data_set_t;

int WG_DataOpen(wg_data_set_t *data_set, const char *root);
void WG_DataClose(wg_data_set_t *data_set);
const char *WG_DataVariantName(wg_game_variant_t variant);

#endif

