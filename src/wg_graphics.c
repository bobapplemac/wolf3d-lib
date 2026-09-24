#include "wg_graphics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wg_endian.h"

#define WG_SPARSE_OFFSET 0x00ffffffU

static int WG_GraphicsPath(char *destination, size_t destination_size,
                           const wg_data_set_t *data_set, const char *base)
{
    int result = snprintf(destination, destination_size, "%s/%s%s",
                          data_set->root, base, data_set->extension);
    return result >= 0 && (size_t)result < destination_size;
}

int WG_GraphicsOpen(wg_graphics_t *graphics, const wg_data_set_t *data_set)
{
    char path[1200];
    wg_file_buffer_t dictionary;
    wg_file_buffer_t header;
    size_t index;

    if (graphics == NULL || data_set == NULL)
    {
        return 0;
    }
    memset(graphics, 0, sizeof(*graphics));
    memset(&dictionary, 0, sizeof(dictionary));
    memset(&header, 0, sizeof(header));

    if (!WG_GraphicsPath(path, sizeof(path), data_set, "VGADICT")
        || !WG_LoadFile(path, &dictionary)
        || dictionary.size < sizeof(graphics->dictionary))
    {
        goto failure;
    }
    for (index = 0; index < 255U; ++index)
    {
        graphics->dictionary[index].bit0 =
            WG_ReadLE16(dictionary.data + index * 4U);
        graphics->dictionary[index].bit1 =
            WG_ReadLE16(dictionary.data + index * 4U + 2U);
    }

    if (!WG_GraphicsPath(path, sizeof(path), data_set, "VGAHEAD")
        || !WG_LoadFile(path, &header) || header.size < 6U
        || header.size % 3U != 0)
    {
        goto failure;
    }
    graphics->offset_count = header.size / 3U;
    graphics->offsets = (uint32_t *)malloc(graphics->offset_count
                                           * sizeof(*graphics->offsets));
    if (graphics->offsets == NULL)
    {
        goto failure;
    }
    for (index = 0; index < graphics->offset_count; ++index)
    {
        graphics->offsets[index] = WG_ReadLE24(header.data + index * 3U);
    }

    if (!WG_GraphicsPath(path, sizeof(path), data_set, "VGAGRAPH")
        || !WG_LoadFile(path, &graphics->graph))
    {
        goto failure;
    }

    for (index = 0; index < graphics->offset_count; ++index)
    {
        uint32_t offset = graphics->offsets[index];
        if (offset != WG_SPARSE_OFFSET && (size_t)offset > graphics->graph.size)
        {
            goto failure;
        }
    }

    WG_FreeFile(&dictionary);
    WG_FreeFile(&header);
    return 1;

failure:
    WG_FreeFile(&dictionary);
    WG_FreeFile(&header);
    WG_GraphicsClose(graphics);
    return 0;
}

void WG_GraphicsClose(wg_graphics_t *graphics)
{
    if (graphics == NULL)
    {
        return;
    }
    free(graphics->offsets);
    WG_FreeFile(&graphics->graph);
    memset(graphics, 0, sizeof(*graphics));
}

int WG_GraphicsDecodeChunk(const wg_graphics_t *graphics, size_t chunk,
                           uint8_t **decoded, size_t *decoded_size)
{
    size_t next;
    size_t start;
    size_t end;
    size_t expanded_size;
    uint8_t *output;

    if (graphics == NULL || decoded == NULL || decoded_size == NULL
        || graphics->offset_count < 2U || chunk + 1U >= graphics->offset_count)
    {
        return 0;
    }
    *decoded = NULL;
    *decoded_size = 0;

    if (graphics->offsets[chunk] == WG_SPARSE_OFFSET)
    {
        return 0;
    }
    next = chunk + 1U;
    while (next < graphics->offset_count
           && graphics->offsets[next] == WG_SPARSE_OFFSET)
    {
        ++next;
    }
    if (next >= graphics->offset_count)
    {
        return 0;
    }

    start = graphics->offsets[chunk];
    end = graphics->offsets[next];
    if (end < start || end > graphics->graph.size || end - start < 4U)
    {
        return 0;
    }
    expanded_size = WG_ReadLE32(graphics->graph.data + start);
    if (expanded_size == 0)
    {
        return 0;
    }

    output = (uint8_t *)malloc(expanded_size);
    if (output == NULL)
    {
        return 0;
    }
    if (!WG_HuffmanExpand(graphics->graph.data + start + 4U,
                          end - start - 4U, output, expanded_size,
                          graphics->dictionary))
    {
        free(output);
        return 0;
    }

    *decoded = output;
    *decoded_size = expanded_size;
    return 1;
}

int WG_GraphicsDecodeTitle(const wg_graphics_t *graphics,
                           wg_game_variant_t variant,
                           uint8_t framebuffer[320 * 200])
{
    size_t title_chunk;
    uint8_t *planar;
    size_t planar_size;
    size_t y;
    size_t x;

    if (framebuffer == NULL)
    {
        return 0;
    }
    if (variant == WG_GAME_WOLF3D_FULL_GT_14)
    {
        title_chunk = 87;
    }
    else if (variant == WG_GAME_WOLF3D_SHAREWARE_14)
    {
        title_chunk = 99;
    }
    else
    {
        return 0;
    }

    if (!WG_GraphicsDecodeChunk(graphics, title_chunk, &planar, &planar_size)
        || planar_size != 320U * 200U)
    {
        free(planar);
        return 0;
    }

    for (y = 0; y < 200U; ++y)
    {
        for (x = 0; x < 320U; ++x)
        {
            size_t planar_index = y * 80U + (x >> 2)
                                + (x & 3U) * 80U * 200U;
            framebuffer[y * 320U + x] = planar[planar_index];
        }
    }
    free(planar);
    return 1;
}

