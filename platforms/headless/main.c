#include "wolf3dgeneric.h"

#include <stdio.h>
#include <string.h>

static const char *WG_DumpPath(int argc, char **argv)
{
    int index;

    for (index = 1; index + 1 < argc; ++index)
    {
        if (strcmp(argv[index], "--dump-frame") == 0)
        {
            return argv[index + 1];
        }
    }
    return NULL;
}

static int WG_WriteFrame(const char *path)
{
    FILE *stream;
    size_t index;

#ifdef _MSC_VER
    stream = NULL;
    if (fopen_s(&stream, path, "wb") != 0)
    {
        return 0;
    }
#else
    stream = fopen(path, "wb");
    if (stream == NULL)
    {
        return 0;
    }
#endif
    if (fprintf(stream, "P6\n%d %d\n255\n", WG_SCREEN_WIDTH,
                WG_SCREEN_HEIGHT) < 0)
    {
        fclose(stream);
        return 0;
    }
    for (index = 0; index < WG_SCREEN_WIDTH * WG_SCREEN_HEIGHT; ++index)
    {
        size_t color = (size_t)WG_ScreenBuffer[index] * 3U;
        if (fwrite(WG_Palette + color, 1, 3, stream) != 3U)
        {
            fclose(stream);
            return 0;
        }
    }
    return fclose(stream) == 0;
}

int main(int argc, char **argv)
{
    wg_result_t result;
    int bootstrap_test;
    const char *dump_path;

    bootstrap_test = argc == 2 && strcmp(argv[1], "--bootstrap-test") == 0;
    dump_path = WG_DumpPath(argc, argv);
    result = wolf3dgeneric_Create(argc, argv);
    if (result != WG_RESULT_OK)
    {
        fprintf(stderr, "Initialization failed with result %d.\n", (int)result);
        return 1;
    }

    result = wolf3dgeneric_Run();
    if (dump_path != NULL && !WG_WriteFrame(dump_path))
    {
        fprintf(stderr, "Unable to write frame dump: %s\n", dump_path);
        wolf3dgeneric_Shutdown();
        return 1;
    }
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

    if (dump_path == NULL)
    {
        fprintf(stderr, "The engine is not playable yet (bootstrap result %d).\n",
                (int)result);
    }
    return result == WG_RESULT_NOT_IMPLEMENTED ? 0 : 1;
}
