#include "wolf3dgeneric.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    wg_result_t result;
    int bootstrap_test;

    bootstrap_test = argc == 2 && strcmp(argv[1], "--bootstrap-test") == 0;
    result = wolf3dgeneric_Create(argc, argv);
    if (result != WG_RESULT_OK)
    {
        fprintf(stderr, "Initialization failed with result %d.\n", (int)result);
        return 1;
    }

    result = wolf3dgeneric_Run();
    wolf3dgeneric_Shutdown();

    if (bootstrap_test)
    {
        if (result != WG_RESULT_NOT_IMPLEMENTED)
        {
            fprintf(stderr, "Unexpected bootstrap result %d.\n", (int)result);
            return 1;
        }
        return 0;
    }

    fprintf(stderr, "The engine is not playable yet (bootstrap result %d).\n",
            (int)result);
    return result == WG_RESULT_NOT_IMPLEMENTED ? 0 : 1;
}

