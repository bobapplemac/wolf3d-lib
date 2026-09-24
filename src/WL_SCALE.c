/* Portable wall-post, ScaleShape, and SimpleScaleShape replacements. */
#include "WL_SCALE.h"

#include <stddef.h>

/* Portable form of the original generated ScalePost wall scaler. */
int WG_ScaleWallPost(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    int view_x, int view_y, int view_width, int view_height,
    int post_x, int post_width,
    const uint8_t texture[WG_TEXTURE_SIZE * WG_TEXTURE_SIZE],
    unsigned texture_column, int32_t wall_height)
{
    int source_count;
    int source_step;
    int source_y;
    int first_y;
    int last_y;
    int x;

    if (framebuffer == NULL || texture == NULL
        || view_x < 0 || view_y < 0 || view_width <= 0 || view_height <= 0
        || view_x + view_width > WG_VIDEO_WIDTH
        || view_y + view_height > WG_VIDEO_HEIGHT
        || post_x < 0 || post_width <= 0
        || post_x + post_width > view_width
        || texture_column >= WG_TEXTURE_SIZE || wall_height < 0)
    {
        return 0;
    }

    source_count = source_step = wall_height >> 3;
    if (source_step <= 0)
    {
        source_step = 100;
    }
    first_y = view_height / 2 - source_count;
    if (first_y < 0)
    {
        first_y = 0;
    }
    last_y = view_height / 2 + source_count - 1;
    source_y = WG_TEXTURE_SIZE - 1;

    while (last_y >= view_height)
    {
        source_count -= WG_TEXTURE_SIZE / 2;
        while (source_count <= 0)
        {
            source_count += source_step;
            --source_y;
        }
        --last_y;
    }
    if (source_y < 0)
    {
        return 1;
    }

    while (first_y <= last_y)
    {
        uint8_t color =
            texture[source_y * WG_TEXTURE_SIZE + texture_column];

        for (x = 0; x < post_width; ++x)
        {
            framebuffer[(view_y + last_y) * WG_VIDEO_WIDTH
                        + view_x + post_x + x] = color;
        }
        source_count -= WG_TEXTURE_SIZE / 2;
        if (source_count <= 0)
        {
            do
            {
                source_count += source_step;
                --source_y;
            }
            while (source_count <= 0);
            if (source_y < 0)
            {
                break;
            }
        }
        --last_y;
    }
    return 1;
}

static int WG_ScaleSpriteInternal(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    int view_x, int view_y, int view_width, int view_height, int x_center,
    const wg_sprite_image_t *sprite, unsigned height,
    const int32_t wall_height[WG_MAX_VIEW_WIDTH])
{
    unsigned scale;
    unsigned pixel_height;
    int actual_x;
    int upper_edge;
    unsigned source_x;

    if (framebuffer == NULL || sprite == NULL
        || view_x < 0 || view_y < 0 || view_width <= 0 || view_height <= 0
        || view_x + view_width > WG_VIDEO_WIDTH
        || view_y + view_height > WG_VIDEO_HEIGHT || height < 2U)
    {
        return 0;
    }
    scale = height >> 1;
    pixel_height = scale * 2U;
    actual_x = x_center - (int)scale;
    upper_edge = view_height / 2 - (int)scale;

    for (source_x = sprite->left; source_x <= sprite->right; ++source_x)
    {
        int left = (int)((source_x * pixel_height) >> 6) + actual_x;
        int right = (int)(((source_x + 1U) * pixel_height) >> 6) + actual_x;
        unsigned source_y;

        if (left < 0)
        {
            left = 0;
        }
        if (right > view_width)
        {
            right = view_width;
        }
        if (left >= right)
        {
            continue;
        }
        if (wall_height != NULL)
        {
            while (left < right && wall_height[left] >= (int32_t)height)
            {
                ++left;
            }
            while (right > left && wall_height[right - 1] >= (int32_t)height)
            {
                --right;
            }
        }
        for (source_y = 0; source_y < WG_TEXTURE_SIZE; ++source_y)
        {
            int top;
            int bottom;
            int x;
            int y;
            uint8_t color;

            if (sprite->mask[source_y * WG_TEXTURE_SIZE + source_x] == 0U)
            {
                continue;
            }
            top = (int)((source_y * pixel_height) >> 6) + upper_edge;
            bottom = (int)(((source_y + 1U) * pixel_height) >> 6) + upper_edge;
            if (top < 0)
            {
                top = 0;
            }
            if (bottom > view_height)
            {
                bottom = view_height;
            }
            color = sprite->pixels[source_y * WG_TEXTURE_SIZE + source_x];
            for (y = top; y < bottom; ++y)
            {
                for (x = left; x < right; ++x)
                {
                    if (wall_height == NULL
                        || wall_height[x] < (int32_t)height)
                    {
                        framebuffer[(view_y + y) * WG_VIDEO_WIDTH
                                    + view_x + x] = color;
                    }
                }
            }
        }
    }
    return 1;
}

int WG_ScaleSprite(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    int view_x, int view_y, int view_width, int view_height, int x_center,
    const wg_sprite_image_t *sprite, unsigned height)
{
    return WG_ScaleSpriteInternal(framebuffer, view_x, view_y, view_width,
                                  view_height, x_center, sprite, height, NULL);
}

int WG_ScaleSpriteClipped(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    int view_x, int view_y, int view_width, int view_height, int x_center,
    const wg_sprite_image_t *sprite, unsigned height,
    const int32_t wall_height[WG_MAX_VIEW_WIDTH])
{
    if (wall_height == NULL)
    {
        return 0;
    }
    return WG_ScaleSpriteInternal(framebuffer, view_x, view_y, view_width,
                                  view_height, x_center, sprite, height,
                                  wall_height);
}
