#include "WOLF3DGENERIC.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ID_SD.h"
#include "ID_PM.h"
#include "WG_AUDIO.h"
#include "WG_DATA.h"
#include "WG_ENDIAN.h"
#include "WL_PLAY.h"

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

static const char *WG_ArgumentValue(int argc, char **argv,
                                    const char *argument)
{
    int index;

    for (index = 1; index + 1 < argc; ++index)
    {
        if (strcmp(argv[index], argument) == 0)
        {
            return argv[index + 1];
        }
    }
    return NULL;
}

static void WG_WriteLE16(FILE *stream, uint16_t value)
{
    uint8_t bytes[2];

    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    (void)fwrite(bytes, 1U, sizeof(bytes), stream);
}

static void WG_WriteLE32(FILE *stream, uint32_t value)
{
    uint8_t bytes[4];

    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
    (void)fwrite(bytes, 1U, sizeof(bytes), stream);
}

static int WG_WriteMusic(const char *data_path,
                         wg_game_variant_t requested_variant,
                         wg_game_family_t preferred_family,
                         unsigned map_number,
                         int sound_number, int digitized, int pc_speaker,
                         unsigned left_position, unsigned right_position,
                         const char *output_path)
{
    enum { sample_rate = 48000, seconds = 10, block_frames = 1024 };
    wg_data_set_t data_set;
    wg_audio_t audio;
    wg_pages_t pages;
    id_sd_digi_bank_t digi_bank;
    id_sd_music_t *music = NULL;
    const uint8_t *chunk;
    size_t chunk_size;
    int16_t samples[block_frames * 2U];
    uint8_t pcm_bytes[block_frames * 4U];
    uint32_t frames_left = sample_rate * seconds;
    uint32_t data_bytes = frames_left * 2U * sizeof(int16_t);
    FILE *stream = NULL;
    int success = 0;

    memset(&data_set, 0, sizeof(data_set));
    memset(&audio, 0, sizeof(audio));
    memset(&pages, 0, sizeof(pages));
    memset(&digi_bank, 0, sizeof(digi_bank));
    if (!WG_DataOpenSelected(&data_set, data_path, requested_variant,
                             preferred_family)
        || !WG_AudioOpen(&audio, &data_set)
        || !WG_AudioGetChunk(&audio,
                             WL_MusicChunkForVariant(data_set.variant,
                                                     map_number),
                             &chunk, &chunk_size))
    {
        goto cleanup;
    }
    music = ID_SD_MusicCreate(sample_rate);
    if (music == NULL || !ID_SD_MusicStart(music, chunk, chunk_size))
    {
        goto cleanup;
    }
    if (sound_number >= 0)
    {
        int played = 0;

        if ((size_t)sound_number >= WG_DataSoundCount(data_set.variant)
            || !WG_AudioGetChunk(
                &audio,
                (pc_speaker && !digitized
                     ? 0U : WG_DataSoundCount(data_set.variant))
                    + (size_t)sound_number,
                                 &chunk, &chunk_size))
        {
            goto cleanup;
        }
        if (digitized)
        {
            int digital_number = ID_SD_DigitalNumberForSoundForVariant(
                data_set.variant, (unsigned)sound_number);
            uint8_t *digital_data = NULL;
            size_t digital_length = 0U;

            if (digital_number < 0 || chunk_size < 6U
                || !WG_PagesOpen(&pages, &data_set)
                || !ID_SD_DigiBankOpen(&digi_bank, &pages)
                || !ID_SD_DigiBankLoad(&digi_bank, (size_t)digital_number,
                                       &digital_data, &digital_length))
            {
                free(digital_data);
                goto cleanup;
            }
            played = ID_SD_DigitalStart(music, digital_data, digital_length,
                                        WG_ReadLE16(chunk + 4U),
                                        (uint8_t)left_position,
                                        (uint8_t)right_position);
            free(digital_data);
        }
        else if (pc_speaker)
        {
            played = ID_SD_PCStart(music, chunk, chunk_size);
        }
        else
        {
            played = ID_SD_EffectStart(music, chunk, chunk_size);
        }
        if (!played)
        {
            goto cleanup;
        }
    }
#ifdef _MSC_VER
    if (fopen_s(&stream, output_path, "wb") != 0)
    {
        stream = NULL;
    }
#else
    stream = fopen(output_path, "wb");
#endif
    if (stream == NULL)
    {
        goto cleanup;
    }
    (void)fwrite("RIFF", 1U, 4U, stream);
    WG_WriteLE32(stream, 36U + data_bytes);
    (void)fwrite("WAVEfmt ", 1U, 8U, stream);
    WG_WriteLE32(stream, 16U);
    WG_WriteLE16(stream, 1U);
    WG_WriteLE16(stream, 2U);
    WG_WriteLE32(stream, sample_rate);
    WG_WriteLE32(stream, sample_rate * 4U);
    WG_WriteLE16(stream, 4U);
    WG_WriteLE16(stream, 16U);
    (void)fwrite("data", 1U, 4U, stream);
    WG_WriteLE32(stream, data_bytes);
    while (frames_left != 0U)
    {
        uint32_t frames = frames_left < block_frames
                              ? frames_left : block_frames;
        size_t sample;

        if (!ID_SD_MusicRender(music, samples, frames))
        {
            goto cleanup;
        }
        for (sample = 0U; sample < (size_t)frames * 2U; ++sample)
        {
            uint16_t value = (uint16_t)samples[sample];

            pcm_bytes[sample * 2U] = (uint8_t)value;
            pcm_bytes[sample * 2U + 1U] = (uint8_t)(value >> 8);
        }
        if (fwrite(pcm_bytes, 4U, frames, stream) != frames)
        {
            goto cleanup;
        }
        frames_left -= frames;
    }
    success = fclose(stream) == 0;
    stream = NULL;

cleanup:
    if (stream != NULL)
    {
        fclose(stream);
    }
    ID_SD_MusicDestroy(music);
    WG_PagesClose(&pages);
    WG_AudioClose(&audio);
    WG_DataClose(&data_set);
    return success;
}

