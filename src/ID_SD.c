/* Portable IMF sequencer and sample clock derived from ID_SD.C. */
#include "ID_SD.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "WG_ENDIAN.h"
#include "WG_OPL.h"

/* The original SDL_StartSB raised the SB Pro FM mixer to maximum so its
   output was comparable with the digitized voice path.  Nuked-OPL3 returns
   the chip's unamplified digital level, so reproduce that board-level gain at
   our mixer boundary without modifying the emulator itself. */
#define ID_SD_FM_MIX_GAIN 4

struct id_sd_music
{
    wg_opl_t *opl;
    id_sd_imf_t sequence;
    id_sd_sample_clock_t clock;
    uint32_t sample_rate;
    const uint8_t *effect_data;
    uint32_t effect_length;
    uint32_t effect_position;
    uint16_t effect_priority;
    uint8_t effect_block;
    uint8_t effect_divider;
    const uint8_t *pc_data;
    uint32_t pc_length;
    uint32_t pc_position;
    uint64_t pc_phase;
    uint16_t pc_priority;
    uint16_t pc_divisor;
    uint8_t pc_last_sample;
    uint8_t pc_polarity;
    uint8_t *digital_data;
    size_t digital_length;
    size_t digital_position;
    uint32_t digital_phase;
    uint16_t digital_priority;
    uint8_t digital_left;
    uint8_t digital_right;
    uint8_t music_paused;
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

    WG_OPL_WriteRegisterBuffered(music->opl, register_number, value);
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
    music->opl = WG_OPL_Create(sample_rate);
    if (music->opl == NULL
        || !ID_SD_SampleClockStart(&music->clock, sample_rate))
    {
        WG_OPL_Destroy(music->opl);
        free(music);
        return NULL;
    }
    return music;
}

void ID_SD_MusicDestroy(id_sd_music_t *music)
{
    if (music != NULL)
    {
        free(music->digital_data);
        WG_OPL_Destroy(music->opl);
    }
    free(music);
}

static void ID_SD_EffectWriteInstrument(id_sd_music_t *music,
                                        const uint8_t *instrument)
{
    static const uint16_t registers[10] =
    {
        0x20U, 0x23U, 0x40U, 0x43U, 0x60U,
        0x63U, 0x80U, 0x83U, 0xe0U, 0xe3U
    };
    size_t index;

    for (index = 0U; index < 10U; ++index)
    {
        WG_OPL_WriteRegisterBuffered(music->opl, registers[index],
                                     instrument[index]);
    }
    /* The Wolf3D source deliberately uses zero rather than inst.nConn. */
    WG_OPL_WriteRegisterBuffered(music->opl, 0xc0U, 0U);
}

int ID_SD_EffectStart(id_sd_music_t *music, const uint8_t *chunk,
                      size_t chunk_size)
{
    uint32_t length;
    uint16_t priority;

    if (music == NULL || chunk == NULL || chunk_size < 24U)
    {
        return 0;
    }
    length = WG_ReadLE32(chunk);
    priority = WG_ReadLE16(chunk + 4U);
    if (length == 0U || length > chunk_size - 23U
        || (chunk[12U] == 0U && chunk[13U] == 0U)
        || (music->effect_data != NULL
            && priority < music->effect_priority))
    {
        return 0;
    }
    ID_SD_PCStop(music);
    ID_SD_EffectStop(music);
    music->effect_data = chunk + 23U;
    music->effect_length = length;
    music->effect_position = 0U;
    music->effect_priority = priority;
    music->effect_block = (uint8_t)(((chunk[22U] & 7U) << 2) | 0x20U);
    ID_SD_EffectWriteInstrument(music, chunk + 6U);
    return 1;
}

void ID_SD_EffectStop(id_sd_music_t *music)
{
    if (music == NULL)
    {
        return;
    }
    music->effect_data = NULL;
    music->effect_length = 0U;
    music->effect_position = 0U;
    music->effect_priority = 0U;
    WG_OPL_WriteRegisterBuffered(music->opl, 0xb0U, 0U);
}

