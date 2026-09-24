#ifndef WL_SCALE_H
#define WL_SCALE_H

#include <stdint.h>

#include "WG_ASSETS.h"
#include "ID_VL.h"
#include "WL_MAIN.h"

int WG_ScaleWallPost(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    int view_x, int view_y, int view_width, int view_height,
    int post_x, int post_width,
    const uint8_t texture[WG_TEXTURE_SIZE * WG_TEXTURE_SIZE],
    unsigned texture_column, int32_t wall_height);

int WG_ScaleSprite(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    int view_x, int view_y, int view_width, int view_height, int x_center,
    const wg_sprite_image_t *sprite, unsigned height);

int WG_ScaleSpriteClipped(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    int view_x, int view_y, int view_width, int view_height, int x_center,
    const wg_sprite_image_t *sprite, unsigned height,
    const int32_t wall_height[WG_MAX_VIEW_WIDTH]);

#endif
