#include "WG_FIXED.h"

int32_t WG_FixedByFrac(int32_t a, int32_t b)
{
    int negative = 0;
    uint64_t magnitude_a;
    uint64_t magnitude_b;
    uint64_t result;

    /* FixedByFrac in WL_DRAW.C treats the second operand as a signed
       magnitude 16-bit fraction.  The canonical portable table includes
       exact +/-65536 cardinal endpoints, while the Borland table values
       consumed by this routine stopped at a magnitude of 65535. */
    if (b == WG_FIXED_ONE)
    {
        b = WG_FIXED_ONE - 1;
    }
    else if (b == -WG_FIXED_ONE)
    {
        b = WG_FIXED_ONE - 1;
        negative = 1;
    }
    else if (b < 0)
    {
        b = -b;
        negative = 1;
    }
    if (a < 0)
    {
        magnitude_a = (uint64_t)(-(int64_t)a);
        negative = !negative;
    }
    else
    {
        magnitude_a = (uint64_t)a;
    }
    magnitude_b = (uint64_t)b;
    result = (magnitude_a * magnitude_b) >> 16U;
    return negative ? -(int32_t)result : (int32_t)result;
}
