#include "WOLF3D.h"

int main(void)
{
    return wolf3d_SetPlatform(NULL) == WOLF3D_RESULT_INVALID_ARGUMENT ? 0 : 1;
}
