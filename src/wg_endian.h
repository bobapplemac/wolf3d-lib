#ifndef WG_ENDIAN_H
#define WG_ENDIAN_H

#include <stdint.h>

static uint16_t WG_ReadLE16(const uint8_t *source)
{
    return (uint16_t)((uint16_t)source[0]
                    | ((uint16_t)source[1] << 8));
}

static uint32_t WG_ReadLE24(const uint8_t *source)
{
    return (uint32_t)source[0]
         | ((uint32_t)source[1] << 8)
         | ((uint32_t)source[2] << 16);
}

static uint32_t WG_ReadLE32(const uint8_t *source)
{
    return (uint32_t)source[0]
         | ((uint32_t)source[1] << 8)
         | ((uint32_t)source[2] << 16)
         | ((uint32_t)source[3] << 24);
}

#endif

