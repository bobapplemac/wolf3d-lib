#include "WG_OPL.h"
#include "WG_OPL_DRIVER.h"

#include <stdlib.h>

#include "opl3.h"

typedef struct wg_opl_nuked
{
    opl3_chip chip;
} wg_opl_nuked_t;

static void *WG_OPL_NukedCreate(uint32_t sample_rate)
{
    wg_opl_nuked_t *opl = (wg_opl_nuked_t *)calloc(1U, sizeof(*opl));

    if (opl != NULL)
    {
        OPL3_Reset(&opl->chip, sample_rate);
    }
    return opl;
}

static void WG_OPL_NukedDestroy(void *state)
{
    free(state);
}

static void WG_OPL_NukedWrite(void *state, uint16_t register_number,
                              uint8_t value)
{
    wg_opl_nuked_t *opl = (wg_opl_nuked_t *)state;
    if (opl != NULL)
    {
        OPL3_WriteReg(&opl->chip, register_number, value);
    }
}

static void WG_OPL_NukedWriteBuffered(void *state,
                                      uint16_t register_number,
                                      uint8_t value)
{
    wg_opl_nuked_t *opl = (wg_opl_nuked_t *)state;
    if (opl != NULL)
    {
        OPL3_WriteRegBuffered(&opl->chip, register_number, value);
    }
}

static void WG_OPL_NukedGenerate(void *state, int16_t *stereo,
                                 size_t frame_count)
{
    wg_opl_nuked_t *opl = (wg_opl_nuked_t *)state;
    if (opl != NULL && stereo != NULL)
    {
        OPL3_GenerateStream(&opl->chip, stereo, (uint32_t)frame_count);
    }
}

const wg_opl_driver_t *WG_OPL_NukedDriver(void)
{
    static const wg_opl_driver_t driver =
    {
        "nuked", WG_OPL_CAP_PCM_RENDER,
        WG_OPL_NukedCreate, WG_OPL_NukedDestroy,
        WG_OPL_NukedWrite, WG_OPL_NukedWriteBuffered,
        WG_OPL_NukedGenerate
    };
    return &driver;
}
