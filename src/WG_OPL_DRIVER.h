#ifndef WG_OPL_DRIVER_H
#define WG_OPL_DRIVER_H

#include <stddef.h>
#include <stdint.h>

typedef struct wg_opl_driver
{
    const char *name;
    uint32_t capabilities;
    void *(*create)(uint32_t sample_rate);
    void (*destroy)(void *state);
    void (*write)(void *state, uint16_t register_number, uint8_t value);
    void (*write_buffered)(void *state, uint16_t register_number,
                           uint8_t value);
    void (*generate)(void *state, int16_t *stereo, size_t frame_count);
} wg_opl_driver_t;

#if defined(WG_OPL_ENABLE_NUKED)
const wg_opl_driver_t *WG_OPL_NukedDriver(void);
#endif
#if defined(WG_OPL_ENABLE_DBOPL)
const wg_opl_driver_t *WG_OPL_DBOPLDriver(void);
#endif
#if defined(WG_OPL_ENABLE_SILENT)
const wg_opl_driver_t *WG_OPL_SilentDriver(void);
#endif
#if defined(WG_OPL_ENABLE_ADLIB)
const wg_opl_driver_t *WG_OPL_AdLibDriver(void);
#endif

#endif
