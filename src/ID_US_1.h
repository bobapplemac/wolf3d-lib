#ifndef ID_US_1_H
#define ID_US_1_H

#include <stdint.h>

typedef struct wg_random
{
    uint8_t index;
} wg_random_t;

void WG_RandomSeed(wg_random_t *random, uint8_t index);
uint8_t WG_RandomNext(wg_random_t *random);
char ID_US_ScanToASCII(uint16_t scan_code, int shifted, int caps_lock);
const char *ID_US_ScanName(uint16_t scan_code);

#endif
