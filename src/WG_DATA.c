#include "WG_DATA.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "WG_ENDIAN.h"
#include "WG_FILE.h"

static int WG_DataPath(char *destination, size_t destination_size,
                       const char *root, const char *base,
                       const char *extension)
{
    int result = snprintf(destination, destination_size, "%s/%s%s",
                          root, base, extension);
    return result >= 0 && (size_t)result < destination_size;
}

static int WG_DataHasCoreFiles(const char *root, const char *extension)
{
    static const char *const names[] =
    {
        "AUDIOHED", "AUDIOT", "GAMEMAPS", "MAPHEAD",
        "VGADICT", "VGAGRAPH", "VGAHEAD", "VSWAP"
    };
    char path[1200];
    size_t index;

    for (index = 0; index < sizeof(names) / sizeof(names[0]); ++index)
    {
        if (!WG_DataPath(path, sizeof(path), root, names[index], extension)
            || !WG_FileExists(path))
        {
            return 0;
        }
    }
    return 1;
}

static int WG_DataReadHeaderCounts(wg_data_set_t *data_set)
{
    char path[1200];
    wg_file_buffer_t file;

    if (!WG_DataPath(path, sizeof(path), data_set->root, "VGAHEAD",
                     data_set->extension)
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
                     data_set->extension)
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

int WG_DataOpen(wg_data_set_t *data_set, const char *root)
{
    size_t root_length;

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

    if (WG_DataHasCoreFiles(root, ".WL6"))
    {
        data_set->variant = WG_GAME_WOLF3D_FULL_GT_14;
        memcpy(data_set->extension, ".WL6", 5);
    }
    else if (WG_DataHasCoreFiles(root, ".WL1"))
    {
        data_set->variant = WG_GAME_WOLF3D_SHAREWARE_14;
        memcpy(data_set->extension, ".WL1", 5);
    }
    else
    {
        WG_DataClose(data_set);
        return 0;
    }

    if (!WG_DataReadHeaderCounts(data_set) || !WG_DataReadPages(data_set))
    {
        WG_DataClose(data_set);
        return 0;
    }
    return 1;
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
    switch (variant)
    {
        case WG_GAME_WOLF3D_SHAREWARE_14:
            return "Wolfenstein 3D v1.4 shareware";
        case WG_GAME_WOLF3D_FULL_GT_14:
            return "Wolfenstein 3D v1.4 GT/ID/Activision";
        default:
            return "unknown Wolfenstein 3D data";
    }
}

