#include "WG_PLATFORM.h"

#include <string.h>

static wg_platform_api_t wg_platform;

wg_result_t wolf3dgeneric_SetPlatform(const wg_platform_api_t *platform)
{
    if (platform == NULL
        || platform->api_version != WG_PLATFORM_API_VERSION
        || platform->struct_size < sizeof(*platform)
        || platform->init == NULL
        || platform->shutdown == NULL
        || platform->present == NULL
        || platform->get_ticks_ms == NULL
        || platform->sleep_ms == NULL
        || platform->poll_event == NULL
        || platform->is_interactive == NULL
        || platform->set_window_title == NULL
        || platform->report_error == NULL
        || platform->pcm_init == NULL
        || platform->pcm_shutdown == NULL
        || platform->pcm_writable_frames == NULL
        || platform->pcm_submit == NULL)
    {
        return WG_RESULT_INVALID_ARGUMENT;
    }
    memcpy(&wg_platform, platform, sizeof(wg_platform));
    return WG_RESULT_OK;
}

int WG_Init(void)
{
    return wg_platform.init != NULL && wg_platform.init();
}

void WG_Shutdown(void)
{
    if (wg_platform.shutdown != NULL)
    {
        wg_platform.shutdown();
    }
}

void WG_Present(const uint8_t *pixels, const uint8_t *palette)
{
    if (wg_platform.present != NULL)
    {
        wg_platform.present(pixels, palette);
    }
}

uint32_t WG_GetTicksMs(void)
{
    return wg_platform.get_ticks_ms != NULL
               ? wg_platform.get_ticks_ms() : 0U;
}

void WG_SleepMs(uint32_t milliseconds)
{
    if (wg_platform.sleep_ms != NULL)
    {
        wg_platform.sleep_ms(milliseconds);
    }
}

int WG_PollEvent(wg_event_t *event)
{
    return wg_platform.poll_event != NULL
               && wg_platform.poll_event(event);
}

int WG_IsInteractive(void)
{
    return wg_platform.is_interactive != NULL
               && wg_platform.is_interactive();
}

void WG_SetWindowTitle(const char *title)
{
    if (wg_platform.set_window_title != NULL)
    {
        wg_platform.set_window_title(title);
    }
}

void WG_ReportError(const char *message)
{
    if (wg_platform.report_error != NULL)
    {
        wg_platform.report_error(message);
    }
}

int WG_PCMInit(uint32_t sample_rate, uint16_t channels)
{
    return wg_platform.pcm_init != NULL
               && wg_platform.pcm_init(sample_rate, channels);
}

void WG_PCMShutdown(void)
{
    if (wg_platform.pcm_shutdown != NULL)
    {
        wg_platform.pcm_shutdown();
    }
}

size_t WG_PCMWritableFrames(void)
{
    return wg_platform.pcm_writable_frames != NULL
               ? wg_platform.pcm_writable_frames() : 0U;
}

int WG_PCMSubmit(const int16_t *samples, size_t frame_count)
{
    return wg_platform.pcm_submit != NULL
               && wg_platform.pcm_submit(samples, frame_count);
}
