#include "WG_DATA.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "WG_ENDIAN.h"
#include "WG_FILE.h"

typedef struct wg_data_profile
{
    wg_game_variant_t variant;
    wg_game_family_t family;
    const char *extension;
    const char *graphics_extension;
    const char *audio_extension;
    const char *name;
} wg_data_profile_t;

static const wg_data_profile_t WG_DataProfiles[] =
{
    { WG_GAME_WOLF3D_FULL_GT_14, WG_GAME_FAMILY_WOLF3D,
      ".WL6", ".WL6", ".WL6",
      "Wolfenstein 3D v1.4 GT/ID/Activision" },
    { WG_GAME_WOLF3D_SHAREWARE_14, WG_GAME_FAMILY_WOLF3D,
      ".WL1", ".WL1", ".WL1",
      "Wolfenstein 3D v1.4 shareware" },
    { WG_GAME_SPEAR_FULL_SOD, WG_GAME_FAMILY_SPEAR,
      ".SOD", ".SOD", ".SOD",
      "Spear of Destiny" },
    { WG_GAME_SPEAR_DEMO_SDM, WG_GAME_FAMILY_SPEAR,
      ".SDM", ".SDM", ".SDM",
      "Spear of Destiny demo" },
    { WG_GAME_SPEAR_MISSION_1_SD1, WG_GAME_FAMILY_SPEAR,
      ".SD1", ".SOD", ".SOD",
      "Spear of Destiny mission 1" },
    { WG_GAME_SPEAR_MISSION_2_SD2, WG_GAME_FAMILY_SPEAR,
      ".SD2", ".SOD", ".SOD",
      "Spear of Destiny mission 2: Return to Danger" },
    { WG_GAME_SPEAR_MISSION_3_SD3, WG_GAME_FAMILY_SPEAR,
      ".SD3", ".SOD", ".SOD",
      "Spear of Destiny mission 3: Ultimate Challenge" }
};

static int WG_DataASCIIEqual(char left, char right)
{
    if (left >= 'a' && left <= 'z')
    {
        left = (char)(left - ('a' - 'A'));
    }
    if (right >= 'a' && right <= 'z')
    {
        right = (char)(right - ('a' - 'A'));
    }
    return left == right;
}

static int WG_DataASCIIStringEqual(const char *left, const char *right)
{
    while (*left != '\0' && *right != '\0'
           && WG_DataASCIIEqual(*left, *right))
    {
        ++left;
        ++right;
    }
    return *left == '\0' && *right == '\0';
}

static int WG_DataASCIIPrefix(const char *text, const char *prefix)
{
    while (*prefix != '\0')
    {
        if (*text == '\0' || !WG_DataASCIIEqual(*text, *prefix))
        {
            return 0;
        }
        ++text;
        ++prefix;
    }
    return 1;
}

static const wg_data_profile_t *WG_DataProfile(wg_game_variant_t variant)
{
    size_t index;

    for (index = 0U;
         index < sizeof(WG_DataProfiles) / sizeof(WG_DataProfiles[0]);
         ++index)
    {
        if (WG_DataProfiles[index].variant == variant)
        {
            return &WG_DataProfiles[index];
        }
    }
    return NULL;
}

static int WG_DataPath(char *destination, size_t destination_size,
                       const char *root, const char *base,
                       const char *extension)
{
    int result = snprintf(destination, destination_size, "%s/%s%s",
                          root, base, extension);
    return result >= 0 && (size_t)result < destination_size;
}

static int WG_DataHasFiles(const char *root, const char *extension,
                           const char *const *names, size_t name_count)
{
    char path[1200];
    size_t index;

    for (index = 0U; index < name_count; ++index)
    {
        if (!WG_DataPath(path, sizeof(path), root, names[index], extension)
            || !WG_FileExists(path))
        {
            return 0;
        }
    }
    return 1;
}

static int WG_DataHasProfile(const char *root,
                             const wg_data_profile_t *profile)
{
    static const char *const map_names[] = { "GAMEMAPS", "MAPHEAD", "VSWAP" };
    static const char *const graphics_names[] =
        { "VGADICT", "VGAGRAPH", "VGAHEAD" };
    static const char *const audio_names[] = { "AUDIOHED", "AUDIOT" };

    return WG_DataHasFiles(root, profile->extension, map_names,
                           sizeof(map_names) / sizeof(map_names[0]))
        && WG_DataHasFiles(root, profile->graphics_extension, graphics_names,
                           sizeof(graphics_names) / sizeof(graphics_names[0]))
        && WG_DataHasFiles(root, profile->audio_extension, audio_names,
                           sizeof(audio_names) / sizeof(audio_names[0]));
}

