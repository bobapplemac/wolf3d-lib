#ifndef WG_RANDOM_H
#define WG_RANDOM_H

#include <stdint.h>

typedef struct wg_random
{
    uint8_t index;
} wg_random_t;

void WG_RandomSeed(wg_random_t *random, uint8_t index);
uint8_t WG_RandomNext(wg_random_t *random);

#endif
