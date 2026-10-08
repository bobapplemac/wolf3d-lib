#ifndef WG_GRAPHICS_H
#define WG_GRAPHICS_H

#include <stddef.h>
#include <stdint.h>

#include "ID_CA.h"
#include "WG_DATA.h"
#include "WG_FILE.h"

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
int WG_GraphicsDecodeTitleWithPalette(const wg_graphics_t *graphics,
                                      wg_game_variant_t variant,
                                      uint8_t framebuffer[320 * 200],
                                      uint8_t palette[256 * 3]);
int WG_GraphicsDecodeScreenWithPalette(const wg_graphics_t *graphics,
                                       size_t screen_chunk,
                                       size_t palette_chunk,
                                       uint8_t framebuffer[320 * 200],
                                       uint8_t palette[256 * 3]);
int WG_GraphicsDecodeTextScreen(const wg_graphics_t *graphics, int error,
                                uint8_t cells[80 * 25 * 2]);

#endif
