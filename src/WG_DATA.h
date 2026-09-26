#ifndef WG_DATA_H
#define WG_DATA_H

#include <stddef.h>
#include <stdint.h>

typedef enum wg_game_variant
{
    WG_GAME_UNKNOWN = 0,
    WG_GAME_WOLF3D_SHAREWARE_14,
    WG_GAME_WOLF3D_FULL_GT_14,
    WG_GAME_SPEAR_FULL_SOD,
    WG_GAME_SPEAR_DEMO_SDM,
    WG_GAME_SPEAR_MISSION_1_SD1,
    WG_GAME_SPEAR_MISSION_2_SD2,
    WG_GAME_SPEAR_MISSION_3_SD3
} wg_game_variant_t;

typedef enum wg_game_family
{
    WG_GAME_FAMILY_UNKNOWN = 0,
    WG_GAME_FAMILY_WOLF3D,
    WG_GAME_FAMILY_SPEAR
} wg_game_family_t;

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
    char graphics_extension[5];
    char audio_extension[5];
    uint16_t page_count;
    uint16_t sprite_start;
    uint16_t sound_start;
    uint16_t rlew_tag;
    size_t graphics_offset_count;
    size_t audio_offset_count;
    wg_page_entry_t *pages;
} wg_data_set_t;

int WG_DataOpen(wg_data_set_t *data_set, const char *root);
int WG_DataOpenSelected(wg_data_set_t *data_set, const char *root,
                        wg_game_variant_t requested_variant,
                        wg_game_family_t preferred_family);
void WG_DataClose(wg_data_set_t *data_set);
wg_game_variant_t WG_DataVariantFromExtension(const char *extension);
int WG_DataParseGame(const char *text, wg_game_variant_t *variant);
wg_game_family_t WG_DataVariantFamily(wg_game_variant_t variant);
wg_game_family_t WG_DataExecutableFamily(const char *path);
const char *WG_DataVariantExtension(wg_game_variant_t variant);
const char *WG_DataVariantName(wg_game_variant_t variant);
size_t WG_DataSoundCount(wg_game_variant_t variant);
size_t WG_DataMusicBase(wg_game_variant_t variant);
size_t WG_DataDemoChunk(wg_game_variant_t variant, unsigned demo_number);
unsigned WG_DataDemoCount(wg_game_variant_t variant);
size_t WG_DataCreditsChunk(wg_game_variant_t variant);

#endif
