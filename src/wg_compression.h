#ifndef WG_COMPRESSION_H
#define WG_COMPRESSION_H

#include <stddef.h>
#include <stdint.h>

typedef struct wg_huffman_node
{
    uint16_t bit0;
    uint16_t bit1;
} wg_huffman_node_t;

int WG_HuffmanExpand(const uint8_t *source, size_t source_size,
                     uint8_t *destination, size_t destination_size,
                     const wg_huffman_node_t nodes[255]);

int WG_CarmackExpand(const uint8_t *source, size_t source_size,
                     uint16_t *destination, size_t destination_words);

int WG_RLEWExpand(const uint16_t *source, size_t source_words,
                  uint16_t *destination, size_t destination_words,
                  uint16_t tag);

#endif

