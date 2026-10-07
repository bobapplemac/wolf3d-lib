#include "WG_PLATFORM.h"

#include <string.h>

static wolf3d_platform_api_t wg_platform;

wolf3d_result_t wolf3d_SetPlatform(const wolf3d_platform_api_t *platform)
{
    if (platform == NULL
        || platform->api_version != WOLF3D_PLATFORM_API_VERSION
        || platform->struct_size < sizeof(*platform)
        || platform->init == NULL
        || platform->shutdown == NULL
        || platform->present == NULL
        || platform->get_ticks_ms == NULL
        || platform->sleep_ms == NULL
        || platform->poll_event == NULL
        || platform->is_interactive == NULL
        || platform->set_window_title == NULL
        || platform->print_message == NULL
        || platform->report_error == NULL
        || platform->present_text == NULL
        || platform->pcm_init == NULL
        || platform->pcm_shutdown == NULL
        || platform->pcm_writable_frames == NULL
        || platform->pcm_submit == NULL)
    {
        return WOLF3D_RESULT_INVALID_ARGUMENT;
    }
    memcpy(&wg_platform, platform, sizeof(wg_platform));
    return WOLF3D_RESULT_OK;
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

int WG_PollEvent(wolf3d_event_t *event)
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

void WG_PrintMessage(const char *message)
{
    if (wg_platform.print_message != NULL)
    {
        wg_platform.print_message(message);
    }
}

void WG_PresentText(const uint8_t *cells, uint16_t columns, uint16_t rows)
{
    if (wg_platform.present_text != NULL)
    {
        wg_platform.present_text(cells, columns, rows);
    }
}

int WG_PCMInit(uint32_t requested_rate, uint16_t channels,
               uint32_t *obtained_rate)
{
    wolf3d_pcm_format_t requested;
    wolf3d_pcm_format_t obtained;

    if (obtained_rate == NULL)
    {
        return 0;
    }
    requested.sample_rate = requested_rate;
    requested.channels = channels;
    requested.bits_per_sample = 16U;
    obtained = requested;
    if (wg_platform.pcm_init_ex != NULL)
    {
        if (!wg_platform.pcm_init_ex(&requested, &obtained)
            || obtained.sample_rate == 0U
            || obtained.channels != channels
            || obtained.bits_per_sample != 16U)
        {
            return 0;
        }
        *obtained_rate = obtained.sample_rate;
        return 1;
    }
    if (wg_platform.pcm_init != NULL
        && wg_platform.pcm_init(requested_rate, channels))
    {
        *obtained_rate = requested_rate;
        return 1;
    }
    return 0;
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

int WG_OPLHardwareInit(void)
{
    return wg_platform.opl_hardware_init != NULL
               && wg_platform.opl_hardware_init();
}

void WG_OPLHardwareShutdown(void)
{
    if (wg_platform.opl_hardware_shutdown != NULL)
    {
        wg_platform.opl_hardware_shutdown();
    }
}

void WG_OPLHardwareWrite(uint16_t register_number, uint8_t value)
{
    if (wg_platform.opl_hardware_write != NULL)
    {
        wg_platform.opl_hardware_write(register_number, value);
    }
}
