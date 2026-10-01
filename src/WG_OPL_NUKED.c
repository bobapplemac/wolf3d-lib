#include "WG_OPL.h"

#include <stdlib.h>

#include "opl3.h"

struct wg_opl
{
    opl3_chip chip;
};

wg_opl_t *WG_OPL_Create(uint32_t sample_rate)
{
    wg_opl_t *opl = (wg_opl_t *)calloc(1U, sizeof(*opl));

    if (opl != NULL)
    {
        OPL3_Reset(&opl->chip, sample_rate);
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
        OPL3_WriteReg(&opl->chip, register_number, value);
    }
}

void WG_OPL_WriteRegisterBuffered(wg_opl_t *opl,
                                  uint16_t register_number,
                                  uint8_t value)
{
    if (opl != NULL)
    {
        OPL3_WriteRegBuffered(&opl->chip, register_number, value);
    }
}

void WG_OPL_Generate(wg_opl_t *opl, int16_t *stereo, size_t frame_count)
{
    if (opl != NULL && stereo != NULL)
    {
        OPL3_GenerateStream(&opl->chip, stereo, (uint32_t)frame_count);
    }
}