static int WG_DataReadHeaderCounts(wg_data_set_t *data_set)
{
    char path[1200];
    wg_file_buffer_t file;

    if (!WG_DataPath(path, sizeof(path), data_set->root, "VGAHEAD",
                     data_set->graphics_extension)
        || !WG_LoadFile(path, &file))
    {
        return 0;
    }
    if (file.size == 0 || file.size % 3U != 0)
    {
        WG_FreeFile(&file);
        return 0;
    }
    data_set->graphics_offset_count = file.size / 3U;
    WG_FreeFile(&file);

    if (!WG_DataPath(path, sizeof(path), data_set->root, "AUDIOHED",
                     data_set->audio_extension)
        || !WG_LoadFile(path, &file))
    {
        return 0;
    }
    if (file.size < 8U || file.size % 4U != 0)
    {
        WG_FreeFile(&file);
        return 0;
    }
    data_set->audio_offset_count = file.size / 4U;
    WG_FreeFile(&file);

    if (!WG_DataPath(path, sizeof(path), data_set->root, "MAPHEAD",
                     data_set->extension)
        || !WG_LoadFile(path, &file))
    {
        return 0;
    }
    if (file.size < 2U)
    {
        WG_FreeFile(&file);
        return 0;
    }
    data_set->rlew_tag = WG_ReadLE16(file.data);
    WG_FreeFile(&file);
    return 1;
}

static int WG_DataReadPages(wg_data_set_t *data_set)
{
    char path[1200];
    wg_file_buffer_t file;
    size_t header_size;
    size_t index;

    if (!WG_DataPath(path, sizeof(path), data_set->root, "VSWAP",
                     data_set->extension)
        || !WG_LoadFile(path, &file))
    {
        return 0;
    }
    if (file.size < 6U)
    {
        WG_FreeFile(&file);
        return 0;
    }

    data_set->page_count = WG_ReadLE16(file.data);
    data_set->sprite_start = WG_ReadLE16(file.data + 2);
    data_set->sound_start = WG_ReadLE16(file.data + 4);
    header_size = 6U + (size_t)data_set->page_count * 6U;
    if (data_set->page_count == 0 || data_set->sprite_start > data_set->page_count
        || data_set->sound_start > data_set->page_count
        || data_set->sprite_start > data_set->sound_start
        || header_size > file.size)
    {
        WG_FreeFile(&file);
        return 0;
    }

    data_set->pages = (wg_page_entry_t *)calloc(data_set->page_count,
                                                 sizeof(*data_set->pages));
    if (data_set->pages == NULL)
    {
        WG_FreeFile(&file);
        return 0;
    }

    for (index = 0; index < data_set->page_count; ++index)
    {
        uint32_t offset = WG_ReadLE32(file.data + 6U + index * 4U);
        uint16_t length = WG_ReadLE16(file.data + 6U
                                      + (size_t)data_set->page_count * 4U
                                      + index * 2U);
        if (offset != 0 && ((size_t)offset > file.size
            || (size_t)length > file.size - (size_t)offset))
        {
            WG_FreeFile(&file);
            free(data_set->pages);
            data_set->pages = NULL;
            return 0;
        }
        data_set->pages[index].offset = offset;
        data_set->pages[index].length = length;
    }

    WG_FreeFile(&file);
    return 1;
}

