#include "WOLF3DGENERIC.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "WG_DATA.h"
#include "WG_FIXED.h"
#include "WG_GRAPHICS.h"
#include "WL_AGENT.h"
#include "WL_GAME.h"
#include "WG_MAPS.h"
#include "WG_PALETTE.h"
#include "ID_PM.h"
#include "WG_PLATFORM.h"
#include "WG_RENDERER.h"
#include "WL_DRAW.h"
#include "WL_MAIN.h"

uint8_t *WG_ScreenBuffer;
uint8_t WG_Palette[WG_PALETTE_COLORS * 3];

static int wg_initialized;
static int wg_data_loaded;
static wg_data_set_t wg_data_set;

static const char *WG_FindDataPath(int argc, char **argv)
{
    int index;

    for (index = 1; index < argc; ++index)
    {
        if (strcmp(argv[index], "--data") == 0)
        {
            return index + 1 < argc ? argv[index + 1] : NULL;
        }
    }
    return NULL;
}

static int WG_HasArgument(int argc, char **argv, const char *argument)
{
    int index;

    for (index = 1; index < argc; ++index)
    {
        if (strcmp(argv[index], argument) == 0)
        {
            return 1;
        }
    }
    return 0;
}

static int WG_LoadTitleScreen(const char *data_path)
{
    wg_graphics_t graphics;
    char title[160];
    int title_length;

    if (!WG_DataOpen(&wg_data_set, data_path))
    {
        WG_ReportError("The selected directory is not a supported Wolf3D v1.4 data set.");
        return 0;
    }
    if (!WG_GraphicsOpen(&graphics, &wg_data_set))
    {
        WG_ReportError("Unable to open the Wolf3D graphics resources.");
        WG_DataClose(&wg_data_set);
        return 0;
    }
    if (!WG_GraphicsDecodeTitle(&graphics, wg_data_set.variant, WG_ScreenBuffer))
    {
        WG_ReportError("Unable to decode the Wolf3D title screen.");
        WG_GraphicsClose(&graphics);
        WG_DataClose(&wg_data_set);
        return 0;
    }
    WG_GraphicsClose(&graphics);

    memcpy(WG_Palette, WG_WolfPalette, sizeof(WG_Palette));
    title_length = snprintf(title, sizeof(title), "wolf3dgeneric - %s",
                            WG_DataVariantName(wg_data_set.variant));
    if (title_length >= 0 && (size_t)title_length < sizeof(title))
    {
        WG_SetWindowTitle(title);
    }
    wg_data_loaded = 1;
    return 1;
}

