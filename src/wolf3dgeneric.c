#include "wolf3dgeneric.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wg_data.h"
#include "wg_graphics.h"
#include "wg_palette.h"
#include "wg_platform.h"

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
