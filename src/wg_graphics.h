#ifndef WG_GRAPHICS_H
#define WG_GRAPHICS_H

#include <stddef.h>
#include <stdint.h>

#include "wg_compression.h"
#include "wg_data.h"
#include "wg_file.h"

typedef struct wg_graphics
{
    wg_game_variant_t variant;
    wg_huffman_node_t dictionary[255];
    uint32_t *offsets;
    size_t offset_count;
    struct wg_picture_dimensions *pictures;
    size_t picture_count;
    wg_file_buffer_t graph;
} wg_graphics_t;

typedef struct wg_picture_dimensions
{
    uint16_t width;
    uint16_t height;
} wg_picture_dimensions_t;

int WG_GraphicsOpen(wg_graphics_t *graphics, const wg_data_set_t *data_set);
void WG_GraphicsClose(wg_graphics_t *graphics);
int WG_GraphicsDecodeChunk(const wg_graphics_t *graphics, size_t chunk,
                           uint8_t **decoded, size_t *decoded_size);
int WG_GraphicsDecodePicture(const wg_graphics_t *graphics, size_t chunk,
                             uint8_t **pixels, uint16_t *width,
                             uint16_t *height);
int WG_GraphicsDecodeTitle(const wg_graphics_t *graphics,
                           wg_game_variant_t variant,
                           uint8_t framebuffer[320 * 200]);

#endif
