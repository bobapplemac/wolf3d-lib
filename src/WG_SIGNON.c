#include "WG_SIGNON.h"

#include <stddef.h>
#include <string.h>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverlength-strings"
#endif
#include "WG_SIGNON_ASSETS.inc"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

typedef struct wg_signon_asset
{
    const char *name;
    const char *pixels;
    wg_game_family_t family;
} wg_signon_asset_t;

static const wg_signon_asset_t WG_SignonAssets[] =
{
    { "apogee", WG_SignonApogee, WG_GAME_FAMILY_WOLF3D },
    { "gt", WG_SignonGT, WG_GAME_FAMILY_WOLF3D },
    { "id", WG_SignonID, WG_GAME_FAMILY_WOLF3D },
    { "activision", WG_SignonActivision, WG_GAME_FAMILY_WOLF3D },
    { "spear", WG_SignonSpear, WG_GAME_FAMILY_SPEAR }
};

static int WG_SignonEqualCaseInsensitive(const char *left, const char *right)
{
    while (*left != '\0' && *right != '\0')
    {
        char left_character = *left++;
        char right_character = *right++;
        if (left_character >= 'A' && left_character <= 'Z')
        {
            left_character = (char)(left_character - 'A' + 'a');
        }
        if (right_character >= 'A' && right_character <= 'Z')
        {
            right_character = (char)(right_character - 'A' + 'a');
        }
        if (left_character != right_character)
        {
            return 0;
        }
    }
    return *left == *right;
}

static const wg_signon_asset_t *WG_SignonFind(const char *name)
{
    size_t index;

    if (name == NULL || WG_SignonEqualCaseInsensitive(name, "auto"))
    {
        return NULL;
    }
    for (index = 0U;
         index < sizeof(WG_SignonAssets) / sizeof(WG_SignonAssets[0]);
         ++index)
    {
        if (WG_SignonEqualCaseInsensitive(name, WG_SignonAssets[index].name))
        {
            return &WG_SignonAssets[index];
        }
    }
    return NULL;
}

static const wg_signon_asset_t *WG_SignonDefault(
    wg_game_variant_t variant, wg_data_edition_t edition)
{
    if (edition == WG_DATA_EDITION_APOGEE)
    {
        return &WG_SignonAssets[0];
    }
    if (edition == WG_DATA_EDITION_GT)
    {
        return &WG_SignonAssets[1];
    }
    if (edition == WG_DATA_EDITION_ID)
    {
        return &WG_SignonAssets[2];
    }
    if (edition == WG_DATA_EDITION_ACTIVISION)
    {
        return &WG_SignonAssets[3];
    }
    if (edition == WG_DATA_EDITION_SPEAR)
    {
        return &WG_SignonAssets[4];
    }
    if (WG_DataUsesApogeeWolfGraphics(variant))
    {
        return &WG_SignonAssets[0];
    }
    if (variant == WG_GAME_WOLF3D_FULL_GT_14)
    {
        return &WG_SignonAssets[1];
    }
    if (WG_DataVariantFamily(variant) == WG_GAME_FAMILY_SPEAR)
    {
        return &WG_SignonAssets[4];
    }
    return NULL;
}

static void WG_SignonBar(uint8_t *framebuffer, unsigned x, unsigned y,
                         unsigned width, unsigned height, uint8_t color)
{
    unsigned row;

    for (row = 0U; row < height; ++row)
    {
        memset(framebuffer + (y + row) * 320U + x, color, width);
    }
}

void WG_SignonDrawIndicators(uint8_t framebuffer[WG_SIGNON_SIZE],
                             wg_game_family_t family,
                             int mouse_present, int joystick_present,
                             int sound_blaster_present)
{
    uint8_t memory_color;
    unsigned index;

    if (framebuffer == NULL)
    {
        return;
    }
    memory_color = family == WG_GAME_FAMILY_SPEAR ? 0x4fU : 0x6cU;
    for (index = 0U; index < 10U; ++index)
    {
        uint8_t color = (uint8_t)(memory_color - index);
        WG_SignonBar(framebuffer, 49U, 163U - 8U * index, 6U, 5U, color);
        WG_SignonBar(framebuffer, 89U, 163U - 8U * index, 6U, 5U, color);
        WG_SignonBar(framebuffer, 129U, 163U - 8U * index, 6U, 5U, color);
    }

    /* As in the original IntroScreen, these markers report hardware that the
       input/audio startup accepted. Sound Blaster takes precedence over its
       AdLib-compatible OPL hardware. Disney Sound Source is absent. */
    if (mouse_present)
    {
        WG_SignonBar(framebuffer, 164U, 82U, 12U, 2U, 14U);
    }
    if (joystick_present)
    {
        WG_SignonBar(framebuffer, 164U, 105U, 12U, 2U, 14U);
    }
    WG_SignonBar(framebuffer, 164U,
                 sound_blaster_present ? 151U : 128U, 12U, 2U, 14U);
}

int WG_SignonIsEmbeddedName(const char *name)
{
    return name == NULL || WG_SignonEqualCaseInsensitive(name, "auto")
        || WG_SignonFind(name) != NULL;
}

int WG_SignonDraw(uint8_t framebuffer[WG_SIGNON_SIZE],
                  wg_game_variant_t game_variant,
                  wg_data_edition_t data_edition, const char *name,
                  int mouse_present, int joystick_present,
                  int sound_blaster_present,
                  wg_game_family_t *palette_family)
{
    const wg_signon_asset_t *asset;

    if (framebuffer == NULL || palette_family == NULL)
    {
        return 0;
    }
    asset = name == NULL || WG_SignonEqualCaseInsensitive(name, "auto")
                ? WG_SignonDefault(game_variant, data_edition)
                : WG_SignonFind(name);
    if (asset == NULL)
    {
        return 0;
    }
    memcpy(framebuffer, asset->pixels, WG_SIGNON_SIZE);
    WG_SignonDrawIndicators(framebuffer, asset->family, mouse_present,
                            joystick_present, sound_blaster_present);
    *palette_family = asset->family;
    return 1;
}
