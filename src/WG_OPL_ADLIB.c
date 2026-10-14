#include "WG_OPL.h"
#include "WG_OPL_DRIVER.h"
#include "WG_PLATFORM.h"

#include <stdlib.h>
#include <string.h>

typedef struct wg_adlib_clock
{
    uint64_t frame;
    uint32_t sample_rate;
} wg_adlib_clock_t;

static void *WG_OPL_AdLibCreate(uint32_t sample_rate)
{
    wg_adlib_clock_t *state;

    if (sample_rate == 0U || !WG_OPLHardwareInit())
    {
        return NULL;
    }
    state = (wg_adlib_clock_t *)calloc(1U, sizeof(*state));
    if (state == NULL)
    {
        WG_OPLHardwareShutdown();
    }
    if (state != NULL) state->sample_rate = sample_rate;
    return state;
}

static void WG_OPL_AdLibDestroy(void *state)
{
    if (state != NULL)
    {
        WG_OPLHardwareShutdown();
        free(state);
    }
}

static void WG_OPL_AdLibWrite(void *state, uint16_t register_number,
                              uint8_t value)
{
    if (state != NULL && register_number < 0x100U)
    {
        wg_adlib_clock_t *clock = (wg_adlib_clock_t *)state;
        WG_OPLHardwareWriteTimed(register_number, value,
                                  clock->frame, clock->sample_rate);
    }
}

static void WG_OPL_AdLibGenerate(void *state, int16_t *stereo,
                                 size_t frame_count)
{
    wg_adlib_clock_t *clock = (wg_adlib_clock_t *)state;
    if (stereo != NULL)
    {
        memset(stereo, 0, frame_count * 2U * sizeof(*stereo));
        if (clock != NULL) clock->frame += frame_count;
    }
}

const wg_opl_driver_t *WG_OPL_AdLibDriver(void)
{
    static const wg_opl_driver_t driver =
    {
        "adlib", WG_OPL_CAP_HARDWARE,
        WG_OPL_AdLibCreate, WG_OPL_AdLibDestroy,
        WG_OPL_AdLibWrite, WG_OPL_AdLibWrite,
        WG_OPL_AdLibGenerate
    };
    return &driver;
}
