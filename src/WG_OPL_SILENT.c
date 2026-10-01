#include "WG_OPL.h"

#include <stdlib.h>
#include <string.h>

struct wg_opl
{
    uint8_t registers[512];
};

wg_opl_t *WG_OPL_Create(uint32_t sample_rate)
{
    if (sample_rate == 0U)
    {
        return NULL;
    }
    return (wg_opl_t *)calloc(1U, sizeof(wg_opl_t));
}

void WG_OPL_Destroy(wg_opl_t *opl)
{
    free(opl);
}

void WG_OPL_WriteRegister(wg_opl_t *opl, uint16_t register_number,
                          uint8_t value)
{
    if (opl != NULL && register_number < 512U)
    {
        opl->registers[register_number] = value;
    }
}

void WG_OPL_WriteRegisterBuffered(wg_opl_t *opl,
                                  uint16_t register_number,
                                  uint8_t value)
{
    WG_OPL_WriteRegister(opl, register_number, value);
}

void WG_OPL_Generate(wg_opl_t *opl, int16_t *stereo, size_t frame_count)
{
    (void)opl;
    if (stereo != NULL)
    {
        memset(stereo, 0, frame_count * 2U * sizeof(*stereo));
    }
}
