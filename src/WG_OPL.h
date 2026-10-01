#ifndef WG_OPL_H
#define WG_OPL_H

#include <stddef.h>
#include <stdint.h>

typedef struct wg_opl wg_opl_t;

wg_opl_t *WG_OPL_Create(uint32_t sample_rate);
void WG_OPL_Destroy(wg_opl_t *opl);
void WG_OPL_WriteRegister(wg_opl_t *opl, uint16_t register_number,
                          uint8_t value);
void WG_OPL_WriteRegisterBuffered(wg_opl_t *opl,
                                  uint16_t register_number,
                                  uint8_t value);
void WG_OPL_Generate(wg_opl_t *opl, int16_t *stereo, size_t frame_count);

#endif
