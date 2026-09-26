#include "WG_AUDIO.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "WG_ENDIAN.h"

static int WG_AudioPath(char *destination, size_t destination_size,
                        const wg_data_set_t *data_set, const char *base)
{
    int result = snprintf(destination, destination_size, "%s/%s%s",
                          data_set->root, base, data_set->audio_extension);
    return result >= 0 && (size_t)result < destination_size;
}

int WG_AudioOpen(wg_audio_t *audio, const wg_data_set_t *data_set)
{
    char path[1200];
    wg_file_buffer_t header;
    size_t index;

    if (audio == NULL || data_set == NULL)
    {
        return 0;
    }
    memset(audio, 0, sizeof(*audio));
    memset(&header, 0, sizeof(header));
    if (!WG_AudioPath(path, sizeof(path), data_set, "AUDIOHED")
        || !WG_LoadFile(path, &header) || header.size < 8U
        || header.size % 4U != 0)
    {
        goto failure;
    }
    audio->offset_count = header.size / 4U;
    audio->offsets = (uint32_t *)malloc(audio->offset_count
                                        * sizeof(*audio->offsets));
    if (audio->offsets == NULL)
    {
        goto failure;
    }
    for (index = 0; index < audio->offset_count; ++index)
    {
        audio->offsets[index] = WG_ReadLE32(header.data + index * 4U);
    }
    if (!WG_AudioPath(path, sizeof(path), data_set, "AUDIOT")
        || !WG_LoadFile(path, &audio->data))
    {
        goto failure;
    }
    for (index = 0; index < audio->offset_count; ++index)
    {
        if ((size_t)audio->offsets[index] > audio->data.size
            || (index != 0 && audio->offsets[index] < audio->offsets[index - 1U]))
        {
            goto failure;
        }
    }
    WG_FreeFile(&header);
    return 1;

failure:
    WG_FreeFile(&header);
    WG_AudioClose(audio);
    return 0;
}

void WG_AudioClose(wg_audio_t *audio)
{
    if (audio == NULL)
    {
        return;
    }
    free(audio->offsets);
    WG_FreeFile(&audio->data);
    memset(audio, 0, sizeof(*audio));
}

int WG_AudioGetChunk(const wg_audio_t *audio, size_t chunk,
                     const uint8_t **data, size_t *size)
{
    size_t start;
    size_t end;

    if (audio == NULL || data == NULL || size == NULL
        || chunk + 1U >= audio->offset_count)
    {
        return 0;
    }
    start = audio->offsets[chunk];
    end = audio->offsets[chunk + 1U];
    if (start > end || end > audio->data.size)
    {
        return 0;
    }
    *data = audio->data.data + start;
    *size = end - start;
    return 1;
}
