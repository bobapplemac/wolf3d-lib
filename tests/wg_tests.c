#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wg_audio.h"
#include "wg_compression.h"
#include "wg_data.h"
#include "wg_font.h"
#include "wg_graphics.h"
#include "wg_maps.h"
#include "wg_pages.h"
#include "wg_palette.h"
#include "wg_video.h"

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

static void TestVideo(void)
{
    uint8_t source[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT];
    uint8_t destination[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT];
    uint8_t small[] = { 1, 2, 3, 4, 5, 6 };
    uint8_t black[256 * 3];
    uint8_t faded[256 * 3];
    uint8_t converted[256 * 3];
    wg_fizzle_t fizzle;
    size_t index;
    unsigned iterations = 0;

    WG_VideoClear(destination, 7);
    CHECK(destination[0] == 7);
    CHECK(destination[sizeof(destination) - 1U] == 7);
    WG_VideoBar(destination, -2, -1, 4, 3, 9);
    CHECK(destination[0] == 9);
    CHECK(destination[1] == 9);
    CHECK(destination[WG_VIDEO_WIDTH] == 9);
    CHECK(destination[WG_VIDEO_WIDTH + 1U] == 9);
    WG_VideoPlot(destination, WG_VIDEO_WIDTH, 0, 3);
    CHECK(destination[WG_VIDEO_WIDTH - 1U] == 7);

    WG_VideoClear(destination, 0);
    WG_VideoBlit(destination, -1, 1, small, 3, 2);
    CHECK(destination[WG_VIDEO_WIDTH] == 2);
    CHECK(destination[WG_VIDEO_WIDTH + 1U] == 3);
    CHECK(destination[WG_VIDEO_WIDTH * 2U] == 5);
    CHECK(destination[WG_VIDEO_WIDTH * 2U + 1U] == 6);

    memset(black, 0, sizeof(black));
    WG_PaletteFade(black, WG_WolfPaletteVGA, 15, 30, faded);
    CHECK(faded[5] == 21U);
    WG_PaletteFade(black, WG_WolfPaletteVGA, 30, 30, faded);
    CHECK(memcmp(faded, WG_WolfPaletteVGA, sizeof(faded)) == 0);
    WG_PaletteFromVGA(WG_WolfPaletteVGA, converted);
    CHECK(memcmp(converted, WG_WolfPalette, sizeof(converted)) == 0);

    for (index = 0; index < sizeof(source); ++index)
    {
        source[index] = (uint8_t)(index * 37U + 11U);
    }
    memset(destination, 0, sizeof(destination));
    WG_FizzleStart(&fizzle);
    while (!WG_FizzleStep(&fizzle, source, destination,
                           WG_VIDEO_WIDTH, WG_VIDEO_HEIGHT, 1000U))
    {
        CHECK(++iterations < 1000U);
    }
    CHECK(memcmp(source, destination, sizeof(source)) == 0);
}

