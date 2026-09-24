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
void WG_ReportError(const char *message);

#endif
