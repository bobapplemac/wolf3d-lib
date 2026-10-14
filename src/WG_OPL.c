#include "WG_OPL.h"

#include <stdlib.h>
#include <stdio.h>
#include "WG_PLATFORM.h"
#include <string.h>

#include "WG_OPL_DRIVER.h"

#ifndef WG_DEFAULT_OPL_DRIVER
#define WG_DEFAULT_OPL_DRIVER "auto"
#endif

struct wg_opl
{
    const wg_opl_driver_t *driver;
    void *state;
};

static const wg_opl_driver_t *wg_selected_driver;
static const char *wg_requested_driver = WG_DEFAULT_OPL_DRIVER;
static int wg_request_valid = 1;
static char wg_opl_error[256];
static char wg_opl_trace[2048];
#if defined(__DOS__) || defined(WG_AUDIO_LEGACY_TARGET) || (defined(_WIN32_WINNT) && _WIN32_WINNT <= 0x0501)
#define WG_LEGACY_AUDIO 1
#else
#define WG_LEGACY_AUDIO 0
#endif
void WG_OPL_SetError(const char *message)
{
    (void)snprintf(wg_opl_error, sizeof(wg_opl_error), "%s", message != NULL ? message : "");
}
const char *WG_OPL_Error(void) { return wg_opl_error; }
const char *WG_OPL_Trace(void) { return wg_opl_trace; }
static void WG_OPL_TraceLine(const char *name, const char *message)
{
    size_t used = strlen(wg_opl_trace);
    (void)snprintf(wg_opl_trace + used, sizeof(wg_opl_trace) - used,
                   "  %s: %s\n", name, message);
}
int WG_OPL_ParseRate(const char *text, uint32_t *rate)
{
    unsigned long value = 0;
    const char *p;
    if (text == NULL || strcmp(text, "auto") == 0) { *rate = 0; return 1; }
    if (*text == '\0') return 0;
    for (p = text; *p; ++p)
    {
        if (*p < '0' || *p > '9') return 0;
        value = value * 10UL + (unsigned)(*p - '0');
        if (value > 192000UL) return 0;
    }
    if (value < 8000UL) return 0;
    *rate = (uint32_t)value;
    return 1;
}


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
#if defined(WG_OPL_ENABLE_ADLIB)
    if (index-- == 0U) return WG_OPL_AdLibDriver();
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
#if defined(WG_OPL_ENABLE_ADLIB)
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

int WG_OPL_SelectDriver(const char *name)
{
    const wg_opl_driver_t *driver;
    wg_selected_driver = NULL;
    wg_opl_error[0] = wg_opl_trace[0] = '\0';
    if (name == NULL) name = WG_DEFAULT_OPL_DRIVER;
    if (strcmp(name, "auto") == 0)
    {
        wg_requested_driver = "auto"; wg_request_valid = 1; return 1;
    }
    driver = WG_OPL_FindDriver(name);
    wg_request_valid = driver != NULL;
    if (driver == NULL)
    {
        (void)snprintf(wg_opl_error, sizeof(wg_opl_error),
                       "OPL driver '%s' is not included in this build.", name);
        return 0;
    }
    wg_requested_driver = driver->name;
    return 1;
}
const char *WG_OPL_SelectedDriver(void)
{
    return wg_selected_driver != NULL ? wg_selected_driver->name : wg_requested_driver;
}
uint32_t WG_OPL_SelectedCapabilities(void)
{
    const wg_opl_driver_t *driver = wg_selected_driver;
    if (driver == NULL) driver = WG_OPL_FindDriver(wg_requested_driver);
    return driver != NULL ? driver->capabilities : 0U;
}
static uint32_t WG_OPL_AutoRate(const char *name)
{
    if (strcmp(name, "adlib") == 0) return 22050U;
    if (!WG_LEGACY_AUDIO) return 48000U;
    return strcmp(name, "nuked") == 0 ? 44100U : 22050U;
}
static wg_opl_t *WG_OPL_CreateInternal(uint32_t rate, int pcm, uint32_t *obtained)
{
    static const char *order[] = { "adlib",
#if WG_LEGACY_AUDIO
        "dbopl", "nuked",
#else
        "nuked", "dbopl",
#endif
        "silent" };
    unsigned index, count = strcmp(wg_requested_driver, "auto") == 0 ? 4U : 1U;
    wg_opl_t *opl;
    if (!wg_request_valid) return NULL;
    wg_opl_trace[0] = '\0';
    wg_selected_driver = NULL;
    opl = (wg_opl_t *)calloc(1U, sizeof(*opl));
    if (opl == NULL) { WG_OPL_SetError("Unable to allocate OPL state."); return NULL; }
    for (index = 0; index < count; ++index)
    {
        const char *name = count == 1U ? wg_requested_driver : order[index];
        const wg_opl_driver_t *driver = WG_OPL_FindDriver(name);
        uint32_t preferred = rate != 0 ? rate : WG_OPL_AutoRate(name);
        uint32_t actual = preferred;
        char detail[160];
        if (driver == NULL) { WG_OPL_TraceLine(name, "not compiled; skipped"); continue; }
        if (pcm && !WG_PCMInit(preferred, 2U, &actual))
        {
            WG_PCMShutdown();
            WG_OPL_SetError("Unable to initialize PCM output (not an OPL driver failure).");
            WG_OPL_TraceLine(name, wg_opl_error);
            break;
        }
        WG_OPL_SetError(strcmp(name, "adlib") == 0 ? "native OPL hardware not detected or inaccessible" : "driver initialization failed");
        opl->state = driver->create(actual);
        if (opl->state != NULL)
        {
            opl->driver = driver;
            wg_selected_driver = driver;
            *obtained = actual;
            (void)snprintf(detail, sizeof(detail), "selected; preferred %lu Hz, device %lu Hz",
                           (unsigned long)preferred, (unsigned long)actual);
            WG_OPL_TraceLine(name, detail);
            wg_opl_error[0] = '\0';
            return opl;
        }
        WG_OPL_TraceLine(name, wg_opl_error);
        if (pcm) WG_PCMShutdown();
    }
    if (count == 1U)
    {
        char reason[256];
        (void)snprintf(reason, sizeof(reason), "OPL driver '%s' could not start: %.170s",
                       wg_requested_driver, wg_opl_error);
        WG_OPL_SetError(reason);
    }
    free(opl);
    return NULL;
}
wg_opl_t *WG_OPL_Create(uint32_t rate)
{
    uint32_t obtained;
    return WG_OPL_CreateInternal(rate, 0, &obtained);
}
wg_opl_t *WG_OPL_CreateAudio(uint32_t rate, uint32_t *obtained)
{
    return WG_OPL_CreateInternal(rate, 1, obtained);
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
