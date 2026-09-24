#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "WG_AUDIO.h"
#include "WG_ASSETS.h"
#include "ID_CA.h"
#include "WG_DATA.h"
#include "WG_FIXED.h"
#include "ID_VH.h"
#include "WG_GRAPHICS.h"
#include "WL_GAME.h"
#include "WL_AGENT.h"
#include "WG_MAPS.h"
#include "ID_PM.h"
#include "WG_PALETTE.h"
#include "ID_US_1.h"
#include "WL_DRAW.h"
#include "WG_RENDERER.h"
#include "WL_SCALE.h"
#include "ID_VL.h"
#include "WL_MAIN.h"

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

static void TestRandom(void)
{
    wg_random_t random;

    WG_RandomSeed(&random, 0);
    CHECK(WG_RandomNext(&random) == 8U);
    CHECK(WG_RandomNext(&random) == 109U);
    WG_RandomSeed(&random, 255U);
    CHECK(WG_RandomNext(&random) == 0U);
}

static void TestViewMath(void)
{
    wg_view_tables_t tables;
    const int32_t *cosine;

    memset(&tables, 0, sizeof(tables));
    CHECK(WG_FixedMul(WG_FIXED_ONE, WG_FIXED_ONE / 2) ==
          WG_FIXED_ONE / 2);
    CHECK(WG_FixedMul(-WG_FIXED_ONE, WG_FIXED_ONE / 2) ==
          -WG_FIXED_ONE / 2);
    WG_ViewBuildTrigTables(&tables);
    cosine = WG_ViewCosineTable(&tables);
    CHECK(cosine != NULL);
    CHECK(tables.sine[0] == 0);
    CHECK(tables.sine[WG_ANGLE_QUADRANT] == WG_FIXED_ONE);
    CHECK(tables.sine[2 * WG_ANGLE_QUADRANT] == 0);
    CHECK(tables.sine[3 * WG_ANGLE_QUADRANT] == -WG_FIXED_ONE);
    CHECK(cosine[0] == WG_FIXED_ONE);
    CHECK(cosine[WG_ANGLE_QUADRANT] == 0);
    CHECK(tables.fine_tangent[0] > 0);
    CHECK(tables.fine_tangent[WG_FINE_ANGLES / 4 - 1] > WG_FIXED_ONE);
    CHECK(WG_ViewCalculateProjection(&tables, WG_MAX_VIEW_WIDTH,
                                     WG_FOCAL_LENGTH));
    CHECK(tables.pixel_angle[159] == 0);
    CHECK(tables.pixel_angle[160] == 0);
    CHECK(tables.pixel_angle[0] == -tables.pixel_angle[319]);
    CHECK(tables.scale == 218);
    CHECK(tables.height_numerator == 223232);
    CHECK(tables.min_height_divisor == 7);
    CHECK(tables.max_slope > 0);
    CHECK(!WG_ViewCalculateProjection(&tables, 319, WG_FOCAL_LENGTH));
}

static void TestWallScaler(void)
{
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT];
    uint8_t texture[WG_TEXTURE_SIZE * WG_TEXTURE_SIZE];
    int x;
    int y;

    memset(framebuffer, 3, sizeof(framebuffer));
    for (y = 0; y < WG_TEXTURE_SIZE; ++y)
    {
        for (x = 0; x < WG_TEXTURE_SIZE; ++x)
        {
            texture[y * WG_TEXTURE_SIZE + x] = (uint8_t)y;
        }
    }
    CHECK(WG_ScaleWallPost(framebuffer, 0, 0, 320, 160, 10, 2,
                           texture, 7, 256));
    CHECK(framebuffer[47 * WG_VIDEO_WIDTH + 10] == 3);
    CHECK(framebuffer[48 * WG_VIDEO_WIDTH + 10] == 0);
    CHECK(framebuffer[48 * WG_VIDEO_WIDTH + 11] == 0);
    CHECK(framebuffer[111 * WG_VIDEO_WIDTH + 10] == 63);
    CHECK(framebuffer[112 * WG_VIDEO_WIDTH + 10] == 3);
    CHECK(!WG_ScaleWallPost(framebuffer, 0, 0, 320, 160, 319, 2,
                            texture, 0, 256));
}

