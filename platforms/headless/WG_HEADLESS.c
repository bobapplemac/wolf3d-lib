#include "WG_PLATFORM.h"

#include <stdio.h>

static uint32_t wg_headless_ticks;

int WG_Init(void)
{
    wg_headless_ticks = 0;
    return 1;
}

void WG_Shutdown(void)
{
}

void WG_Present(const uint8_t *pixels, const uint8_t *palette)
{
    (void)pixels;
    (void)palette;
}

uint32_t WG_GetTicksMs(void)
{
    return wg_headless_ticks;
}

void WG_SleepMs(uint32_t milliseconds)
{
    wg_headless_ticks += milliseconds;
}

int WG_PollEvent(wg_event_t *event)
{
    if (event != NULL)
    {
        event->type = WG_EVENT_NONE;
    }
    return 0;
}

int WG_IsInteractive(void)
{
    return 0;
}

void WG_SetWindowTitle(const char *title)
{
    (void)title;
}

void WG_ReportError(const char *message)
{
    fprintf(stderr, "wolf3dgeneric: %s\n", message);
}

int WG_PCMInit(uint32_t sample_rate, uint16_t channels)
{
    return sample_rate != 0U && channels != 0U;
}

void WG_PCMShutdown(void)
{
}

size_t WG_PCMWritableFrames(void)
{
    return 0U;
}

int WG_PCMSubmit(const int16_t *samples, size_t frame_count)
{
    return samples != NULL || frame_count == 0U;
}