int ID_SD_EffectPlaying(const id_sd_music_t *music)
{
    return music != NULL && music->effect_data != NULL;
}

int ID_SD_PCStart(id_sd_music_t *music, const uint8_t *chunk,
                  size_t chunk_size)
{
    uint32_t length;
    uint16_t priority;

    if (music == NULL || chunk == NULL || chunk_size < 7U)
    {
        return 0;
    }
    length = WG_ReadLE32(chunk);
    priority = WG_ReadLE16(chunk + 4U);
    if (length == 0U || length > chunk_size - 6U
        || (music->pc_data != NULL && priority < music->pc_priority))
    {
        return 0;
    }
    ID_SD_EffectStop(music);
    ID_SD_PCStop(music);
    music->pc_data = chunk + 6U;
    music->pc_length = length;
    music->pc_priority = priority;
    music->pc_last_sample = UINT8_MAX;
    return 1;
}

void ID_SD_PCStop(id_sd_music_t *music)
{
    if (music == NULL)
    {
        return;
    }
    music->pc_data = NULL;
    music->pc_length = 0U;
    music->pc_position = 0U;
    music->pc_phase = 0U;
    music->pc_priority = 0U;
    music->pc_divisor = 0U;
    music->pc_last_sample = UINT8_MAX;
    music->pc_polarity = 0U;
}

int ID_SD_PCPlaying(const id_sd_music_t *music)
{
    return music != NULL && music->pc_data != NULL;
}

int ID_SD_DigiBankOpen(id_sd_digi_bank_t *bank, const wg_pages_t *pages)
{
    const uint8_t *table;
    size_t table_size;
    size_t offset;
    size_t page;

    if (bank == NULL || pages == NULL || pages->data_set == NULL
        || pages->data_set->page_count == 0U)
    {
        return 0;
    }
    memset(bank, 0, sizeof(*bank));
    if (!WG_PagesGet(pages, pages->data_set->page_count - 1U,
                     &table, &table_size)
        || table_size % 4U != 0U)
    {
        return 0;
    }
    page = pages->data_set->sound_start;
    for (offset = 0U;
         offset + 4U <= table_size
         && page < pages->data_set->page_count - 1U; offset += 4U)
    {
        uint16_t start_page = WG_ReadLE16(table + offset);
        uint16_t length = WG_ReadLE16(table + offset + 2U);
        size_t page_count;

        if (bank->count == ID_SD_MAX_DIGITIZED_SOUNDS || length == 0U
            || start_page != page - pages->data_set->sound_start)
        {
            return 0;
        }
        page_count = ((size_t)length + 4095U) / 4096U;
        if ((size_t)pages->data_set->sound_start + start_page
                >= pages->data_set->page_count - 1U
            || page_count > pages->data_set->page_count - 1U
                                - pages->data_set->sound_start - start_page)
        {
            return 0;
        }
        bank->entries[bank->count].start_page = start_page;
        bank->entries[bank->count].length = length;
        ++bank->count;
        page += page_count;
    }
    bank->pages = pages;
    return bank->count != 0U;
}

int ID_SD_DigiBankLoad(const id_sd_digi_bank_t *bank, size_t sound,
                       uint8_t **data, size_t *length)
{
    const id_sd_digi_entry_t *entry;
    uint8_t *copy;
    size_t copied = 0U;
    size_t page_index;

    if (data == NULL || length == NULL)
    {
        return 0;
    }
    *data = NULL;
    *length = 0U;
    if (bank == NULL || bank->pages == NULL || sound >= bank->count)
    {
        return 0;
    }
    entry = &bank->entries[sound];
    copy = (uint8_t *)malloc(entry->length);
    if (copy == NULL)
    {
        return 0;
    }
    page_index = bank->pages->data_set->sound_start + entry->start_page;
    while (copied < entry->length)
    {
        const uint8_t *page_data;
        size_t page_size;
        size_t amount;

        if (!WG_PagesGet(bank->pages, page_index++, &page_data, &page_size))
        {
            free(copy);
            return 0;
        }
        amount = entry->length - copied;
        if (amount > page_size)
        {
            amount = page_size;
        }
        memcpy(copy + copied, page_data, amount);
        copied += amount;
    }
    *data = copy;
    *length = entry->length;
    return 1;
}

