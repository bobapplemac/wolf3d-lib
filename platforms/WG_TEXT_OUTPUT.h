#ifndef WG_TEXT_OUTPUT_H
#define WG_TEXT_OUTPUT_H

#include <stdio.h>
#include <stdint.h>

void WG_WriteTextScreen(FILE *stream, const uint8_t *cells,
                        uint16_t columns, uint16_t rows, int color);
int WG_TextOutputSupportsColor(FILE *stream);
uint16_t WG_TextScreenContentRows(const uint8_t *cells,
                                  uint16_t columns, uint16_t rows);
#ifdef _WIN32
void WG_WriteWindowsTextScreen(const uint8_t *cells,
                               uint16_t columns, uint16_t rows);
#endif

#endif
