#include "WG_MAPS.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "WG_COMPAT.h"

#include "ID_CA.h"
#include "WG_ENDIAN.h"

#define WG_MAP_HEADER_SIZE 38U
#define WG_SPARSE_MAP_OFFSET 0xffffffffU
#define WG_ORIGINAL_MAP_COUNT 60U

static int WG_MapsPath(char *destination, size_t destination_size,
                       const wg_data_set_t *data_set, const char *base)
{
    int result = snprintf(destination, destination_size, "%s/%s%s",
                          data_set->root, base, data_set->extension);
    return result >= 0 && (size_t)result < destination_size;
}

int WG_MapsOpen(wg_maps_t *maps, const wg_data_set_t *data_set)
{
    char path[1200];
    wg_file_buffer_t header;
    size_t index;

    if (maps == NULL || data_set == NULL)
    {
        return 0;
    }
    memset(maps, 0, sizeof(*maps));
    memset(&header, 0, sizeof(header));
    if (!WG_MapsPath(path, sizeof(path), data_set, "MAPHEAD")
        || !WG_LoadFile(path, &header) || header.size < 6U
        || (header.size - 2U) % 4U != 0)
    {
        goto failure;
    }
    maps->rlew_tag = WG_ReadLE16(header.data);
    maps->header_offset_count = (header.size - 2U) / 4U;
    if (maps->header_offset_count > WG_ORIGINAL_MAP_COUNT)
    {
        maps->header_offset_count = WG_ORIGINAL_MAP_COUNT;
    }
    maps->header_offsets = (uint32_t *)malloc(maps->header_offset_count
                                               * sizeof(*maps->header_offsets));
    if (maps->header_offsets == NULL)
    {
        goto failure;
    }
    for (index = 0; index < maps->header_offset_count; ++index)
    {
        maps->header_offsets[index] = WG_ReadLE32(header.data + 2U + index * 4U);
    }
    if (!WG_MapsPath(path, sizeof(path), data_set, "GAMEMAPS")
        || !WG_LoadFile(path, &maps->data))
    {
        goto failure;
    }
    for (index = 0; index < maps->header_offset_count; ++index)
    {
        uint32_t offset = maps->header_offsets[index];
        if (offset != WG_SPARSE_MAP_OFFSET
            && ((size_t)offset > maps->data.size
                || WG_MAP_HEADER_SIZE > maps->data.size - (size_t)offset))
        {
            goto failure;
        }
    }
    WG_FreeFile(&header);
    return 1;

failure:
    WG_FreeFile(&header);
    WG_MapsClose(maps);
    return 0;
}

void WG_MapsClose(wg_maps_t *maps)
{
    if (maps == NULL)
    {
        return;
    }
    free(maps->header_offsets);
    WG_FreeFile(&maps->data);
    memset(maps, 0, sizeof(*maps));
}

static int WG_MapsDecodePlane(const wg_maps_t *maps, uint32_t start,
                              uint16_t compressed_size, size_t cells,
                              uint16_t *destination)
{
    const uint8_t *source;
    size_t expanded_bytes;
    size_t expanded_words;
    uint16_t *intermediate;
    int result;

    if ((size_t)start > maps->data.size
        || compressed_size < 4U
        || (size_t)compressed_size > maps->data.size - (size_t)start)
    {
        return 0;
    }
    source = maps->data.data + start;
    expanded_bytes = WG_ReadLE16(source);
    if (expanded_bytes < 4U || expanded_bytes % 2U != 0)
    {
        return 0;
    }
    expanded_words = expanded_bytes / 2U;
    intermediate = (uint16_t *)malloc(expanded_words * sizeof(*intermediate));
    if (intermediate == NULL)
    {
        return 0;
    }
    result = WG_CarmackExpand(source + 2U, compressed_size - 2U,
                              intermediate, expanded_words);
    if (result && intermediate[0] == cells * sizeof(*destination))
    {
        result = WG_RLEWExpand(intermediate + 1U, expanded_words - 1U,
                               destination, cells, maps->rlew_tag);
    }
    else
    {
        result = 0;
    }
    free(intermediate);
    return result;
}

int WG_MapsLoad(const wg_maps_t *maps, size_t map_number, wg_map_t *map)
{
    const uint8_t *header;
    uint32_t plane_starts[3];
    uint16_t plane_lengths[3];
    size_t cells;
    size_t plane;

    if (maps == NULL || map == NULL || maps->header_offsets == NULL
        || map_number >= maps->header_offset_count
        || maps->header_offsets[map_number] == WG_SPARSE_MAP_OFFSET)
    {
        return 0;
    }
    memset(map, 0, sizeof(*map));
    header = maps->data.data + maps->header_offsets[map_number];
    for (plane = 0; plane < 3U; ++plane)
    {
        plane_starts[plane] = WG_ReadLE32(header + plane * 4U);
        plane_lengths[plane] = WG_ReadLE16(header + 12U + plane * 2U);
    }
    map->width = WG_ReadLE16(header + 18U);
    map->height = WG_ReadLE16(header + 20U);
    memcpy(map->name, header + 22U, 16U);
    map->name[16] = '\0';
    if (map->width == 0 || map->height == 0
        || (size_t)map->width > (size_t)-1 / map->height)
    {
        WG_MapFree(map);
        return 0;
    }
    cells = (size_t)map->width * map->height;
    if (cells > (size_t)-1 / sizeof(*map->planes[0]))
    {
        WG_MapFree(map);
        return 0;
    }
    for (plane = 0; plane < WG_MAP_PLANES; ++plane)
    {
        map->planes[plane] = (uint16_t *)malloc(cells
                                                * sizeof(*map->planes[plane]));
        if (map->planes[plane] == NULL
            || !WG_MapsDecodePlane(maps, plane_starts[plane],
                                   plane_lengths[plane], cells,
                                   map->planes[plane]))
        {
            WG_MapFree(map);
            return 0;
        }
    }
    return 1;
}

void WG_MapFree(wg_map_t *map)
{
    size_t plane;

    if (map == NULL)
    {
        return;
    }
    for (plane = 0; plane < WG_MAP_PLANES; ++plane)
    {
        free(map->planes[plane]);
    }
    memset(map, 0, sizeof(*map));
}