int ID_SD_DigitalNumberForSound(unsigned sound)
{
    static const uint8_t mapping[][2] =
    {
        {21U, 0U}, {41U, 1U}, {19U, 2U}, {18U, 3U}, {26U, 4U},
        {24U, 5U}, {11U, 6U}, {51U, 7U}, {55U, 8U}, {50U, 9U},
        {59U, 10U}, {60U, 11U}, {29U, 12U}, {22U, 13U}, {25U, 13U},
        {16U, 14U}, {46U, 15U}, {10U, 16U}, {52U, 17U}, {53U, 18U},
        {54U, 19U}, {56U, 20U}, {58U, 21U}, {61U, 22U}, {62U, 23U},
        {63U, 24U}, {64U, 25U}, {65U, 26U}, {66U, 27U}, {67U, 28U},
        {68U, 29U}, {40U, 30U}, {70U, 31U}, {72U, 32U}, {57U, 33U},
        {73U, 34U}, {74U, 35U}, {79U, 36U}, {80U, 37U}, {81U, 38U},
        {75U, 39U}, {76U, 40U}, {77U, 41U}, {78U, 42U}, {82U, 43U},
        {83U, 44U}, {84U, 45U}
    };
    size_t index;

    for (index = 0U; index < sizeof(mapping) / sizeof(mapping[0]); ++index)
    {
        if (mapping[index][0] == sound)
        {
            return mapping[index][1];
        }
    }
    return -1;
}

int ID_SD_DigitalNumberForSoundForVariant(wg_game_variant_t variant,
                                           unsigned sound)
{
    static const uint8_t spear_mapping[][2] =
    {
        {21U, 0U}, {41U, 1U}, {19U, 2U}, {18U, 3U}, {26U, 4U},
        {24U, 5U}, {11U, 6U}, {51U, 7U}, {59U, 8U}, {60U, 9U},
        {29U, 10U}, {22U, 11U}, {16U, 12U}, {46U, 13U}, {10U, 14U},
        {52U, 15U}, {56U, 16U}, {58U, 17U}, {61U, 18U}, {66U, 19U},
        {67U, 20U}, {68U, 21U}, {40U, 22U}, {50U, 23U}, {25U, 23U},
        {53U, 24U}, {57U, 25U}, {54U, 26U}, {55U, 27U}, {63U, 28U},
        {70U, 29U}, {71U, 30U}, {72U, 31U}, {73U, 32U}, {74U, 33U},
        {75U, 34U}, {76U, 35U}, {65U, 36U}, {77U, 37U}, {38U, 38U},
        {79U, 39U}
    };
    size_t index;

    if (WG_DataVariantFamily(variant) != WG_GAME_FAMILY_SPEAR)
    {
        return ID_SD_DigitalNumberForSound(sound);
    }
    for (index = 0U;
         index < sizeof(spear_mapping) / sizeof(spear_mapping[0]);
         ++index)
    {
        if (spear_mapping[index][0] == sound)
        {
            if (variant == WG_GAME_SPEAR_DEMO_SDM
                && (spear_mapping[index][1] == 1U
                    || spear_mapping[index][1] == 14U
                    || (spear_mapping[index][1] >= 19U
                        && spear_mapping[index][1] <= 21U)
                    || spear_mapping[index][1] >= 24U))
            {
                return -1;
            }
            return spear_mapping[index][1];
        }
    }
    return -1;
}

