#ifndef WG_SCALE_H
#define WG_SCALE_H

#include <stdint.h>

#include "wg_assets.h"
#include "wg_video.h"

int WG_ScaleWallPost(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    int view_x, int view_y, int view_width, int view_height,
    int post_x, int post_width,
    const uint8_t texture[WG_TEXTURE_SIZE * WG_TEXTURE_SIZE],
    unsigned texture_column, int32_t wall_height);

#endif
