#include "WG_OPL.h"

#include <limits.h>
#include <stdlib.h>

#include "dbopl.h"

#define WG_OPL_DBOPL_BLOCK 256U

struct wg_opl
{
    Chip chip;
};

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

wg_opl_t *WG_OPL_Create(uint32_t sample_rate)
{
    wg_opl_t *opl;

    if (sample_rate == 0U)
    {
        return NULL;
    }
    opl = (wg_opl_t *)calloc(1U, sizeof(*opl));
    if (opl != NULL)
    {
        DBOPL_InitTables();
        Chip__Chip(&opl->chip);
        Chip__Setup(&opl->chip, sample_rate);
    }
    return opl;
}

void WG_OPL_Destroy(wg_opl_t *opl)
{
    free(opl);
}

void WG_OPL_WriteRegister(wg_opl_t *opl, uint16_t register_number,
                          uint8_t value)
{
    if (opl != NULL)
    {
        Chip__WriteReg(&opl->chip, register_number, value);
    }
}

void WG_OPL_WriteRegisterBuffered(wg_opl_t *opl,
                                  uint16_t register_number,
                                  uint8_t value)
{
    /* DBOPL has no delayed-write queue. Its host-facing register contract is
       otherwise equivalent, so writes take effect at the current boundary. */
    WG_OPL_WriteRegister(opl, register_number, value);
}

void WG_OPL_Generate(wg_opl_t *opl, int16_t *stereo, size_t frame_count)
{
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