static void TestStaticRaycaster(void)
{
    wg_level_t level;
    wg_view_tables_t tables;
    wg_wall_hit_t hits[WG_MAX_VIEW_WIDTH];
    int x;
    int y;

    memset(&level, 0, sizeof(level));
    memset(&tables, 0, sizeof(tables));
    for (x = 0; x < WG_LEVEL_SIZE; ++x)
    {
        level.tiles[x] = 1;
        level.tiles[(WG_LEVEL_SIZE - 1) * WG_LEVEL_SIZE + x] = 1;
    }
    for (y = 0; y < WG_LEVEL_SIZE; ++y)
    {
        level.tiles[y * WG_LEVEL_SIZE] = 1;
        level.tiles[y * WG_LEVEL_SIZE + WG_LEVEL_SIZE - 1] = 1;
    }
    WG_ViewBuildTrigTables(&tables);
    CHECK(WG_ViewCalculateProjection(&tables, WG_MAX_VIEW_WIDTH,
                                     WG_FOCAL_LENGTH));
    CHECK(WG_RaycastStaticWalls(&level, &tables,
                                32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                                32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                                0, hits));
    CHECK(hits[159].side == WG_WALL_VERTICAL);
    CHECK(hits[159].map_x == WG_LEVEL_SIZE - 1);
    CHECK(hits[159].map_y == 32);
    CHECK(hits[159].tile == 1);
    CHECK(hits[159].wall_page == 1);
    CHECK(hits[159].texture_column == 30);
    CHECK(hits[159].height == 28);
    CHECK(hits[160].x == hits[159].x);
    CHECK(hits[160].y == hits[159].y);
    CHECK(WG_RaycastStaticWalls(&level, &tables,
                                32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                                32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                                90, hits));
    CHECK(hits[159].side == WG_WALL_HORIZONTAL);
    CHECK(hits[159].map_y == 0);
    CHECK(WG_RaycastStaticWalls(&level, &tables,
                                32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                                32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                                180, hits));
    CHECK(hits[159].side == WG_WALL_VERTICAL);
    CHECK(hits[159].map_x == 0);
    CHECK(WG_RaycastStaticWalls(&level, &tables,
                                32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                                32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                                270, hits));
    CHECK(hits[159].side == WG_WALL_HORIZONTAL);
    CHECK(hits[159].map_y == WG_LEVEL_SIZE - 1);

    level.tiles[32 * WG_LEVEL_SIZE + 40] = 0x80U;
    level.door_count = 1;
    level.doors[0].tile_x = 40;
    level.doors[0].tile_y = 32;
    level.doors[0].vertical = 1;
    level.doors[0].lock = WG_DOOR_NORMAL;
    CHECK(WG_RaycastWalls(&level, &tables,
                          32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                          32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                          0, 10, hits));
    CHECK(hits[159].side == WG_WALL_DOOR);
    CHECK(hits[159].map_x == 40);
    CHECK(hits[159].wall_page == 11);
    level.doors[0].position = 0xffffU;
    CHECK(WG_RaycastWalls(&level, &tables,
                          32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                          32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                          0, 10, hits));
    CHECK(hits[159].side == WG_WALL_VERTICAL);
    CHECK(hits[159].map_x == WG_LEVEL_SIZE - 1);
}

