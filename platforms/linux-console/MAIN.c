#include "WOLF3D.h"
#include "../WG_HOST.h"
#include "WG_LINUX_CONSOLE.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void WG_PrintHelp(const char *program)
{
    printf("Usage: %s [Linux console options] [game options]\n\n",
           program != NULL ? program : "wolf3dgeneric-linux-console");
    printf("Linux console options:\n");
    printf("  --drm-device PATH    DRM card to use (default: first usable card)\n");
    printf("  --input-device PATH  evdev device to use (repeatable; default: auto)\n");
    printf("  --alsa-device NAME   ALSA PCM device (default: default)\n");
    printf("  --no-audio           Run without opening an ALSA device\n");
    printf("  --linux-console-help Show this help and exit\n\n");
    printf("Run from an active Linux virtual console with permission to access\n");
    printf("/dev/dri/card*, /dev/input/event*, and the selected ALSA device.\n");
}

int main(int argc, char **argv)
{
    char **game_argv;
    int game_argc = 0;
    int index;
    int success = 1;
    wolf3d_result_t result;

    game_argv = (char **)calloc((size_t)argc + 1U, sizeof(*game_argv));
    if (game_argv == NULL)
    {
        fprintf(stderr, "wolf3dgeneric: unable to allocate arguments.\n");
        return 1;
    }
    game_argv[game_argc++] = argv[0];
    for (index = 1; index < argc; ++index)
    {
        if (strcmp(argv[index], "--linux-console-help") == 0)
        {
            WG_PrintHelp(argv[0]);
            free(game_argv);
            return 0;
        }
        if (strcmp(argv[index], "--no-audio") == 0)
        {
            WG_LinuxConsoleDisableAudio();
            continue;
        }
        if (strcmp(argv[index], "--drm-device") == 0
            || strcmp(argv[index], "--input-device") == 0
            || strcmp(argv[index], "--alsa-device") == 0)
        {
            const char *option = argv[index];

            if (++index >= argc)
            {
                fprintf(stderr, "wolf3dgeneric: %s requires a value.\n",
                        option);
                success = 0;
                break;
            }
            if (strcmp(option, "--drm-device") == 0)
            {
                success = WG_LinuxConsoleSetDRMDevice(argv[index]);
            }
            else if (strcmp(option, "--input-device") == 0)
            {
                success = WG_LinuxConsoleAddInputDevice(argv[index]);
            }
            else
            {
                success = WG_LinuxConsoleSetALSADevice(argv[index]);
            }
            if (!success)
            {
                fprintf(stderr, "wolf3dgeneric: invalid value for %s.\n",
                        option);
                break;
            }
            continue;
        }
        game_argv[game_argc++] = argv[index];
    }

    if (!success || !WG_InstallPlatform())
    {
        if (success)
        {
            fprintf(stderr, "wolf3dgeneric: unable to install platform API.\n");
        }
        free(game_argv);
        return 1;
    }

    result = wolf3d_Create(game_argc, game_argv);
    if (result == WOLF3D_RESULT_OK)
    {
        result = wolf3d_Run();
        wolf3d_Shutdown();
    }
    free(game_argv);
    return result == WOLF3D_RESULT_QUIT || result == WOLF3D_RESULT_NOT_IMPLEMENTED
               ? 0 : 1;
}