static int WG_HasArgument(int argc, char **argv, const char *argument)
{
    int index;

    for (index = 1; index < argc; ++index)
    {
        if (strcmp(argv[index], argument) == 0)
        {
            return 1;
        }
    }
    return 0;
}

static unsigned long long WG_FrameHash(void)
{
    unsigned long long hash = 1469598103934665603ULL;
    size_t index;

    for (index = 0; index < WG_SCREEN_WIDTH * WG_SCREEN_HEIGHT; ++index)
    {
        hash ^= WG_ScreenBuffer[index];
        hash *= 1099511628211ULL;
    }
    return hash;
}

static unsigned long long WG_PaletteHash(void)
{
    unsigned long long hash = 1469598103934665603ULL;
    size_t index;

    for (index = 0U; index < WG_PALETTE_COLORS * 3U; ++index)
    {
        hash ^= WG_Palette[index];
        hash *= 1099511628211ULL;
    }
    return hash;
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
    int smoke_test;
    const char *dump_path;
    const char *music_path;
    const char *data_path;
    const char *map_text;
    const char *sound_text;
    const char *left_text;
    const char *right_text;
    int digitized;
    int pc_speaker;
    wg_game_variant_t requested_variant = WG_GAME_UNKNOWN;
    wg_game_family_t preferred_family;
    const char *game_text;
    unsigned map_number = 0U;
    unsigned left_position = 0U;
    unsigned right_position = 0U;
    int sound_number = -1;

    smoke_test = argc == 2 && strcmp(argv[1], "--headless-smoke") == 0;
    dump_path = WG_DumpPath(argc, argv);
    music_path = WG_ArgumentValue(argc, argv, "--dump-music");
    data_path = WG_ArgumentValue(argc, argv, "--data");
    game_text = WG_ArgumentValue(argc, argv, "--game");
    preferred_family = WG_DataExecutableFamily(argc > 0 ? argv[0] : NULL);
    if (game_text != NULL)
    {
        if (!WG_DataParseGame(game_text, &requested_variant))
        {
            fprintf(stderr, "Unknown game data extension: %s\n", game_text);
            return 1;
        }
        preferred_family = requested_variant == WG_GAME_UNKNOWN
                               ? WG_GAME_FAMILY_UNKNOWN
                               : WG_DataVariantFamily(requested_variant);
    }
    map_text = WG_ArgumentValue(argc, argv, "--map");
    sound_text = WG_ArgumentValue(argc, argv, "--sound");
    left_text = WG_ArgumentValue(argc, argv, "--left-position");
    right_text = WG_ArgumentValue(argc, argv, "--right-position");
    digitized = WG_HasArgument(argc, argv, "--digitized");
    pc_speaker = WG_HasArgument(argc, argv, "--pc-speaker");
    if (map_text != NULL)
    {
        map_number = (unsigned)strtoul(map_text, NULL, 10);
    }
    if (sound_text != NULL)
    {
        sound_number = (int)strtol(sound_text, NULL, 10);
    }
    if (left_text != NULL)
    {
        left_position = (unsigned)strtoul(left_text, NULL, 10);
    }
    if (right_text != NULL)
    {
        right_position = (unsigned)strtoul(right_text, NULL, 10);
    }
    if (left_position > 15U || right_position > 15U
        || (left_position == 15U && right_position == 15U))
    {
        fprintf(stderr, "Sound positions must be 0-15 and not both 15.\n");
        return 1;
    }
    result = wolf3dgeneric_Create(argc, argv);
    if (result != WG_RESULT_OK)
    {
        fprintf(stderr, "Initialization failed with result %d.\n", (int)result);
        return 1;
    }

    result = wolf3dgeneric_Run();
    if (WG_HasArgument(argc, argv, "--frame-hash"))
    {
        printf("%016llx\n", WG_FrameHash());
    }
    if (WG_HasArgument(argc, argv, "--palette-hash"))
    {
        printf("%016llx\n", WG_PaletteHash());
    }
    if (dump_path != NULL && !WG_WriteFrame(dump_path))
    {
        fprintf(stderr, "Unable to write frame dump: %s\n", dump_path);
        wolf3dgeneric_Shutdown();
        return 1;
    }
    if (music_path != NULL
        && (data_path == NULL
            || !WG_WriteMusic(data_path, requested_variant, preferred_family,
                              map_number, sound_number, digitized, pc_speaker,
                              left_position, right_position, music_path)))
    {
        fprintf(stderr, "Unable to write music dump: %s\n", music_path);
        wolf3dgeneric_Shutdown();
        return 1;
    }
    wolf3dgeneric_Shutdown();

    if (smoke_test)
    {
        if (result != WG_RESULT_NOT_IMPLEMENTED)
        {
            fprintf(stderr, "Unexpected headless result %d.\n", (int)result);
            return 1;
        }
        return 0;
    }

    if (dump_path == NULL && music_path == NULL)
    {
        fprintf(stderr, "Headless run completed with result %d.\n", (int)result);
    }
    return result == WG_RESULT_NOT_IMPLEMENTED ? 0 : 1;
}
