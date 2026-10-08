#include "WG_OPL.h"
#include "WG_OPL_DRIVER.h"

#include <limits.h>
#include <stdlib.h>

#include "dbopl.h"

#define WG_OPL_DBOPL_BLOCK 256U

typedef struct wg_opl_dbopl
{
    Chip chip;
} wg_opl_dbopl_t;

static int16_t WG_OPL_DBOPLClamp(int32_t sample)
{
    if (sample > INT16_MAX)
    {
        return INT16_MAX;
    }
    if (sample < INT16_MIN)
    {
        return INT16_MIN;
    }
    return (int16_t)sample;
}

static void *WG_OPL_DBOPLCreate(uint32_t sample_rate)
{
    wg_opl_dbopl_t *opl;

    if (sample_rate == 0U)
    {
        return NULL;
    }
    opl = (wg_opl_dbopl_t *)calloc(1U, sizeof(*opl));
    if (opl != NULL)
    {
        DBOPL_InitTables();
        Chip__Chip(&opl->chip);
        Chip__Setup(&opl->chip, sample_rate);
    }
    return opl;
}

static void WG_OPL_DBOPLDestroy(void *state)
{
    free(state);
}

static void WG_OPL_DBOPLWrite(void *state, uint16_t register_number,
                              uint8_t value)
{
    wg_opl_dbopl_t *opl = (wg_opl_dbopl_t *)state;
    if (opl != NULL)
    {
        Chip__WriteReg(&opl->chip, register_number, value);
    }
}

static void WG_OPL_DBOPLWriteBuffered(void *state,
                                      uint16_t register_number,
                                      uint8_t value)
{
    /* DBOPL has no delayed-write queue. Its host-facing register contract is
       otherwise equivalent, so writes take effect at the current boundary. */
    WG_OPL_DBOPLWrite(state, register_number, value);
}

static void WG_OPL_DBOPLGenerate(void *state, int16_t *stereo,
                                 size_t frame_count)
{
    wg_opl_dbopl_t *opl = (wg_opl_dbopl_t *)state;
    int32_t mono[WG_OPL_DBOPL_BLOCK];

    if (opl == NULL || stereo == NULL)
    {
        return;
    }
    while (frame_count != 0U)
    {
        size_t index;
        size_t frames = frame_count < WG_OPL_DBOPL_BLOCK
                            ? frame_count : WG_OPL_DBOPL_BLOCK;

        Chip__GenerateBlock2(&opl->chip, (Bitu)frames, mono);
        for (index = 0U; index < frames; ++index)
        {
            int16_t sample = WG_OPL_DBOPLClamp(mono[index]);

            stereo[index * 2U] = sample;
            stereo[index * 2U + 1U] = sample;
        }
        stereo += frames * 2U;
        frame_count -= frames;
    }
}

const wg_opl_driver_t *WG_OPL_DBOPLDriver(void)
{
    static const wg_opl_driver_t driver =
    {
        "dbopl", WG_OPL_CAP_PCM_RENDER,
        WG_OPL_DBOPLCreate, WG_OPL_DBOPLDestroy,
        WG_OPL_DBOPLWrite, WG_OPL_DBOPLWriteBuffered,
        WG_OPL_DBOPLGenerate
    };
    return &driver;
}
