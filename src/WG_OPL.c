#include "WG_OPL.h"

#include <stdlib.h>
#include <string.h>

#include "WG_OPL_DRIVER.h"

#ifndef WG_DEFAULT_OPL_DRIVER
#define WG_DEFAULT_OPL_DRIVER "nuked"
#endif

struct wg_opl
{
    const wg_opl_driver_t *driver;
    void *state;
};

static const wg_opl_driver_t *wg_selected_driver;

static const wg_opl_driver_t *WG_OPL_DriverAt(size_t index)
{
#if defined(WG_OPL_ENABLE_NUKED)
    if (index-- == 0U) return WG_OPL_NukedDriver();
#endif
#if defined(WG_OPL_ENABLE_DBOPL)
    if (index-- == 0U) return WG_OPL_DBOPLDriver();
#endif
#if defined(WG_OPL_ENABLE_SILENT)
    if (index-- == 0U) return WG_OPL_SilentDriver();
#endif
    (void)index;
    return NULL;
}

size_t WG_OPL_DriverCount(void)
{
    size_t count = 0U;
#if defined(WG_OPL_ENABLE_NUKED)
    ++count;
#endif
#if defined(WG_OPL_ENABLE_DBOPL)
    ++count;
#endif
#if defined(WG_OPL_ENABLE_SILENT)
    ++count;
#endif
    return count;
}

const char *WG_OPL_DriverName(size_t index)
{
    const wg_opl_driver_t *driver = WG_OPL_DriverAt(index);
    return driver != NULL ? driver->name : NULL;
}

uint32_t WG_OPL_DriverCapabilities(size_t index)
{
    const wg_opl_driver_t *driver = WG_OPL_DriverAt(index);
    return driver != NULL ? driver->capabilities : 0U;
}

static const wg_opl_driver_t *WG_OPL_FindDriver(const char *name)
{
    size_t index;
    for (index = 0U; index < WG_OPL_DriverCount(); ++index)
    {
        const wg_opl_driver_t *driver = WG_OPL_DriverAt(index);
        if (driver != NULL && strcmp(driver->name, name) == 0)
        {
            return driver;
        }
    }
    return NULL;
}

static const wg_opl_driver_t *WG_OPL_DefaultDriver(void)
{
    const wg_opl_driver_t *driver = WG_OPL_FindDriver(WG_DEFAULT_OPL_DRIVER);
    return driver != NULL ? driver : WG_OPL_DriverAt(0U);
}

int WG_OPL_SelectDriver(const char *name)
{
    if (name == NULL || strcmp(name, "auto") == 0)
    {
        wg_selected_driver = WG_OPL_DefaultDriver();
        return wg_selected_driver != NULL;
    }
    wg_selected_driver = WG_OPL_FindDriver(name);
    return wg_selected_driver != NULL;
}

const char *WG_OPL_SelectedDriver(void)
{
    const wg_opl_driver_t *driver = wg_selected_driver != NULL
                                        ? wg_selected_driver
                                        : WG_OPL_DefaultDriver();
    return driver != NULL ? driver->name : NULL;
}

uint32_t WG_OPL_SelectedCapabilities(void)
{
    const wg_opl_driver_t *driver = wg_selected_driver != NULL
                                        ? wg_selected_driver
                                        : WG_OPL_DefaultDriver();
    return driver != NULL ? driver->capabilities : 0U;
}

wg_opl_t *WG_OPL_Create(uint32_t sample_rate)
{
    const wg_opl_driver_t *driver = wg_selected_driver != NULL
                                        ? wg_selected_driver
                                        : WG_OPL_DefaultDriver();
    wg_opl_t *opl;
    if (driver == NULL || sample_rate == 0U) return NULL;
    opl = (wg_opl_t *)calloc(1U, sizeof(*opl));
    if (opl == NULL) return NULL;
    opl->state = driver->create(sample_rate);
    if (opl->state == NULL)
    {
        free(opl);
        return NULL;
    }
    opl->driver = driver;
    return opl;
}

void WG_OPL_Destroy(wg_opl_t *opl)
{
    if (opl != NULL)
    {
        opl->driver->destroy(opl->state);
        free(opl);
    }
}

void WG_OPL_WriteRegister(wg_opl_t *opl, uint16_t register_number,
                          uint8_t value)
{
    if (opl != NULL) opl->driver->write(opl->state, register_number, value);
}

void WG_OPL_WriteRegisterBuffered(wg_opl_t *opl, uint16_t register_number,
                                  uint8_t value)
{
    if (opl != NULL)
    {
        opl->driver->write_buffered(opl->state, register_number, value);
    }
}

void WG_OPL_Generate(wg_opl_t *opl, int16_t *stereo, size_t frame_count)
{
    if (opl != NULL && stereo != NULL)
    {
        opl->driver->generate(opl->state, stereo, frame_count);
    }
}
