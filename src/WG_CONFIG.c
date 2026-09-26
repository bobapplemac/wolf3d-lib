/* Portable replacement for WL_MAIN.C's compiler-layout CONFIG file. */
#include "WG_CONFIG.h"

#include <stdlib.h>
#include <string.h>

#include "WG_ENDIAN.h"
#include "WG_FILE.h"
#include "WOLF3DGENERIC.h"

#define WG_CONFIG_VERSION 1U

static const uint8_t WG_ConfigMagic[8] =
    { 'W', '3', 'D', 'G', 'C', 'F', 'G', '1' };

typedef struct wg_config_cursor
{
    uint8_t *write_data;
    const uint8_t *read_data;
    size_t size;
    size_t position;
    int valid;
} wg_config_cursor_t;

static uint32_t WG_ConfigChecksum(const uint8_t *data, size_t size)
{
    uint32_t hash = 2166136261U;
    size_t index;

    for (index = 0U; index < size; ++index)
    {
        hash ^= data[index];
        hash *= 16777619U;
    }
    return hash;
}

static void WG_ConfigPut8(wg_config_cursor_t *cursor, uint8_t value)
{
    if (!cursor->valid || cursor->position == cursor->size)
    {
        cursor->valid = 0;
        return;
    }
    cursor->write_data[cursor->position++] = value;
}

static void WG_ConfigPut16(wg_config_cursor_t *cursor, uint16_t value)
{
    WG_ConfigPut8(cursor, (uint8_t)value);
    WG_ConfigPut8(cursor, (uint8_t)(value >> 8));
}

static void WG_ConfigPut32(wg_config_cursor_t *cursor, uint32_t value)
{
    WG_ConfigPut16(cursor, (uint16_t)value);
    WG_ConfigPut16(cursor, (uint16_t)(value >> 16));
}

static uint8_t WG_ConfigGet8(wg_config_cursor_t *cursor)
{
    if (!cursor->valid || cursor->position == cursor->size)
    {
        cursor->valid = 0;
        return 0U;
    }
    return cursor->read_data[cursor->position++];
}

static uint16_t WG_ConfigGet16(wg_config_cursor_t *cursor)
{
    uint16_t low = WG_ConfigGet8(cursor);
    uint16_t high = WG_ConfigGet8(cursor);
    return (uint16_t)(low | (high << 8));
}

static uint32_t WG_ConfigGet32(wg_config_cursor_t *cursor)
{
    uint32_t low = WG_ConfigGet16(cursor);
    uint32_t high = WG_ConfigGet16(cursor);
    return low | (high << 16);
}

void WG_ConfigDefaults(wg_config_t *config)
{
    static const uint16_t action_keys[WL_CUSTOM_BINDINGS] =
        { WG_KEY_RIGHT_SHIFT, WG_KEY_SPACE, WG_KEY_CONTROL, WG_KEY_ALT };
    static const uint16_t movement_keys[WL_CUSTOM_BINDINGS] =
        { WG_KEY_LEFT, WG_KEY_RIGHT, WG_KEY_UP, WG_KEY_DOWN };
    static const uint8_t mouse_bindings[WL_CUSTOM_BINDINGS] =
        { UINT8_MAX, 2U, 0U, 1U };

    if (config == NULL)
    {
        return;
    }
    memset(config, 0, sizeof(*config));
    WL_HighScoresDefault(config->high_scores);
    memcpy(config->action_keys, action_keys, sizeof(action_keys));
    memcpy(config->movement_keys, movement_keys, sizeof(movement_keys));
    memcpy(config->mouse_bindings, mouse_bindings, sizeof(mouse_bindings));
    config->adlib_effects = 1U;
    config->digitized_effects = 1U;
    config->music_enabled = 1U;
    config->mouse_enabled = 1U;
    config->mouse_adjustment = 5U;
    config->view_size = WL_VIEW_SIZE_DEFAULT;
}

static int WG_ConfigValid(const wg_config_t *config)
{
    unsigned index;

    if (config == NULL || config->adlib_effects > 1U
        || config->digitized_effects > 1U || config->music_enabled > 1U
        || config->mouse_enabled > 1U || config->mouse_adjustment > 9U
        || config->view_size < WL_VIEW_SIZE_MIN
        || config->view_size > WL_VIEW_SIZE_MAX)
    {
        return 0;
    }
    for (index = 0U; index < WL_CUSTOM_BINDINGS; ++index)
    {
        if (config->action_keys[index] >= 128U
            || config->movement_keys[index] >= 128U
            || (config->mouse_bindings[index] >= 3U
                && config->mouse_bindings[index] != UINT8_MAX))
        {
            return 0;
        }
    }
    for (index = 0U; index < WL_MAX_HIGH_SCORES; ++index)
    {
        if (memchr(config->high_scores[index].name, '\0',
                   sizeof(config->high_scores[index].name)) == NULL)
        {
            return 0;
        }
    }
    return 1;
}

