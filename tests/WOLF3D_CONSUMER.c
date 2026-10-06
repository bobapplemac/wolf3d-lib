#include "WOLF3D.h"

int main(void)
{
    size_t count = wolf3d_GetOPLDriverCount();

    if (wolf3d_SetPlatform(NULL) != WOLF3D_RESULT_INVALID_ARGUMENT
        || count == 0U
        || wolf3d_GetOPLDriverName(0U) == NULL
        || wolf3d_GetOPLDriverName(count) != NULL
        || wolf3d_GetOPLDriverCapabilities(count) != 0U
        || wolf3d_GetSelectedOPLDriver() == NULL)
    {
        return 1;
    }
    return 0;
}
