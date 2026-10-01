#include "WG_PLATFORM.h"
#include "../WG_HOST.h"
#include "../WG_TEXT_OUTPUT.h"

#include <stdio.h>

static uint32_t wg_headless_ticks;

static int WG_HeadlessInit(void)
{
    wg_headless_ticks = 0;
    return 1;
}

static void WG_HeadlessShutdown(void)
{
}

static void WG_HeadlessPresent(const uint8_t *pixels, const uint8_t *palette)
{
    (void)pixels;
    (void)palette;
}

static uint32_t WG_HeadlessGetTicksMs(void)
{
    return wg_headless_ticks;
}

static void WG_HeadlessSleepMs(uint32_t milliseconds)
{
    wg_headless_ticks += milliseconds;
}

static int WG_HeadlessPollEvent(wolf3d_event_t *event)
{
    if (event != NULL)
    {
        event->type = WOLF3D_EVENT_NONE;
    }
    return 0;
}

static int WG_HeadlessIsInteractive(void)
{
    return 0;
}

static void WG_HeadlessSetWindowTitle(const char *title)
{
    (void)title;
}

static void WG_HeadlessReportError(const char *message)
{
    fprintf(stderr, "wolf3dgeneric: %s\n", message);
}

static void WG_HeadlessPrintMessage(const char *message)
{
    fprintf(stdout, "%s\n", message);
    fflush(stdout);
}

static void WG_HeadlessPresentText(const uint8_t *cells, uint16_t columns,
                                   uint16_t rows)
{
    WG_WriteTextScreen(stdout, cells, columns, rows,
                       WG_TextOutputSupportsColor(stdout));
}

static int WG_HeadlessPCMInit(uint32_t sample_rate, uint16_t channels)
{
    return sample_rate != 0U && channels != 0U;
}

static void WG_HeadlessPCMShutdown(void)
{
}

static size_t WG_HeadlessPCMWritableFrames(void)
{
    return 0U;
}

static int WG_HeadlessPCMSubmit(const int16_t *samples, size_t frame_count)
{
    return samples != NULL || frame_count == 0U;
}

int WG_InstallPlatform(void)
{
    static const wolf3d_platform_api_t platform =
    {
        WOLF3D_PLATFORM_API_VERSION,
        sizeof(wolf3d_platform_api_t),
        WG_HeadlessInit,
        WG_HeadlessShutdown,
        WG_HeadlessPresent,
        WG_HeadlessGetTicksMs,
        WG_HeadlessSleepMs,
        WG_HeadlessPollEvent,
        WG_HeadlessIsInteractive,
        WG_HeadlessSetWindowTitle,
        WG_HeadlessPrintMessage,
        WG_HeadlessReportError,
        WG_HeadlessPresentText,
        WG_HeadlessPCMInit,
        WG_HeadlessPCMShutdown,
        WG_HeadlessPCMWritableFrames,
        WG_HeadlessPCMSubmit
    };

    return wolf3d_SetPlatform(&platform) == WOLF3D_RESULT_OK;
}
