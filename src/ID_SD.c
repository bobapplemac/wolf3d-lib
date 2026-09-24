/* Portable IMF sequencer and sample clock derived from ID_SD.C. */
#include "ID_SD.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "WG_ENDIAN.h"
#include "opl3.h"

struct id_sd_music
{
    opl3_chip chip;
    id_sd_imf_t sequence;
    id_sd_sample_clock_t clock;
    uint32_t sample_rate;
};

int ID_SD_IMFStart(id_sd_imf_t *sequence, const uint8_t *chunk,
                   size_t chunk_size, id_sd_opl_write_fn write,
                   void *write_user)
{
    size_t length;

    if (sequence == NULL || chunk == NULL || chunk_size < 6U || write == NULL)
    {
        return 0;
    }
    length = WG_ReadLE16(chunk);
    if (length == 0U || length % 4U != 0U || length > chunk_size - 2U)
    {
        return 0;
    }
    memset(sequence, 0, sizeof(*sequence));
    sequence->events = chunk + 2U;
    sequence->length = length;
    sequence->write = write;
    sequence->write_user = write_user;
    sequence->active = 1U;
    return 1;
}

void ID_SD_IMFStop(id_sd_imf_t *sequence)
{
    if (sequence != NULL)
    {
        sequence->active = 0U;
        sequence->position = 0U;
        sequence->time = 0U;
        sequence->next_event = 0U;
    }
}

void ID_SD_IMFService(id_sd_imf_t *sequence)
{
    if (sequence == NULL || !sequence->active)
    {
        return;
    }

    while (sequence->position < sequence->length
           && sequence->next_event <= sequence->time)
    {
        const uint8_t *event = sequence->events + sequence->position;
        uint16_t delay = WG_ReadLE16(event + 2U);

        sequence->write(sequence->write_user, event[0], event[1]);
        sequence->next_event = sequence->time + delay;
        sequence->position += 4U;
    }
    ++sequence->time;
    if (sequence->position == sequence->length)
    {
        sequence->position = 0U;
        sequence->time = 0U;
        sequence->next_event = 0U;
    }
}

int ID_SD_SampleClockStart(id_sd_sample_clock_t *clock,
                           uint32_t sample_rate)
{
    if (clock == NULL || sample_rate < ID_SD_IMF_RATE)
    {
        return 0;
    }
    clock->sample_rate = sample_rate;
    clock->phase = 0U;
    return 1;
}

uint32_t ID_SD_SampleClockFramesToTick(const id_sd_sample_clock_t *clock)
{
    uint32_t remaining;

    if (clock == NULL || clock->sample_rate < ID_SD_IMF_RATE
        || clock->phase >= clock->sample_rate)
    {
        return 0U;
    }
    remaining = clock->sample_rate - clock->phase;
    return (remaining + ID_SD_IMF_RATE - 1U) / ID_SD_IMF_RATE;
}

uint32_t ID_SD_SampleClockAdvance(id_sd_sample_clock_t *clock,
                                  uint32_t frames)
{
    uint64_t phase;
    uint32_t ticks;

    if (clock == NULL || clock->sample_rate < ID_SD_IMF_RATE)
    {
        return 0U;
    }
    phase = (uint64_t)clock->phase + (uint64_t)frames * ID_SD_IMF_RATE;
    ticks = (uint32_t)(phase / clock->sample_rate);
    clock->phase = (uint32_t)(phase % clock->sample_rate);
    return ticks;
}

static void ID_SD_MusicWrite(void *user, uint16_t register_number,
                             uint8_t value)
{
    id_sd_music_t *music = (id_sd_music_t *)user;

    OPL3_WriteRegBuffered(&music->chip, register_number, value);
}

id_sd_music_t *ID_SD_MusicCreate(uint32_t sample_rate)
{
    id_sd_music_t *music;

    if (sample_rate < ID_SD_IMF_RATE)
    {
        return NULL;
    }
    music = (id_sd_music_t *)calloc(1U, sizeof(*music));
    if (music == NULL)
    {
        return NULL;
    }
    music->sample_rate = sample_rate;
    OPL3_Reset(&music->chip, sample_rate);
    if (!ID_SD_SampleClockStart(&music->clock, sample_rate))
    {
        free(music);
        return NULL;
    }
    return music;
}

void ID_SD_MusicDestroy(id_sd_music_t *music)
{
    free(music);
}

int ID_SD_MusicStart(id_sd_music_t *music, const uint8_t *chunk,
                     size_t chunk_size)
{
    if (music == NULL)
    {
        return 0;
    }
    ID_SD_MusicStop(music);
    OPL3_Reset(&music->chip, music->sample_rate);
    if (!ID_SD_SampleClockStart(&music->clock, music->sample_rate)
        || !ID_SD_IMFStart(&music->sequence, chunk, chunk_size,
                           ID_SD_MusicWrite, music))
    {
        return 0;
    }
    /* The original 700 Hz service writes all time-zero IMF events first. */
    ID_SD_IMFService(&music->sequence);
    return 1;
}

void ID_SD_MusicStop(id_sd_music_t *music)
{
    uint16_t channel;

    if (music == NULL)
    {
        return;
    }
    ID_SD_IMFStop(&music->sequence);
    OPL3_WriteReg(&music->chip, 0xbdU, 0U);
    for (channel = 0U; channel < 9U; ++channel)
    {
        OPL3_WriteReg(&music->chip, (uint16_t)(0xb0U + channel), 0U);
    }
}

int ID_SD_MusicRender(id_sd_music_t *music, int16_t *stereo,
                      size_t frame_count)
{
    if (music == NULL || (frame_count != 0U && stereo == NULL))
    {
        return 0;
    }
    while (frame_count != 0U)
    {
        uint32_t until_tick = ID_SD_SampleClockFramesToTick(&music->clock);
        uint32_t frames = frame_count < until_tick
                              ? (uint32_t)frame_count : until_tick;
        uint32_t ticks;

        if (frames == 0U)
        {
            return 0;
        }
        OPL3_GenerateStream(&music->chip, stereo, frames);
        stereo += (size_t)frames * 2U;
        frame_count -= frames;
        ticks = ID_SD_SampleClockAdvance(&music->clock, frames);
        while (ticks-- != 0U)
        {
            ID_SD_IMFService(&music->sequence);
        }
    }
    return 1;
}
