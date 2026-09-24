#include "WG_ASSETS.h"

#include <string.h>

#include "WG_ENDIAN.h"

int WG_DecodeWall(const wg_pages_t *pages, size_t wall,
                  uint8_t pixels[WG_TEXTURE_SIZE * WG_TEXTURE_SIZE])
{
    const uint8_t *page;
    size_t page_size;
    size_t x;
    size_t y;

    if (pages == NULL || pages->data_set == NULL || pixels == NULL
        || wall >= pages->data_set->sprite_start
        || !WG_PagesGet(pages, wall, &page, &page_size)
        || page_size != WG_TEXTURE_SIZE * WG_TEXTURE_SIZE)
    {
        return 0;
    }
    for (y = 0; y < WG_TEXTURE_SIZE; ++y)
    {
        for (x = 0; x < WG_TEXTURE_SIZE; ++x)
        {
            pixels[y * WG_TEXTURE_SIZE + x] =
                page[x * WG_TEXTURE_SIZE + y];
        }
    }
    return 1;
}

int WG_DecodeSprite(const wg_pages_t *pages, size_t sprite,
                    wg_sprite_image_t *image)
{
    const uint8_t *page;
    size_t page_size;
    size_t page_number;
    size_t x;

    if (pages == NULL || pages->data_set == NULL || image == NULL
        || sprite >= (size_t)(pages->data_set->sound_start
                              - pages->data_set->sprite_start))
    {
        return 0;
    }
    page_number = (size_t)pages->data_set->sprite_start + sprite;
    if (!WG_PagesGet(pages, page_number, &page, &page_size)
        || page_size < 6U)
    {
        return 0;
    }
    memset(image, 0, sizeof(*image));
    image->left = WG_ReadLE16(page);
    image->right = WG_ReadLE16(page + 2U);
    if (image->left >= WG_TEXTURE_SIZE || image->right >= WG_TEXTURE_SIZE
        || image->left > image->right
        || 4U + (size_t)(image->right - image->left + 1U) * 2U > page_size)
    {
        return 0;
    }

    for (x = image->left; x <= image->right; ++x)
    {
        size_t command = WG_ReadLE16(page + 4U + (x - image->left) * 2U);

        if (command >= page_size)
        {
            return 0;
        }
        for (;;)
        {
            uint16_t end_encoded;
            int16_t pixel_offset;
            uint16_t start_encoded;
            size_t start;
            size_t end;
            size_t y;

            if (command + 2U > page_size)
            {
                return 0;
            }
            end_encoded = WG_ReadLE16(page + command);
            command += 2U;
            if (end_encoded == 0)
            {
                break;
            }
            if (command + 4U > page_size || (end_encoded & 1U) != 0)
            {
                return 0;
            }
            pixel_offset = (int16_t)WG_ReadLE16(page + command);
            start_encoded = WG_ReadLE16(page + command + 2U);
            command += 4U;
            if ((start_encoded & 1U) != 0)
            {
                return 0;
            }
            start = start_encoded / 2U;
            end = end_encoded / 2U;
            if (start >= end || end > WG_TEXTURE_SIZE
                || (int)pixel_offset + (int)start < 0
                || (size_t)((int)pixel_offset + (int)end) > page_size)
            {
                return 0;
            }
            for (y = start; y < end; ++y)
            {
                image->pixels[y * WG_TEXTURE_SIZE + x] =
                    page[(size_t)((int)pixel_offset + (int)y)];
                image->mask[y * WG_TEXTURE_SIZE + x] = 1;
            }
        }
    }
    return 1;
}
