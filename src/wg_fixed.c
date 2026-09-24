#include "wg_fixed.h"

int32_t WG_FixedMul(int32_t a, int32_t b)
{
    return (int32_t)(((int64_t)a * (int64_t)b) / WG_FIXED_ONE);
}
