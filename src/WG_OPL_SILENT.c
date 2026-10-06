#include "WG_OPL.h"
#include "WG_OPL_DRIVER.h"

#include <stdlib.h>
#include <string.h>

typedef struct wg_opl_silent
{
    uint8_t registers[512];
} wg_opl_silent_t;

static void *WG_OPL_SilentCreate(uint32_t sample_rate)
{
    if (sample_rate == 0U)
    {
        return NULL;
    }
    return calloc(1U, sizeof(wg_opl_silent_t));
}

static void WG_OPL_SilentDestroy(void *state)
{
    free(state);
}

static void WG_OPL_SilentWrite(void *state, uint16_t register_number,
                               uint8_t value)
{
    wg_opl_silent_t *opl = (wg_opl_silent_t *)state;
    if (opl != NULL && register_number < 512U)
    {
        opl->registers[register_number] = value;
    }
}

static void WG_OPL_SilentWriteBuffered(void *state,
                                       uint16_t register_number,
                                       uint8_t value)
{
    WG_OPL_SilentWrite(state, register_number, value);
}

static void WG_OPL_SilentGenerate(void *state, int16_t *stereo,
                                  size_t frame_count)
{
    (void)state;
    if (stereo != NULL)
    {
        memset(stereo, 0, frame_count * 2U * sizeof(*stereo));
    }
}

const wg_opl_driver_t *WG_OPL_SilentDriver(void)
{
    static const wg_opl_driver_t driver =
    {
        "silent", WG_OPL_CAP_SILENT,
        WG_OPL_SilentCreate, WG_OPL_SilentDestroy,
        WG_OPL_SilentWrite, WG_OPL_SilentWriteBuffered,
        WG_OPL_SilentGenerate
    };
    return &driver;
}