static void TestDataSet(const char *path, wg_game_variant_t expected_variant,
                        size_t expected_graphics_offsets)
{
    wg_data_set_t data_set;
    wg_graphics_t graphics;
    wg_font_t font;
    wg_audio_t audio;
    wg_pages_t pages;
    wg_maps_t maps;
    wg_map_t map;
    const uint8_t *page_data;
    size_t page_size;
    uint8_t framebuffer[320 * 200];
    uint8_t *picture_pixels;
    uint16_t picture_width;
    uint16_t picture_height;
    uint64_t frame_hash = 1469598103934665603ULL;
    uint64_t map_hash = 1469598103934665603ULL;
    size_t index;
    size_t decoded_graphics = 0;
    size_t loaded_maps = 0;
    size_t present_pages = 0;

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

    CHECK(WG_GraphicsOpen(&graphics, &data_set));
    if (graphics.offsets != NULL)
    {
        CHECK(graphics.picture_count == (expected_variant
              == WG_GAME_WOLF3D_SHAREWARE_14 ? 144U : 132U));
        CHECK(WG_GraphicsDecodeTitle(&graphics, data_set.variant, framebuffer));
        for (index = 0; index < sizeof(framebuffer); ++index)
        {
            frame_hash ^= framebuffer[index];
            frame_hash *= 1099511628211ULL;
        }
        printf("%s title framebuffer FNV-1a: %016llx\n",
               WG_DataVariantName(data_set.variant),
               (unsigned long long)frame_hash);
        CHECK(frame_hash == 0x01e337d015f1541cULL);
        CHECK(WG_WolfPalette[0] == 0);
        CHECK(WG_WolfPalette[1] == 0);
        CHECK(WG_WolfPalette[2] == 0);
        CHECK(WG_WolfPalette[5] == 170);
        CHECK(WG_FontOpen(&font, &graphics, 0));
        if (font.data != NULL)
        {
            CHECK(font.height != 0U);
            CHECK(WG_FontMeasure(&font, "WOLF") != 0U);
            memset(framebuffer, 0, sizeof(framebuffer));
            WG_FontDraw(&font, framebuffer, 4, 4, "WOLF", 15);
            CHECK(memchr(framebuffer, 15, sizeof(framebuffer)) != NULL);
            WG_FontClose(&font);
        }
        CHECK(WG_GraphicsDecodePicture(&graphics,
              expected_variant == WG_GAME_WOLF3D_SHAREWARE_14 ? 98U : 86U,
              &picture_pixels, &picture_width, &picture_height));
        if (picture_pixels != NULL)
        {
            CHECK(picture_width == 320U);
            CHECK(picture_height == 40U);
            free(picture_pixels);
        }
        for (index = 0; index + 1U < graphics.offset_count; ++index)
        {
            uint8_t *chunk_data = NULL;
            size_t chunk_size;

            if (graphics.offsets[index] == 0x00ffffffU)
            {
                continue;
            }
            if ((expected_variant == WG_GAME_WOLF3D_SHAREWARE_14
                 && index == 147U)
                || (expected_variant == WG_GAME_WOLF3D_FULL_GT_14
                    && index == 135U))
            {
                continue;
            }
            if (!WG_GraphicsDecodeChunk(&graphics, index,
                                         &chunk_data, &chunk_size))
            {
                fprintf(stderr, "failed to decode graphics chunk %u\n",
                        (unsigned)index);
                CHECK(0);
            }
            if (chunk_data != NULL)
            {
                CHECK(chunk_size != 0U);
                ++decoded_graphics;
                free(chunk_data);
            }
        }
        CHECK(decoded_graphics != 0U);
        WG_GraphicsClose(&graphics);
    }

    CHECK(WG_PagesOpen(&pages, &data_set));
    if (pages.file.data != NULL)
    {
        CHECK(WG_PagesGet(&pages, 0, &page_data, &page_size));
        CHECK(page_size == 4096U);
        CHECK(WG_PagesGet(&pages, data_set.sprite_start,
                          &page_data, &page_size));
        CHECK(page_size != 0U);
        CHECK(!WG_PagesGet(&pages, data_set.page_count,
                           &page_data, &page_size));
        for (index = 0; index < data_set.page_count; ++index)
        {
            if (data_set.pages[index].offset == 0
                || data_set.pages[index].length == 0)
            {
                continue;
            }
            CHECK(WG_PagesGet(&pages, index, &page_data, &page_size));
            if (page_data != NULL)
            {
                CHECK(page_size == data_set.pages[index].length);
                ++present_pages;
            }
        }
        CHECK(present_pages != 0U);
        WG_PagesClose(&pages);
    }

    CHECK(WG_MapsOpen(&maps, &data_set));
    if (maps.header_offsets != NULL)
    {
        CHECK(maps.rlew_tag == data_set.rlew_tag);
        CHECK(WG_MapsLoad(&maps, 0, &map));
        if (map.planes[0] != NULL)
        {
            CHECK(map.width == 64U);
            CHECK(map.height == 64U);
            for (index = 0; index < (size_t)map.width * map.height; ++index)
            {
                uint16_t value = map.planes[0][index];
                map_hash ^= value & 0xffU;
                map_hash *= 1099511628211ULL;
                map_hash ^= value >> 8;
                map_hash *= 1099511628211ULL;
            }
            printf("%s map 0 plane 0 FNV-1a: %016llx (%s)\n",
                   WG_DataVariantName(data_set.variant),
                   (unsigned long long)map_hash, map.name);
            CHECK(map_hash == 0x2f163ae2e768c7e8ULL);
            WG_MapFree(&map);
        }
        for (index = 0; index < maps.header_offset_count; ++index)
        {
            if (maps.header_offsets[index] == 0xffffffffU
                || maps.header_offsets[index] == 0U)
            {
                continue;
            }
            if (!WG_MapsLoad(&maps, index, &map))
            {
                fprintf(stderr, "failed to load map %u\n", (unsigned)index);
                CHECK(0);
            }
            if (map.planes[0] != NULL)
            {
                CHECK(map.width == 64U);
                CHECK(map.height == 64U);
                ++loaded_maps;
                WG_MapFree(&map);
            }
        }
        CHECK(loaded_maps == (expected_variant
              == WG_GAME_WOLF3D_SHAREWARE_14 ? 10U : 60U));
        WG_MapsClose(&maps);
    }

    CHECK(WG_AudioOpen(&audio, &data_set));
    if (audio.offsets != NULL)
    {
        CHECK(audio.offset_count == 289U);
        CHECK(WG_AudioGetChunk(&audio, 261U, &page_data, &page_size));
        CHECK(page_size > 2U);
        CHECK(!WG_AudioGetChunk(&audio, 288U, &page_data, &page_size));
        for (index = 0; index + 1U < audio.offset_count; ++index)
        {
            CHECK(WG_AudioGetChunk(&audio, index, &page_data, &page_size));
        }
        WG_AudioClose(&audio);
    }
    WG_DataClose(&data_set);
}

int main(int argc, char **argv)
{
    TestHuffman();
    TestCarmack();
    TestRLEW();
    TestMalformedCompression();
    TestVideo();

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
