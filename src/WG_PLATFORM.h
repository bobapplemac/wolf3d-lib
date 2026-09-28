#ifndef WG_PLATFORM_H
#define WG_PLATFORM_H

#include "WOLF3DGENERIC.h"

int WG_Init(void);
void WG_Shutdown(void);
void WG_Present(const uint8_t *pixels, const uint8_t *palette);
uint32_t WG_GetTicksMs(void);
void WG_SleepMs(uint32_t milliseconds);
int WG_PollEvent(wg_event_t *event);
int WG_IsInteractive(void);
void WG_SetWindowTitle(const char *title);
void WG_PrintMessage(const char *message);
void WG_ReportError(const char *message);
void WG_PresentText(const uint8_t *cells, uint16_t columns, uint16_t rows);
int WG_PCMInit(uint32_t sample_rate, uint16_t channels);
void WG_PCMShutdown(void);
size_t WG_PCMWritableFrames(void);
int WG_PCMSubmit(const int16_t *samples, size_t frame_count);

#endif