int ID_SD_DigitalStart(id_sd_music_t *music, const uint8_t *data,
                       size_t length, uint16_t priority,
                       uint8_t left_position, uint8_t right_position)
{
    uint8_t *copy;

    if (music == NULL || data == NULL || length == 0U
        || left_position > 15U || right_position > 15U
        || (left_position == 15U && right_position == 15U)
        || (music->digital_data != NULL
            && priority < music->digital_priority))
    {
        return 0;
    }
    copy = (uint8_t *)malloc(length);
    if (copy == NULL)
    {
        return 0;
    }
    memcpy(copy, data, length);
    ID_SD_DigitalStop(music);
    music->digital_data = copy;
    music->digital_length = length;
    music->digital_priority = priority;
    music->digital_left = left_position;
    music->digital_right = right_position;
    return 1;
}

int ID_SD_DigitalSetPosition(id_sd_music_t *music,
                            uint8_t left_position, uint8_t right_position)
{
    if (music == NULL || music->digital_data == NULL
        || left_position > 15U || right_position > 15U
        || (left_position == 15U && right_position == 15U))
    {
        return 0;
    }
    music->digital_left = left_position;
    music->digital_right = right_position;
    return 1;
}

void ID_SD_DigitalStop(id_sd_music_t *music)
{
    if (music == NULL)
    {
        return;
    }
    free(music->digital_data);
    music->digital_data = NULL;
    music->digital_length = 0U;
    music->digital_position = 0U;
    music->digital_phase = 0U;
    music->digital_priority = 0U;
}

int ID_SD_DigitalPlaying(const id_sd_music_t *music)
{
    return music != NULL && music->digital_data != NULL;
}

static int16_t ID_SD_ClampSample(int32_t sample)
{
    if (sample > INT16_MAX)
    {
        return INT16_MAX;
    }
    if (sample < INT16_MIN)
    {
        return INT16_MIN;
    }
    return (int16_t)sample;
}

static void ID_SD_ApplyFMMixGain(int16_t *stereo, uint32_t frame_count)
{
    uint32_t sample;

    for (sample = 0U; sample < frame_count * 2U; ++sample)
    {
        stereo[sample] = ID_SD_ClampSample(
            (int32_t)stereo[sample] * ID_SD_FM_MIX_GAIN);
    }
}

static void ID_SD_DigitalMix(id_sd_music_t *music, int16_t *stereo,
                             uint32_t frame_count)
{
    uint32_t frame;

    for (frame = 0U; frame < frame_count && music->digital_data != NULL;
         ++frame)
    {
        int32_t sample = ((int32_t)music->digital_data[music->digital_position]
                          - 128) * 256;
        int32_t left = sample * (15 - music->digital_left) / 15;
        int32_t right = sample * (15 - music->digital_right) / 15;

        stereo[frame * 2U] =
            ID_SD_ClampSample((int32_t)stereo[frame * 2U] + left);
        stereo[frame * 2U + 1U] =
            ID_SD_ClampSample((int32_t)stereo[frame * 2U + 1U] + right);
        music->digital_phase += ID_SD_DIGITAL_RATE;
        while (music->digital_phase >= music->sample_rate)
        {
            music->digital_phase -= music->sample_rate;
            if (++music->digital_position == music->digital_length)
            {
                ID_SD_DigitalStop(music);
                break;
            }
        }
    }
}

static void ID_SD_EffectService(id_sd_music_t *music)
{
    uint8_t sample;

    if (music->effect_data == NULL)
    {
        return;
    }
    sample = music->effect_data[music->effect_position++];
    if (sample == 0U)
    {
        WG_OPL_WriteRegisterBuffered(music->opl, 0xb0U, 0U);
    }
    else
    {
        WG_OPL_WriteRegisterBuffered(music->opl, 0xa0U, sample);
        WG_OPL_WriteRegisterBuffered(music->opl, 0xb0U,
                                     music->effect_block);
    }
    if (music->effect_position == music->effect_length)
    {
        ID_SD_EffectStop(music);
    }
}