int WG_DataOpenSelected(wg_data_set_t *data_set, const char *root,
                        wg_game_variant_t requested_variant,
                        wg_game_family_t preferred_family)
{
    const wg_data_profile_t *selected = NULL;
    size_t root_length;
    size_t index;
    size_t matches = 0U;

    if (data_set == NULL || root == NULL)
    {
        return 0;
    }
    memset(data_set, 0, sizeof(*data_set));
    root_length = strlen(root);
    if (root_length == 0 || root_length >= sizeof(data_set->root))
    {
        return 0;
    }
    memcpy(data_set->root, root, root_length + 1U);

    if (requested_variant != WG_GAME_UNKNOWN)
    {
        selected = WG_DataProfile(requested_variant);
        if (selected == NULL
            || !WG_DataHasProfile(root, selected))
        {
            WG_DataClose(data_set);
            return 0;
        }
    }
    else
    {
        for (index = 0U;
             index < sizeof(WG_DataProfiles) / sizeof(WG_DataProfiles[0]);
             ++index)
        {
            if (WG_DataHasProfile(root, &WG_DataProfiles[index])
                && (preferred_family == WG_GAME_FAMILY_UNKNOWN
                    || WG_DataProfiles[index].family == preferred_family))
            {
                selected = &WG_DataProfiles[index];
                ++matches;
            }
        }

        if (matches == 0U && preferred_family != WG_GAME_FAMILY_UNKNOWN)
        {
            for (index = 0U;
                 index < sizeof(WG_DataProfiles) / sizeof(WG_DataProfiles[0]);
                 ++index)
            {
                if (WG_DataHasProfile(root, &WG_DataProfiles[index]))
                {
                    selected = &WG_DataProfiles[index];
                    ++matches;
                }
            }
        }

        if (matches != 1U)
        {
            WG_DataClose(data_set);
            return 0;
        }
    }

    data_set->variant = selected->variant;
    memcpy(data_set->extension, selected->extension, 5U);
    memcpy(data_set->graphics_extension, selected->graphics_extension, 5U);
    memcpy(data_set->audio_extension, selected->audio_extension, 5U);

    if (!WG_DataReadHeaderCounts(data_set) || !WG_DataReadPages(data_set))
    {
        WG_DataClose(data_set);
        return 0;
    }
    return 1;
}

int WG_DataOpen(wg_data_set_t *data_set, const char *root)
{
    return WG_DataOpenSelected(data_set, root, WG_GAME_UNKNOWN,
                               WG_GAME_FAMILY_UNKNOWN);
}

wg_game_variant_t WG_DataVariantFromExtension(const char *extension)
{
    size_t index;

    if (extension == NULL)
    {
        return WG_GAME_UNKNOWN;
    }
    if (*extension == '.')
    {
        ++extension;
    }
    for (index = 0U;
         index < sizeof(WG_DataProfiles) / sizeof(WG_DataProfiles[0]);
         ++index)
    {
        if (WG_DataASCIIStringEqual(extension,
                                    WG_DataProfiles[index].extension + 1))
        {
            return WG_DataProfiles[index].variant;
        }
    }
    return WG_GAME_UNKNOWN;
}

int WG_DataParseGame(const char *text, wg_game_variant_t *variant)
{
    if (text == NULL || variant == NULL)
    {
        return 0;
    }
    if (WG_DataASCIIStringEqual(text, "auto"))
    {
        *variant = WG_GAME_UNKNOWN;
        return 1;
    }
    *variant = WG_DataVariantFromExtension(text);
    return *variant != WG_GAME_UNKNOWN;
}

wg_game_family_t WG_DataVariantFamily(wg_game_variant_t variant)
{
    const wg_data_profile_t *profile = WG_DataProfile(variant);
    return profile != NULL ? profile->family : WG_GAME_FAMILY_UNKNOWN;
}

wg_game_family_t WG_DataExecutableFamily(const char *path)
{
    const char *base;
    const char *cursor;

    if (path == NULL)
    {
        return WG_GAME_FAMILY_UNKNOWN;
    }
    base = path;
    for (cursor = path; *cursor != '\0'; ++cursor)
    {
        if (*cursor == '/' || *cursor == '\\')
        {
            base = cursor + 1;
        }
    }
    if (WG_DataASCIIPrefix(base, "wolf"))
    {
        return WG_GAME_FAMILY_WOLF3D;
    }
    if (WG_DataASCIIPrefix(base, "spear")
        || WG_DataASCIIPrefix(base, "sod"))
    {
        return WG_GAME_FAMILY_SPEAR;
    }
    return WG_GAME_FAMILY_UNKNOWN;
}

const char *WG_DataVariantExtension(wg_game_variant_t variant)
{
    const wg_data_profile_t *profile = WG_DataProfile(variant);
    return profile != NULL ? profile->extension + 1 : NULL;
}

void WG_DataClose(wg_data_set_t *data_set)
{
    if (data_set == NULL)
    {
        return;
    }
    free(data_set->pages);
    memset(data_set, 0, sizeof(*data_set));
}

const char *WG_DataVariantName(wg_game_variant_t variant)
{
    const wg_data_profile_t *profile = WG_DataProfile(variant);
    return profile != NULL ? profile->name : "unknown game data";
}

size_t WG_DataSoundCount(wg_game_variant_t variant)
{
    return WG_DataVariantFamily(variant) == WG_GAME_FAMILY_SPEAR
               ? 81U : 87U;
}

size_t WG_DataMusicBase(wg_game_variant_t variant)
{
    return WG_DataVariantFamily(variant) == WG_GAME_FAMILY_SPEAR
               ? 243U : 261U;
}