int WG_ConfigEncode(uint8_t *data, size_t capacity, size_t *size,
                    wg_game_variant_t variant,
                    const wg_config_t *config)
{
    wg_config_cursor_t cursor;
    unsigned index;
    size_t name_index;
    uint32_t checksum;

    if (data == NULL || size == NULL || !WG_ConfigValid(config))
    {
        return 0;
    }
    cursor.write_data = data;
    cursor.read_data = NULL;
    cursor.size = capacity;
    cursor.position = 0U;
    cursor.valid = 1;
    for (index = 0U; index < sizeof(WG_ConfigMagic); ++index)
    {
        WG_ConfigPut8(&cursor, WG_ConfigMagic[index]);
    }
    WG_ConfigPut16(&cursor, WG_CONFIG_VERSION);
    WG_ConfigPut8(&cursor, (uint8_t)variant);
    for (index = 0U; index < WL_MAX_HIGH_SCORES; ++index)
    {
        for (name_index = 0U; name_index < WL_MAX_HIGH_NAME + 1U;
             ++name_index)
        {
            WG_ConfigPut8(&cursor,
                          (uint8_t)config->high_scores[index].name[name_index]);
        }
        WG_ConfigPut32(&cursor, config->high_scores[index].score);
        WG_ConfigPut16(&cursor, config->high_scores[index].completed);
        WG_ConfigPut16(&cursor, config->high_scores[index].episode);
    }
    for (index = 0U; index < WL_CUSTOM_BINDINGS; ++index)
    {
        WG_ConfigPut16(&cursor, config->action_keys[index]);
        WG_ConfigPut16(&cursor, config->movement_keys[index]);
        WG_ConfigPut8(&cursor, config->mouse_bindings[index]);
    }
    WG_ConfigPut8(&cursor, config->adlib_effects);
    WG_ConfigPut8(&cursor, config->digitized_effects);
    WG_ConfigPut8(&cursor, config->music_enabled);
    WG_ConfigPut8(&cursor, config->mouse_enabled);
    WG_ConfigPut8(&cursor, config->mouse_adjustment);
    WG_ConfigPut8(&cursor, config->view_size);
    if (!cursor.valid || cursor.size - cursor.position < 4U)
    {
        return 0;
    }
    checksum = WG_ConfigChecksum(data, cursor.position);
    WG_ConfigPut32(&cursor, checksum);
    if (!cursor.valid)
    {
        return 0;
    }
    *size = cursor.position;
    return 1;
}

int WG_ConfigDecode(const uint8_t *data, size_t size,
                    wg_game_variant_t variant,
                    wg_config_t *config)
{
    wg_config_cursor_t cursor;
    wg_config_t decoded;
    unsigned index;
    size_t name_index;
    uint8_t magic[sizeof(WG_ConfigMagic)];
    uint16_t version;
    uint8_t stored_variant;

    if (data == NULL || config == NULL
        || size < sizeof(WG_ConfigMagic) + 2U + 1U + 4U
        || WG_ReadLE32(data + size - 4U)
               != WG_ConfigChecksum(data, size - 4U))
    {
        return 0;
    }
    memset(&decoded, 0, sizeof(decoded));
    cursor.write_data = NULL;
    cursor.read_data = data;
    cursor.size = size - 4U;
    cursor.position = 0U;
    cursor.valid = 1;
    for (index = 0U; index < sizeof(magic); ++index)
    {
        magic[index] = WG_ConfigGet8(&cursor);
    }
    version = WG_ConfigGet16(&cursor);
    stored_variant = WG_ConfigGet8(&cursor);
    for (index = 0U; index < WL_MAX_HIGH_SCORES; ++index)
    {
        for (name_index = 0U; name_index < WL_MAX_HIGH_NAME + 1U;
             ++name_index)
        {
            decoded.high_scores[index].name[name_index] =
                (char)WG_ConfigGet8(&cursor);
        }
        decoded.high_scores[index].score = WG_ConfigGet32(&cursor);
        decoded.high_scores[index].completed = WG_ConfigGet16(&cursor);
        decoded.high_scores[index].episode = WG_ConfigGet16(&cursor);
    }
    for (index = 0U; index < WL_CUSTOM_BINDINGS; ++index)
    {
        decoded.action_keys[index] = WG_ConfigGet16(&cursor);
        decoded.movement_keys[index] = WG_ConfigGet16(&cursor);
        decoded.mouse_bindings[index] = WG_ConfigGet8(&cursor);
    }
    decoded.adlib_effects = WG_ConfigGet8(&cursor);
    decoded.digitized_effects = WG_ConfigGet8(&cursor);
    decoded.music_enabled = WG_ConfigGet8(&cursor);
    decoded.mouse_enabled = WG_ConfigGet8(&cursor);
    decoded.mouse_adjustment = WG_ConfigGet8(&cursor);
    decoded.view_size = WG_ConfigGet8(&cursor);
    if (!cursor.valid || cursor.position != cursor.size
        || memcmp(magic, WG_ConfigMagic, sizeof(magic)) != 0
        || version != WG_CONFIG_VERSION
        || stored_variant != (uint8_t)variant
        || !WG_ConfigValid(&decoded))
    {
        return 0;
    }
    *config = decoded;
    return 1;
}

int WG_ConfigReadFile(const char *path, wg_game_variant_t variant,
                      wg_config_t *config)
{
    wg_file_buffer_t file;
    int result;

    memset(&file, 0, sizeof(file));
    if (!WG_LoadFile(path, &file))
    {
        return 0;
    }
    result = WG_ConfigDecode(file.data, file.size, variant, config);
    WG_FreeFile(&file);
    return result;
}

int WG_ConfigWriteFile(const char *path, wg_game_variant_t variant,
                       const wg_config_t *config)
{
    uint8_t data[WG_CONFIG_BUFFER_SIZE];
    size_t size;

    return WG_ConfigEncode(data, sizeof(data), &size, variant, config)
        && WG_WriteFile(path, data, size);
}
