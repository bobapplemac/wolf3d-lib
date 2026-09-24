#include "wolf3dgeneric.h"

#include <stdlib.h>
#include <string.h>

#include "wg_platform.h"

uint8_t *WG_ScreenBuffer;
uint8_t WG_Palette[WG_PALETTE_COLORS * 3];

static int wg_initialized;

wg_result_t wolf3dgeneric_Create(int argc, char **argv)
{
    size_t framebuffer_size;

    (void)argc;
    (void)argv;

    if (wg_initialized)
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
    return WG_RESULT_OK;
}

wg_result_t wolf3dgeneric_Run(void)
{
    wg_event_t event;

    if (!wg_initialized)
    {
        return WG_RESULT_INVALID_ARGUMENT;
    }

    while (WG_PollEvent(&event))
    {
        if (event.type == WG_EVENT_QUIT)
        {
            return WG_RESULT_QUIT;
        }
    }

    WG_Present(WG_ScreenBuffer, WG_Palette);
    return WG_RESULT_NOT_IMPLEMENTED;
}

void wolf3dgeneric_Shutdown(void)
{
    if (!wg_initialized)
    {
        return;
    }

    WG_Shutdown();
    free(WG_ScreenBuffer);
    WG_ScreenBuffer = NULL;
    wg_initialized = 0;
}