static void TestStaticRenderer(void)
{
    wg_level_t level;
    wg_view_tables_t tables;
    wg_wall_cache_t walls;
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT];
    uint8_t wall_pixels[8 * WG_TEXTURE_SIZE * WG_TEXTURE_SIZE];
    wg_wall_hit_t hits[WG_MAX_VIEW_WIDTH];
    uint8_t visible_tiles[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    int x;
    int y;

    memset(&level, 0, sizeof(level));
    memset(&tables, 0, sizeof(tables));
    memset(framebuffer, 0, sizeof(framebuffer));
    memset(wall_pixels, 66, WG_TEXTURE_SIZE * WG_TEXTURE_SIZE);
    memset(wall_pixels + WG_TEXTURE_SIZE * WG_TEXTURE_SIZE, 77,
           WG_TEXTURE_SIZE * WG_TEXTURE_SIZE);
    walls.pixels = wall_pixels;
    walls.count = 8;
    for (x = 0; x < WG_LEVEL_SIZE; ++x)
    {
        level.tiles[x] = 1;
        level.tiles[(WG_LEVEL_SIZE - 1) * WG_LEVEL_SIZE + x] = 1;
    }
    for (y = 0; y < WG_LEVEL_SIZE; ++y)
    {
        level.tiles[y * WG_LEVEL_SIZE] = 1;
        level.tiles[y * WG_LEVEL_SIZE + WG_LEVEL_SIZE - 1] = 1;
    }
    WG_ViewBuildTrigTables(&tables);
    CHECK(WG_ViewCalculateProjection(&tables, WG_MAX_VIEW_WIDTH,
                                     WG_FOCAL_LENGTH));
    CHECK(WG_RenderStaticView(framebuffer, &level, &tables, &walls, 0, 0,
                              32 * WG_FIXED_ONE + WG_FIXED_ONE / 2,
                              32 * WG_FIXED_ONE + WG_FIXED_ONE / 2, 0, hits,
                              visible_tiles));
    CHECK(framebuffer[0] == 0x1d);
    CHECK(framebuffer[159 * WG_VIDEO_WIDTH] == 0x19);
    CHECK(framebuffer[80 * WG_VIDEO_WIDTH + 159] == 77);
    CHECK(visible_tiles[32 * WG_LEVEL_SIZE + 32] == 1U);
    CHECK(visible_tiles[32 * WG_LEVEL_SIZE + 40] == 1U);
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
    wg_level_t level;
    const uint8_t *page_data;
    size_t page_size;
    uint8_t framebuffer[320 * 200];
    uint8_t *picture_pixels;
    uint16_t picture_width;
    uint16_t picture_height;
    uint64_t frame_hash = 1469598103934665603ULL;
    uint64_t map_hash = 1469598103934665603ULL;
    uint64_t view_hash = 1469598103934665603ULL;
    uint64_t scenery_hash = 1469598103934665603ULL;
    uint64_t hud_hash = 1469598103934665603ULL;
    uint64_t open_view_hash = 1469598103934665603ULL;
    uint64_t guard_view_hash = 1469598103934665603ULL;
    size_t index;
    size_t decoded_graphics = 0;
    size_t loaded_maps = 0;
    size_t present_pages = 0;
    size_t decoded_sprites = 0;

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
        {
            uint8_t wall[WG_TEXTURE_SIZE * WG_TEXTURE_SIZE];
            wg_sprite_image_t sprite;
            wg_wall_cache_t wall_cache;

            CHECK(WG_DecodeWall(&pages, 0, wall));
            CHECK(WG_WallCacheLoad(&wall_cache, &pages));
            CHECK(wall_cache.count == data_set.sprite_start);
            WG_WallCacheFree(&wall_cache);
            for (index = 0;
                 index < (size_t)(data_set.sound_start - data_set.sprite_start);
                 ++index)
            {
                size_t sprite_page = (size_t)data_set.sprite_start + index;

                if (data_set.pages[sprite_page].offset == 0
                    || data_set.pages[sprite_page].length == 0)
                {
                    continue;
                }
                if (!WG_DecodeSprite(&pages, index, &sprite))
                {
                    fprintf(stderr, "failed to decode sprite %u\n",
                            (unsigned)index);
                    CHECK(0);
                }
                else
                {
                    ++decoded_sprites;
                }
            }
            CHECK(decoded_sprites == (expected_variant
                  == WG_GAME_WOLF3D_SHAREWARE_14 ? 226U : 436U));
        }
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
            CHECK(WG_LevelBuild(&map, &level));
            CHECK(level.player_tile_x == 29U);
            CHECK(level.player_tile_y == 57U);
            CHECK(level.player_x == 0x001d8000L);
            CHECK(level.player_y == 0x00398000L);
            CHECK(level.player_angle == 0U);
            CHECK(level.door_count > 0U);
            CHECK(level.static_count == 121U);
            CHECK(level.actor_count == 17U);
            CHECK(level.actors[16].tile_x == 39U);
            CHECK(level.actors[16].tile_y == 61U);
            CHECK(level.actors[16].direction == 4U);
            CHECK(level.actors[16].shape == 50U);
            printf("%s map 0 static objects: %u\n",
                   WG_DataVariantName(data_set.variant),
                   (unsigned)level.static_count);
            printf("%s map 0 initial guards: %u\n",
                   WG_DataVariantName(data_set.variant),
                   (unsigned)level.actor_count);
            {
                wg_view_tables_t view_tables;
                wg_wall_cache_t wall_cache;
                wl_status_t status;
                wg_wall_hit_t render_hits[WG_MAX_VIEW_WIDTH];
                uint8_t visible_tiles[WG_LEVEL_SIZE * WG_LEVEL_SIZE];

                memset(&view_tables, 0, sizeof(view_tables));
                memset(&wall_cache, 0, sizeof(wall_cache));
                WG_ViewBuildTrigTables(&view_tables);
                CHECK(WG_ViewCalculateProjection(&view_tables,
                                                 WG_MAX_VIEW_WIDTH,
                                                 WG_FOCAL_LENGTH));
                CHECK(WG_PagesOpen(&pages, &data_set));
                CHECK(WG_WallCacheLoad(&wall_cache, &pages));
                memset(framebuffer, 0, sizeof(framebuffer));
                CHECK(WG_RenderStaticView(framebuffer, &level, &view_tables,
                                          &wall_cache, 0, 0,
                                          level.player_x, level.player_y,
                                          level.player_angle, render_hits,
                                          visible_tiles));
                for (index = 0; index < sizeof(framebuffer); ++index)
                {
                    view_hash ^= framebuffer[index];
                    view_hash *= 1099511628211ULL;
                }
                printf("%s initial play view FNV-1a: %016llx\n",
                       WG_DataVariantName(data_set.variant),
                       (unsigned long long)view_hash);
                CHECK(view_hash == 0x52a9cf2dd9dcab66ULL);
                CHECK(WL_DrawScaleds(framebuffer, &pages, &level,
                                     &view_tables, render_hits, visible_tiles,
                                     level.player_x, level.player_y,
                                     level.player_angle));
                for (index = 0; index < sizeof(framebuffer); ++index)
                {
                    scenery_hash ^= framebuffer[index];
                    scenery_hash *= 1099511628211ULL;
                }
                printf("%s initial scenery view FNV-1a: %016llx\n",
                       WG_DataVariantName(data_set.variant),
                       (unsigned long long)scenery_hash);
                CHECK(scenery_hash == 0x52a9cf2dd9dcab66ULL);
                CHECK(WL_DrawPlayerWeapon(framebuffer, &pages, 1, 0));
                CHECK(WG_GraphicsOpen(&graphics, &data_set));
                WL_StatusDefaults(&status);
                CHECK(WL_DrawStatusBar(framebuffer, &graphics, &status));
                for (index = 0; index < sizeof(framebuffer); ++index)
                {
                    hud_hash ^= framebuffer[index];
                    hud_hash *= 1099511628211ULL;
                }
                printf("%s initial HUD view FNV-1a: %016llx\n",
                       WG_DataVariantName(data_set.variant),
                       (unsigned long long)hud_hash);
                CHECK(hud_hash == 0xab0c1a3f48fece62ULL);
                for (index = 0; index < level.door_count; ++index)
                {
                    level.doors[index].position = 0xffffU;
                }
                CHECK(WG_RenderStaticView(framebuffer, &level, &view_tables,
                                          &wall_cache, 0, 0,
                                          level.player_x, level.player_y,
                                          level.player_angle, render_hits,
                                          visible_tiles));
                CHECK(WL_DrawScaleds(framebuffer, &pages, &level,
                                     &view_tables, render_hits, visible_tiles,
                                     level.player_x, level.player_y,
                                     level.player_angle));
                CHECK(WL_DrawPlayerWeapon(framebuffer, &pages, 1, 0));
                CHECK(WL_DrawStatusBar(framebuffer, &graphics, &status));
                for (index = 0; index < sizeof(framebuffer); ++index)
                {
                    open_view_hash ^= framebuffer[index];
                    open_view_hash *= 1099511628211ULL;
                }
                printf("%s open-door scenery FNV-1a: %016llx\n",
                       WG_DataVariantName(data_set.variant),
                       (unsigned long long)open_view_hash);
                CHECK(open_view_hash == 0x800512fcf839700fULL);
                for (index = 0; index < level.door_count; ++index)
                {
                    level.doors[index].position = 0U;
                }
                CHECK(level.actor_count != 0U);
                CHECK(WG_RenderStaticView(
                    framebuffer, &level, &view_tables, &wall_cache, 0, 0,
                    level.actors[level.actor_count - 1U].x
                        - 3 * WG_FIXED_ONE,
                    level.actors[level.actor_count - 1U].y, 0,
                    render_hits, visible_tiles));
                CHECK(WL_DrawScaleds(
                    framebuffer, &pages, &level, &view_tables, render_hits,
                    visible_tiles,
                    level.actors[level.actor_count - 1U].x
                        - 3 * WG_FIXED_ONE,
                    level.actors[level.actor_count - 1U].y, 0));
                CHECK(WL_DrawPlayerWeapon(framebuffer, &pages, 1, 0));
                CHECK(WL_DrawStatusBar(framebuffer, &graphics, &status));
                for (index = 0; index < sizeof(framebuffer); ++index)
                {
                    guard_view_hash ^= framebuffer[index];
                    guard_view_hash *= 1099511628211ULL;
                }
                printf("%s guard view FNV-1a: %016llx\n",
                       WG_DataVariantName(data_set.variant),
                       (unsigned long long)guard_view_hash);
                CHECK(guard_view_hash == 0xa6db229142f7150bULL);
                WG_GraphicsClose(&graphics);
                WG_WallCacheFree(&wall_cache);
                WG_PagesClose(&pages);
            }
            {
                wg_level_t difficulty_level;

                CHECK(WG_LevelBuildForDifficulty(
                    &map, WG_DIFFICULTY_BABY, &difficulty_level));
                printf("%s baby guards: %u\n",
                       WG_DataVariantName(data_set.variant),
                       (unsigned)difficulty_level.actor_count);
                CHECK(difficulty_level.actor_count == 10U);
                CHECK(WG_LevelBuildForDifficulty(
                    &map, WG_DIFFICULTY_HARD, &difficulty_level));
                printf("%s hard guards: %u\n",
                       WG_DataVariantName(data_set.variant),
                       (unsigned)difficulty_level.actor_count);
                CHECK(difficulty_level.actor_count == 32U);
            }
            printf("%s map 0 player: (%u,%u) angle %u\n",
                   WG_DataVariantName(data_set.variant),
                   (unsigned)level.player_tile_x,
                   (unsigned)level.player_tile_y,
                   (unsigned)level.player_angle);
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
                CHECK(WG_LevelBuild(&map, &level));
                CHECK(level.door_count <= WG_MAX_DOORS);
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
    TestRandom();
    TestViewMath();
    TestWallScaler();
    TestStaticRaycaster();
    TestStaticRenderer();

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
