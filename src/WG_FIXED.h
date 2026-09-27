#ifndef WG_FIXED_H
#define WG_FIXED_H

#include <stdint.h>

#define WG_FIXED_ONE INT32_C(65536)

int32_t WG_FixedByFrac(int32_t a, int32_t b);

#endif
