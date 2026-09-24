#include "WG_GRAPHICS.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "WG_ENDIAN.h"

#define WG_SPARSE_OFFSET 0x00ffffffU
#define WG_START_PICTURES 3U

static size_t WG_GraphicsPictureCount(wg_game_variant_t variant)
{
    if (variant == WG_GAME_WOLF3D_FULL_GT_14)
    {
        return 132U;
    }
    if (variant == WG_GAME_WOLF3D_SHAREWARE_14)
    {
        return 144U;
    }
    return 0;
}

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
    graphics->variant = data_set->variant;
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

    graphics->picture_count = WG_GraphicsPictureCount(data_set->variant);
    if (graphics->picture_count != 0)
    {
        uint8_t *table;
        size_t table_size;

        if (!WG_GraphicsDecodeChunk(graphics, 0, &table, &table_size)
            || table_size != graphics->picture_count * 4U)
        {
            free(table);
            goto failure;
        }
        graphics->pictures = (wg_picture_dimensions_t *)malloc(
            graphics->picture_count * sizeof(*graphics->pictures));
        if (graphics->pictures == NULL)
        {
            free(table);
            goto failure;
        }
        for (index = 0; index < graphics->picture_count; ++index)
        {
            graphics->pictures[index].width = WG_ReadLE16(table + index * 4U);
            graphics->pictures[index].height = WG_ReadLE16(table + index * 4U + 2U);
        }
        free(table);
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
    free(graphics->pictures);
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
    size_t payload_offset;

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
    if (end < start || end > graphics->graph.size)
    {
        return 0;
    }
    if ((graphics->variant == WG_GAME_WOLF3D_FULL_GT_14 && chunk == 135U)
        || (graphics->variant == WG_GAME_WOLF3D_SHAREWARE_14 && chunk == 147U))
    {
        /* The generated headers give TILE8 an implicit size. Keep it bounded;
           unlike the original routine, Huffman expansion must never read into
           the following chunk if a particular data revision disagrees. */
        expanded_size = 72U * 8U * 8U;
        payload_offset = 0;
    }
    else
    {
        if (end - start < 4U)
        {
            return 0;
        }
        expanded_size = WG_ReadLE32(graphics->graph.data + start);
        payload_offset = 4U;
    }
    if (expanded_size == 0)
    {
        return 0;
    }

    output = (uint8_t *)malloc(expanded_size);
    if (output == NULL)
    {
        return 0;
    }
    if (!WG_HuffmanExpand(graphics->graph.data + start + payload_offset,
                          end - start - payload_offset, output, expanded_size,
                          graphics->dictionary))
    {
        free(output);
        return 0;
    }

    *decoded = output;
    *decoded_size = expanded_size;
    return 1;
}

int WG_GraphicsDecodePicture(const wg_graphics_t *graphics, size_t chunk,
                             uint8_t **pixels, uint16_t *width,
                             uint16_t *height)
{
    size_t picture;
    size_t pixel_count;
    uint8_t *planar;
    size_t planar_size;
    uint8_t *chunky;
    size_t y;
    size_t x;

    if (graphics == NULL || pixels == NULL || width == NULL || height == NULL
        || chunk < WG_START_PICTURES)
    {
        return 0;
    }
    *pixels = NULL;
    *width = 0;
    *height = 0;
    picture = chunk - WG_START_PICTURES;
    if (picture >= graphics->picture_count
        || graphics->pictures[picture].width == 0
        || graphics->pictures[picture].width % 4U != 0
        || graphics->pictures[picture].height == 0)
    {
        return 0;
    }
    pixel_count = (size_t)graphics->pictures[picture].width
                * graphics->pictures[picture].height;
    if (!WG_GraphicsDecodeChunk(graphics, chunk, &planar, &planar_size)
        || planar_size != pixel_count)
    {
        free(planar);
        return 0;
    }
    chunky = (uint8_t *)malloc(pixel_count);
    if (chunky == NULL)
    {
        free(planar);
        return 0;
    }
    for (y = 0; y < graphics->pictures[picture].height; ++y)
    {
        for (x = 0; x < graphics->pictures[picture].width; ++x)
        {
            size_t plane_width = graphics->pictures[picture].width / 4U;
            size_t planar_index = y * plane_width + (x >> 2)
                                + (x & 3U) * plane_width
                                * graphics->pictures[picture].height;
            chunky[y * graphics->pictures[picture].width + x] =
                planar[planar_index];
        }
    }
    free(planar);
    *pixels = chunky;
    *width = graphics->pictures[picture].width;
    *height = graphics->pictures[picture].height;
    return 1;
}

int WG_GraphicsDecodeTitle(const wg_graphics_t *graphics,
                           wg_game_variant_t variant,
                           uint8_t framebuffer[320 * 200])
{
    size_t title_chunk;
    uint8_t *pixels;
    uint16_t width;
    uint16_t height;

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

    if (!WG_GraphicsDecodePicture(graphics, title_chunk, &pixels,
                                  &width, &height)
        || width != 320U || height != 200U)
    {
        free(pixels);
        return 0;
    }
    memcpy(framebuffer, pixels, 320U * 200U);
    free(pixels);
    return 1;
}
