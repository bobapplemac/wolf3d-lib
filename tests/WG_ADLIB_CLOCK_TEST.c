#include <assert.h>
#include <stdio.h>
#include "../src/WG_OPL_ADLIB.c"
static unsigned initialized, stopped, written;
static uint64_t observed_frame;
static uint32_t observed_rate;
int WG_OPLHardwareInit(void) { ++initialized; return 1; }
void WG_OPLHardwareShutdown(void) { ++stopped; }
void WG_OPLHardwareWriteTimed(uint16_t reg, uint8_t value, uint64_t frame, uint32_t rate)
{
    assert(reg == 0x20 && value == 1);
    ++written; observed_frame = frame; observed_rate = rate;
}
int main(void)
{
    const wg_opl_driver_t *driver = WG_OPL_AdLibDriver();
    int16_t pcm[1024];
    void *state = driver->create(11025);
    assert(state && initialized == 1);
    driver->write(state, 0x20, 1); assert(observed_frame == 0 && observed_rate == 11025);
    driver->generate(state, pcm, 15); driver->write_buffered(state, 0x20, 1);
    assert(observed_frame == 15 && pcm[0] == 0 && pcm[29] == 0);
    driver->generate(state, pcm, 16); driver->write(state, 0x20, 1);
    assert(observed_frame == 31);
    ((wg_adlib_clock_t *)state)->frame = 0xffffffffULL;
    driver->generate(state, pcm, 2); driver->write(state, 0x20, 1);
    assert(observed_frame == 0x100000001ULL);
    driver->write(state, 0x100, 1); assert(written == 4);
    driver->destroy(state); assert(stopped == 1);
    state = driver->create(48000); driver->write(state, 0x20, 1);
    assert(observed_frame == 0 && observed_rate == 48000);
    driver->destroy(state);
    puts("Native OPL adapter: frame timestamps, rate, 64-bit clock and restart passed.");
    return 0;
}
