#ifndef WG_OPL_H
#define WG_OPL_H

#include <stddef.h>
#include <stdint.h>

typedef struct wg_opl wg_opl_t;

#define WG_OPL_CAP_PCM_RENDER 0x01U
#define WG_OPL_CAP_SILENT 0x02U
#define WG_OPL_CAP_HARDWARE 0x04U

size_t WG_OPL_DriverCount(void);
const char *WG_OPL_DriverName(size_t index);
uint32_t WG_OPL_DriverCapabilities(size_t index);
int WG_OPL_SelectDriver(const char *name);
const char *WG_OPL_SelectedDriver(void);
uint32_t WG_OPL_SelectedCapabilities(void);

wg_opl_t *WG_OPL_Create(uint32_t sample_rate);
void WG_OPL_Destroy(wg_opl_t *opl);
void WG_OPL_WriteRegister(wg_opl_t *opl, uint16_t register_number,
                          uint8_t value);
void WG_OPL_WriteRegisterBuffered(wg_opl_t *opl,
                                  uint16_t register_number,
                                  uint8_t value);
void WG_OPL_Generate(wg_opl_t *opl, int16_t *stereo, size_t frame_count);

#endif
