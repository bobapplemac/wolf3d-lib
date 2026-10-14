/* Exercise the actual frame-update body and modal guards with synthetic state;
   no game assets, window system or sound hardware are needed. */
#include <assert.h>
#include "../src/WOLF3D.c"
#include "../src/WG_PLATFORM.c"
static uint32_t test_ticks;
static unsigned presentations;
static uint32_t TestTicks(void) { return test_ticks; }
static void TestPresent(const uint8_t *pixels, const uint8_t *palette)
{
    (void)pixels; (void)palette; ++presentations;
    /* Host callbacks must not recursively advance the simulation. */
    assert(wolf3d_PumpFrame(1) == WOLF3D_RESULT_OK);
}
int main(void)
{
    uint8_t framebuffer[WOLF3D_SCREEN_WIDTH * WOLF3D_SCREEN_HEIGHT];
    uint8_t blank[sizeof(framebuffer)];
    memset(framebuffer, 0, sizeof(framebuffer));
    memset(blank, 0, sizeof(blank));
    wolf3d_ScreenBuffer = framebuffer;
    wg_platform.get_ticks_ms = TestTicks;
    wg_platform.present = TestPresent;
    wg_game.active = 1;
    wg_game.preload_tics = 100;
    wg_game.preload_fill_tics = 10;
    wg_run_result = WOLF3D_RESULT_OK;
    test_ticks = 100;
    assert(wolf3d_PumpFrame(0) == WOLF3D_RESULT_OK);
    assert(wg_game.preload_fill_tics == 10); /* Outside Run: no-op. */
    wg_run_active = 1;
    assert(wolf3d_PumpFrame(0) == WOLF3D_RESULT_OK);
    assert(wg_game.preload_fill_tics == 10); /* Not inside poll_event. */
    wg_run_polling = 1;
    assert(wolf3d_PumpFrame(0) == WOLF3D_RESULT_OK);
    assert(wg_game.preload_fill_tics == 3); /* Seven 70-Hz simulation ticks. */
    assert(memcmp(blank, framebuffer, sizeof(blank)) != 0); /* Memory rendering. */
    assert(!presentations);
    assert(wolf3d_PumpFrame(1) == WOLF3D_RESULT_OK && presentations == 1);
    assert(wg_game.preload_fill_tics == 3); /* No repeated elapsed time. */
    wg_game.paused = 1; test_ticks += 1000;
    assert(wolf3d_PumpFrame(0) == WOLF3D_RESULT_OK);
    assert(wg_game.preload_fill_tics == 3 && wg_run_accumulator == 0);
    wg_game.paused = 0; test_ticks += 15;
    assert(wolf3d_PumpFrame(0) == WOLF3D_RESULT_OK);
    assert(wg_game.preload_fill_tics == 2); /* No pause backlog. */
    wg_run_polling = 0;
    assert(WG_RunFrameGuarded(1) == WOLF3D_RESULT_OK && presentations == 2);
    assert(wg_game.preload_fill_tics == 2); /* Resume shares the modal clock. */
    wg_run_result = WOLF3D_RESULT_PLATFORM_ERROR;
    wg_run_polling = 1;
    assert(wolf3d_PumpFrame(0) == WOLF3D_RESULT_PLATFORM_ERROR);
    puts("Modal engine: simulation, framebuffer, presentation, pause, reentry and shared-clock checks passed.");
    return 0;
}
