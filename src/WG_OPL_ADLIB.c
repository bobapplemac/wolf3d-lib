#include "WG_OPL.h"
#include "WG_OPL_DRIVER.h"
#include "WG_PLATFORM.h"

#include <stdlib.h>
#include <string.h>

static void *WG_OPL_AdLibCreate(uint32_t sample_rate)
{
    uint8_t *state;

    if (sample_rate == 0U || !WG_OPLHardwareInit())
    {
        return NULL;
    }
    state = (uint8_t *)calloc(1U, 1U);
    if (state == NULL)
    {
        WG_OPLHardwareShutdown();
    }
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
        WG_OPLHardwareWrite(register_number, value);
    }
}

static void WG_OPL_AdLibGenerate(void *state, int16_t *stereo,
                                 size_t frame_count)
{
    (void)state;
    if (stereo != NULL)
    {
        memset(stereo, 0, frame_count * 2U * sizeof(*stereo));
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
