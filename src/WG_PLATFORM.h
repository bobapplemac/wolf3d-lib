#ifndef WG_PLATFORM_H
#define WG_PLATFORM_H

#include "WOLF3D.h"

int WG_Init(void);
void WG_Shutdown(void);
void WG_Present(const uint8_t *pixels, const uint8_t *palette);
uint32_t WG_GetTicksMs(void);
void WG_SleepMs(uint32_t milliseconds);
int WG_PollEvent(wolf3d_event_t *event);
int WG_IsInteractive(void);
void WG_SetWindowTitle(const char *title);
void WG_PrintMessage(const char *message);
void WG_ReportError(const char *message);
void WG_PresentText(const uint8_t *cells, uint16_t columns, uint16_t rows);
int WG_PCMInit(uint32_t requested_rate, uint16_t channels,
               uint32_t *obtained_rate);
void WG_PCMShutdown(void);
size_t WG_PCMWritableFrames(void);
int WG_PCMSubmit(const int16_t *samples, size_t frame_count);
int WG_OPLHardwareInit(void);
void WG_OPLHardwareShutdown(void);
void WG_OPLHardwareWrite(uint16_t register_number, uint8_t value);

#endif