static int WG_LoadInitialPlayView(int open_doors, int guard_view)
{
    wg_maps_t maps;
    wg_map_t map;
    wg_level_t level;
    wg_pages_t pages;
    wg_wall_cache_t walls;
    wg_view_tables_t view;
    wg_graphics_t graphics;
    wl_status_t status;
    wg_wall_hit_t hits[WG_MAX_VIEW_WIDTH];
    uint8_t visible_tiles[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    int success = 0;

    memset(&maps, 0, sizeof(maps));
    memset(&map, 0, sizeof(map));
    memset(&pages, 0, sizeof(pages));
    memset(&walls, 0, sizeof(walls));
    memset(&view, 0, sizeof(view));
    memset(&graphics, 0, sizeof(graphics));
    if (!WG_MapsOpen(&maps, &wg_data_set)
        || !WG_MapsLoad(&maps, 0, &map)
        || !WG_LevelBuild(&map, &level)
        || !WG_PagesOpen(&pages, &wg_data_set)
        || !WG_WallCacheLoad(&walls, &pages)
        || !WG_GraphicsOpen(&graphics, &wg_data_set))
    {
        goto cleanup;
    }
    if (open_doors)
    {
        uint8_t door;

        for (door = 0; door < level.door_count; ++door)
        {
            level.doors[door].position = 0xffffU;
        }
    }
    if (guard_view)
    {
        const wg_actor_t *actor;
        size_t player_tile;

        if (level.actor_count == 0U
            || level.actors[level.actor_count - 1U].tile_x < 3U)
        {
            goto cleanup;
        }
        actor = &level.actors[level.actor_count - 1U];
        level.player_x = actor->x - 3 * WG_FIXED_ONE;
        level.player_y = actor->y;
        level.player_angle = 0U;
        player_tile = (size_t)actor->tile_y * WG_LEVEL_SIZE
                      + actor->tile_x - 3U;
        if (level.tiles[player_tile] != 0U)
        {
            goto cleanup;
        }
    }
    WG_ViewBuildTrigTables(&view);
    if (!WG_ViewCalculateProjection(&view, WG_MAX_VIEW_WIDTH,
                                    WG_FOCAL_LENGTH)
        || !WG_RenderStaticView(WG_ScreenBuffer, &level, &view, &walls,
                                0, 0, level.player_x, level.player_y,
                                level.player_angle, hits, visible_tiles)
        || !WL_DrawScaleds(WG_ScreenBuffer, &pages, &level, &view, hits,
                           visible_tiles,
                           level.player_x, level.player_y,
                           level.player_angle)
        || !WL_DrawPlayerWeapon(WG_ScreenBuffer, &pages, 1, 0))
    {
        goto cleanup;
    }
    WL_StatusDefaults(&status);
    if (!WL_DrawStatusBar(WG_ScreenBuffer, &graphics, &status))
    {
        goto cleanup;
    }
    success = 1;

cleanup:
    WG_GraphicsClose(&graphics);
    WG_WallCacheFree(&walls);
    WG_PagesClose(&pages);
    WG_MapFree(&map);
    WG_MapsClose(&maps);
    if (!success)
    {
        WG_ReportError("Unable to render the initial Wolf3D play view.");
    }
    return success;
}

wg_result_t wolf3dgeneric_Create(int argc, char **argv)
{
    size_t framebuffer_size;
    const char *data_path;

    if (wg_initialized || argc < 0 || (argc > 0 && argv == NULL))
    {
        return WG_RESULT_INVALID_ARGUMENT;
    }

    framebuffer_size = (size_t)WG_SCREEN_WIDTH * (size_t)WG_SCREEN_HEIGHT;
    WG_ScreenBuffer = (uint8_t *)calloc(framebuffer_size, sizeof(*WG_ScreenBuffer));
    if (WG_ScreenBuffer == NULL)
    {
        WG_ReportError("Unable to allocate the Wolf3D framebuffer.");
        return WG_RESULT_PLATFORM_ERROR;
    }

    memset(WG_Palette, 0, sizeof(WG_Palette));
    if (!WG_Init())
    {
        free(WG_ScreenBuffer);
        WG_ScreenBuffer = NULL;
        return WG_RESULT_PLATFORM_ERROR;
    }

    WG_SetWindowTitle("wolf3dgeneric bootstrap");
    wg_initialized = 1;
    data_path = WG_FindDataPath(argc, argv);
    if (data_path != NULL && !WG_LoadTitleScreen(data_path))
    {
        wolf3dgeneric_Shutdown();
        return WG_RESULT_PLATFORM_ERROR;
    }
    if (data_path != NULL && WG_HasArgument(argc, argv, "--play-view")
        && !WG_LoadInitialPlayView(
            WG_HasArgument(argc, argv, "--open-doors"),
            WG_HasArgument(argc, argv, "--guard-view")))
    {
        wolf3dgeneric_Shutdown();
        return WG_RESULT_PLATFORM_ERROR;
    }
    return WG_RESULT_OK;
}

wg_result_t wolf3dgeneric_Run(void)
{
    wg_event_t event;

    if (!wg_initialized)
    {
        return WG_RESULT_INVALID_ARGUMENT;
    }

    WG_Present(WG_ScreenBuffer, WG_Palette);
    if (!wg_data_loaded || !WG_IsInteractive())
    {
        return WG_RESULT_NOT_IMPLEMENTED;
    }

    for (;;)
    {
        while (WG_PollEvent(&event))
        {
            if (event.type == WG_EVENT_QUIT)
            {
                return WG_RESULT_QUIT;
            }
        }
        WG_Present(WG_ScreenBuffer, WG_Palette);
        WG_SleepMs(10);
    }
}

void wolf3dgeneric_Shutdown(void)
{
    if (!wg_initialized)
    {
        return;
    }

    WG_Shutdown();
    WG_DataClose(&wg_data_set);
    free(WG_ScreenBuffer);
    WG_ScreenBuffer = NULL;
    wg_data_loaded = 0;
    wg_initialized = 0;
}
