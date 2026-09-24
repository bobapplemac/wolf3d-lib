#ifndef ID_SD_H
#define ID_SD_H

#include <stddef.h>
#include <stdint.h>

#define ID_SD_IMF_RATE 700U

typedef void (*id_sd_opl_write_fn)(void *user, uint16_t register_number,
                                   uint8_t value);

typedef struct id_sd_imf
{
    const uint8_t *events;
    size_t length;
    size_t position;
    uint32_t time;
    uint32_t next_event;
    id_sd_opl_write_fn write;
    void *write_user;
    uint8_t active;
} id_sd_imf_t;

typedef struct id_sd_sample_clock
{
    uint32_t sample_rate;
    uint32_t phase;
} id_sd_sample_clock_t;

typedef struct id_sd_music id_sd_music_t;

int ID_SD_IMFStart(id_sd_imf_t *sequence, const uint8_t *chunk,
                   size_t chunk_size, id_sd_opl_write_fn write,
                   void *write_user);
void ID_SD_IMFStop(id_sd_imf_t *sequence);
void ID_SD_IMFService(id_sd_imf_t *sequence);

int ID_SD_SampleClockStart(id_sd_sample_clock_t *clock,
                           uint32_t sample_rate);
uint32_t ID_SD_SampleClockFramesToTick(const id_sd_sample_clock_t *clock);
uint32_t ID_SD_SampleClockAdvance(id_sd_sample_clock_t *clock,
                                  uint32_t frames);

id_sd_music_t *ID_SD_MusicCreate(uint32_t sample_rate);
void ID_SD_MusicDestroy(id_sd_music_t *music);
int ID_SD_MusicStart(id_sd_music_t *music, const uint8_t *chunk,
                     size_t chunk_size);
void ID_SD_MusicStop(id_sd_music_t *music);
int ID_SD_MusicRender(id_sd_music_t *music, int16_t *stereo,
                      size_t frame_count);
int ID_SD_EffectStart(id_sd_music_t *music, const uint8_t *chunk,
                      size_t chunk_size);
void ID_SD_EffectStop(id_sd_music_t *music);
int ID_SD_EffectPlaying(const id_sd_music_t *music);

#endif