static void ID_SD_PCService(id_sd_music_t *music)
{
    uint8_t sample;

    if (music->pc_data == NULL)
    {
        return;
    }
    sample = music->pc_data[music->pc_position++];
    if (sample != music->pc_last_sample)
    {
        music->pc_last_sample = sample;
        music->pc_divisor = (uint16_t)((uint16_t)sample * 60U);
        music->pc_phase = 0U;
        music->pc_polarity = 1U;
    }
    if (music->pc_position == music->pc_length)
    {
        ID_SD_PCStop(music);
    }
}

static void ID_SD_PCMix(id_sd_music_t *music, int16_t *stereo,
                        uint32_t frame_count)
{
    uint32_t frame;

    if (music->pc_data == NULL || music->pc_divisor == 0U)
    {
        return;
    }
    for (frame = 0U; frame < frame_count; ++frame)
    {
        int32_t sample = music->pc_polarity ? 4096 : -4096;
        uint64_t threshold =
            (uint64_t)music->sample_rate * music->pc_divisor;

        stereo[frame * 2U] = ID_SD_ClampSample(
            (int32_t)stereo[frame * 2U] + sample);
        stereo[frame * 2U + 1U] = ID_SD_ClampSample(
            (int32_t)stereo[frame * 2U + 1U] + sample);
        music->pc_phase += ID_SD_PIT_RATE;
        while (music->pc_phase >= threshold)
        {
            music->pc_phase -= threshold;
            music->pc_polarity ^= 1U;
        }
    }
}

int ID_SD_MusicStart(id_sd_music_t *music, const uint8_t *chunk,
                     size_t chunk_size)
{
    if (music == NULL)
    {
        return 0;
    }
    ID_SD_MusicStop(music);
    music->music_paused = 0U;
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
    WG_OPL_WriteRegister(music->opl, 0xbdU, 0U);
    for (channel = 1U; channel < 9U; ++channel)
    {
        WG_OPL_WriteRegister(music->opl,
                             (uint16_t)(0xb0U + channel), 0U);
    }
}

void ID_SD_MusicSetPaused(id_sd_music_t *music, int paused)
{
    uint16_t channel;

    if (music == NULL)
    {
        return;
    }
    music->music_paused = paused != 0;
    if (!music->music_paused)
    {
        return;
    }

    /* SD_MusicOff silenced the rhythm register and music channels while
       leaving the sequencer position intact for SD_MusicOn. */
    WG_OPL_WriteRegister(music->opl, 0xbdU, 0U);
    for (channel = 1U; channel < 9U; ++channel)
    {
        WG_OPL_WriteRegister(music->opl,
                             (uint16_t)(0xb0U + channel), 0U);
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
        WG_OPL_Generate(music->opl, stereo, frames);
        ID_SD_ApplyFMMixGain(stereo, frames);
        ID_SD_DigitalMix(music, stereo, frames);
        ID_SD_PCMix(music, stereo, frames);
#ifdef WG_AUDIO_SILENT
        /* Keep every original audio clock and completion transition active,
           but deliberately expose silence to hosts without audio output. */
        memset(stereo, 0, (size_t)frames * 2U * sizeof(*stereo));
#endif
        stereo += (size_t)frames * 2U;
        frame_count -= frames;
        ticks = ID_SD_SampleClockAdvance(&music->clock, frames);
        while (ticks-- != 0U)
        {
            if (!music->music_paused)
            {
                ID_SD_IMFService(&music->sequence);
            }
            if (++music->effect_divider == 5U)
            {
                music->effect_divider = 0U;
                ID_SD_EffectService(music);
                ID_SD_PCService(music);
            }
        }
    }
    return 1;
}
