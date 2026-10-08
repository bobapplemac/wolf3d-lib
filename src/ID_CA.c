/* Portable descendants of CAL_HuffExpand, CAL_CarmackExpand, and CA_RLEWexpand
   from the original ID_CA.C. File/resource ownership lives in WG_* facades. */
#include "ID_CA.h"

#include "WG_ENDIAN.h"

#define WG_CARMACK_NEAR_TAG 0xa7U
#define WG_CARMACK_FAR_TAG 0xa8U

int WG_HuffmanExpand(const uint8_t *source, size_t source_size,
                     uint8_t *destination, size_t destination_size,
                     const wg_huffman_node_t nodes[255])
{
    size_t bit_index;
    size_t destination_index;
    const wg_huffman_node_t *node;

    if (source == NULL || destination == NULL || nodes == NULL
        || destination_size == 0 || source_size == 0)
    {
        return 0;
    }

    bit_index = 0;
    destination_index = 0;
    node = &nodes[254];

    while (destination_index < destination_size)
    {
        uint16_t node_value;
        uint8_t mask;

        if (bit_index / 8U >= source_size)
        {
            return 0;
        }
        mask = (uint8_t)(1U << (bit_index % 8U));
        node_value = (source[bit_index / 8U] & mask) != 0
                         ? node->bit1 : node->bit0;
        ++bit_index;

        if (node_value < 256U)
        {
            destination[destination_index++] = (uint8_t)node_value;
            node = &nodes[254];
        }
        else
        {
            size_t node_index = (size_t)node_value - 256U;
            if (node_index >= 255U)
            {
                return 0;
            }
            node = &nodes[node_index];
        }
    }
    return 1;
}

int WG_CarmackExpand(const uint8_t *source, size_t source_size,
                     uint16_t *destination, size_t destination_words)
{
    size_t source_index = 0;
    size_t destination_index = 0;

    if (source == NULL || destination == NULL)
    {
        return 0;
    }

    while (destination_index < destination_words)
    {
        uint16_t word;
        uint16_t high;
        size_t count;
        size_t copy_index;

        if (source_index + 2U > source_size)
        {
            return 0;
        }
        word = WG_ReadLE16(source + source_index);
        source_index += 2;
        high = word >> 8;

        if (high != WG_CARMACK_NEAR_TAG && high != WG_CARMACK_FAR_TAG)
        {
            destination[destination_index++] = word;
            continue;
        }

        count = word & 0xffU;
        if (count == 0)
        {
            if (source_index >= source_size)
            {
                return 0;
            }
            destination[destination_index++] = (uint16_t)(word | source[source_index++]);
            continue;
        }
        if (count > destination_words - destination_index)
        {
            return 0;
        }

        if (high == WG_CARMACK_NEAR_TAG)
        {
            size_t offset;
            if (source_index >= source_size)
            {
                return 0;
            }
            offset = source[source_index++];
            if (offset == 0 || offset > destination_index)
            {
                return 0;
            }
            copy_index = destination_index - offset;
        }
        else
        {
            size_t offset;
            if (source_index + 2U > source_size)
            {
                return 0;
            }
            offset = WG_ReadLE16(source + source_index);
            source_index += 2;
            if (offset >= destination_index)
            {
                return 0;
            }
            copy_index = offset;
        }

        while (count-- != 0)
        {
            destination[destination_index++] = destination[copy_index++];
        }
    }
    return 1;
}

int WG_RLEWExpand(const uint16_t *source, size_t source_words,
                  uint16_t *destination, size_t destination_words,
                  uint16_t tag)
{
    size_t source_index = 0;
    size_t destination_index = 0;

    if (source == NULL || destination == NULL)
    {
        return 0;
    }

    while (destination_index < destination_words)
    {
        uint16_t value;

        if (source_index >= source_words)
        {
            return 0;
        }
        value = source[source_index++];
        if (value != tag)
        {
            destination[destination_index++] = value;
            continue;
        }

        if (source_index + 2U > source_words)
        {
            return 0;
        }
        {
            size_t count = source[source_index++];
            value = source[source_index++];
            if (count > destination_words - destination_index)
            {
                return 0;
            }
            while (count-- != 0)
            {
                destination[destination_index++] = value;
            }
        }
    }
    return 1;
}
