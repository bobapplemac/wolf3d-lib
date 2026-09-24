#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wg_compression.h"
#include "wg_data.h"

static int failures;

#define CHECK(expression)                                                       \
    do                                                                          \
    {                                                                           \
        if (!(expression))                                                      \
        {                                                                       \
            fprintf(stderr, "%s:%d: check failed: %s\n",                      \
                    __FILE__, __LINE__, #expression);                           \
            ++failures;                                                         \
        }                                                                       \
    } while (0)

static void TestHuffman(void)
{
    wg_huffman_node_t nodes[255];
    const uint8_t source[] = { 0x06 };
    uint8_t destination[4];

    memset(nodes, 0, sizeof(nodes));
    nodes[254].bit0 = 'A';
    nodes[254].bit1 = 'B';
    CHECK(WG_HuffmanExpand(source, sizeof(source), destination,
                           sizeof(destination), nodes));
    CHECK(memcmp(destination, "ABBA", sizeof(destination)) == 0);
}

static void TestCarmack(void)
{
    const uint8_t source[] =
    {
        0x11, 0x11, 0x22, 0x22,
        0x02, 0xa7, 0x02,
        0x00, 0xa7, 0x34
    };
    const uint16_t expected[] =
    {
        0x1111, 0x2222, 0x1111, 0x2222, 0xa734
    };
    uint16_t destination[5];

    CHECK(WG_CarmackExpand(source, sizeof(source), destination,
                           sizeof(destination) / sizeof(destination[0])));
    CHECK(memcmp(destination, expected, sizeof(expected)) == 0);
}

static void TestRLEW(void)
{
    const uint16_t source[] = { 1, 0xabcd, 3, 2, 3 };
    const uint16_t expected[] = { 1, 2, 2, 2, 3 };
    uint16_t destination[5];

    CHECK(WG_RLEWExpand(source, sizeof(source) / sizeof(source[0]),
                        destination,
                        sizeof(destination) / sizeof(destination[0]),
                        0xabcd));
    CHECK(memcmp(destination, expected, sizeof(expected)) == 0);
}

static void TestMalformedCompression(void)
{
    wg_huffman_node_t nodes[255];
    uint8_t byte_output[2];
    uint16_t word_output[4];
    const uint8_t bad_carmack[] = { 0x04, 0xa7, 0x01 };
    const uint16_t bad_rlew[] = { 0xabcd, 5, 1 };

    memset(nodes, 0, sizeof(nodes));
    nodes[254].bit0 = 510;
    nodes[254].bit1 = 510;
    CHECK(!WG_HuffmanExpand((const uint8_t *)"\0", 1, byte_output,
                            sizeof(byte_output), nodes));
    CHECK(!WG_CarmackExpand(bad_carmack, sizeof(bad_carmack), word_output,
                            sizeof(word_output) / sizeof(word_output[0])));
    CHECK(!WG_RLEWExpand(bad_rlew,
                         sizeof(bad_rlew) / sizeof(bad_rlew[0]), word_output,
                         sizeof(word_output) / sizeof(word_output[0]), 0xabcd));
}

static void TestDataSet(const char *path, wg_game_variant_t expected_variant,
                        size_t expected_graphics_offsets)
{
    wg_data_set_t data_set;

    CHECK(WG_DataOpen(&data_set, path));
    if (data_set.variant == WG_GAME_UNKNOWN)
    {
        return;
    }

    CHECK(data_set.variant == expected_variant);
    CHECK(data_set.page_count == 663);
    CHECK(data_set.sprite_start == 106);
    CHECK(data_set.sound_start == 542);
    CHECK(data_set.rlew_tag == 0xabcd);
    CHECK(data_set.graphics_offset_count == expected_graphics_offsets);
    CHECK(data_set.audio_offset_count == 289);
    CHECK(data_set.pages[0].offset == 4096);
    CHECK(data_set.pages[0].length != 0);
    WG_DataClose(&data_set);
}

int main(int argc, char **argv)
{
    TestHuffman();
    TestCarmack();
    TestRLEW();
    TestMalformedCompression();

    if (argc == 4 && strcmp(argv[1], "--data") == 0)
    {
        if (strcmp(argv[2], "wl1") == 0)
        {
            TestDataSet(argv[3], WG_GAME_WOLF3D_SHAREWARE_14, 157);
        }
        else if (strcmp(argv[2], "wl6") == 0)
        {
            TestDataSet(argv[3], WG_GAME_WOLF3D_FULL_GT_14, 150);
        }
        else
        {
            fprintf(stderr, "Unknown test data variant: %s\n", argv[2]);
            return 2;
        }
    }
    else if (argc != 1)
    {
        fprintf(stderr, "usage: wg-tests [--data wl1|wl6 PATH]\n");
        return 2;
    }

    if (failures != 0)
    {
        fprintf(stderr, "%d test check(s) failed.\n", failures);
        return 1;
    }
    return 0;
}

