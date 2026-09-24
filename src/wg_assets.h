#ifndef WG_ASSETS_H
#define WG_ASSETS_H

#include <stdint.h>

#include "wg_pages.h"

#define WG_TEXTURE_SIZE 64

typedef struct wg_sprite_image
{
    uint8_t pixels[WG_TEXTURE_SIZE * WG_TEXTURE_SIZE];
    uint8_t mask[WG_TEXTURE_SIZE * WG_TEXTURE_SIZE];
    uint16_t left;
    uint16_t right;
} wg_sprite_image_t;

int WG_DecodeWall(const wg_pages_t *pages, size_t wall,
                  uint8_t pixels[WG_TEXTURE_SIZE * WG_TEXTURE_SIZE]);
int WG_DecodeSprite(const wg_pages_t *pages, size_t sprite,
                    wg_sprite_image_t *image);

#endif
