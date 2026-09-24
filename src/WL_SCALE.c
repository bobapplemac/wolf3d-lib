/* Portable replacement for the generated wall-post scalers used by WL_DRAW.C. */
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
