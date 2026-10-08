#ifndef WG_AUDIO_H
#define WG_AUDIO_H

#include <stddef.h>
#include <stdint.h>

#include "WG_DATA.h"
#include "WG_FILE.h"

typedef struct wg_audio
{
    uint32_t *offsets;
    size_t offset_count;
    wg_file_buffer_t data;
} wg_audio_t;

int WG_AudioOpen(wg_audio_t *audio, const wg_data_set_t *data_set);
void WG_AudioClose(wg_audio_t *audio);
int WG_AudioGetChunk(const wg_audio_t *audio, size_t chunk,
                     const uint8_t **data, size_t *size);

#endif
