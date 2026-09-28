/* Portable save-game encoding replacing the original raw DOS structure dump. */
#include "WG_SAVE.h"

#include <stdlib.h>
#include <string.h>

#include "WG_ENDIAN.h"
#include "WG_FILE.h"

#define WG_SAVE_VERSION 3U

static const uint8_t WG_SaveMagic[8] =
    { 'W', '3', 'D', 'G', 'S', 'A', 'V', 'E' };

typedef struct wg_save_writer
{
    uint8_t *data;
    size_t capacity;
    size_t position;
    int valid;
} wg_save_writer_t;

typedef struct wg_save_reader
{
    const uint8_t *data;
    size_t size;
    size_t position;
    int valid;
} wg_save_reader_t;

static uint32_t WG_SaveChecksum(const uint8_t *data, size_t size)
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

static void WG_SaveBytes(wg_save_writer_t *writer,
                         const void *source, size_t size)
{
    if (!writer->valid || size > writer->capacity - writer->position)
    {
        writer->valid = 0;
        return;
    }
    memcpy(writer->data + writer->position, source, size);
    writer->position += size;
}

static void WG_SaveU8(wg_save_writer_t *writer, uint8_t value)
{
    WG_SaveBytes(writer, &value, 1U);
}

static void WG_SaveU16(wg_save_writer_t *writer, uint16_t value)
{
    uint8_t encoded[2];

    encoded[0] = (uint8_t)value;
    encoded[1] = (uint8_t)(value >> 8);
    WG_SaveBytes(writer, encoded, sizeof(encoded));
}

static void WG_SaveU32(wg_save_writer_t *writer, uint32_t value)
{
    uint8_t encoded[4];

    encoded[0] = (uint8_t)value;
    encoded[1] = (uint8_t)(value >> 8);
    encoded[2] = (uint8_t)(value >> 16);
    encoded[3] = (uint8_t)(value >> 24);
    WG_SaveBytes(writer, encoded, sizeof(encoded));
}

static void WG_LoadBytes(wg_save_reader_t *reader, void *destination,
                         size_t size)
{
    if (!reader->valid || size > reader->size - reader->position)
    {
        reader->valid = 0;
        if (destination != NULL)
        {
            memset(destination, 0, size);
        }
        return;
    }
    if (destination != NULL)
    {
        memcpy(destination, reader->data + reader->position, size);
    }
    reader->position += size;
}

static uint8_t WG_LoadU8(wg_save_reader_t *reader)
{
    uint8_t value = 0U;
    WG_LoadBytes(reader, &value, 1U);
    return value;
}

static uint16_t WG_LoadU16(wg_save_reader_t *reader)
{
    uint8_t encoded[2];
    WG_LoadBytes(reader, encoded, sizeof(encoded));
    return reader->valid ? WG_ReadLE16(encoded) : 0U;
}

static uint32_t WG_LoadU32(wg_save_reader_t *reader)
{
    uint8_t encoded[4];
    WG_LoadBytes(reader, encoded, sizeof(encoded));
    return reader->valid ? WG_ReadLE32(encoded) : 0U;
}

static void WG_SaveIntermission(wg_save_writer_t *writer,
                                const wl_intermission_t *intermission)
{
    WG_SaveU32(writer, intermission->seconds);
    WG_SaveU32(writer, intermission->bonus);
    WG_SaveU16(writer, intermission->par_seconds);
    WG_SaveU8(writer, intermission->kill_ratio);
    WG_SaveU8(writer, intermission->secret_ratio);
    WG_SaveU8(writer, intermission->treasure_ratio);
    WG_SaveU8(writer, intermission->special_floor);
}

static void WG_LoadIntermission(wg_save_reader_t *reader,
                                wl_intermission_t *intermission)
{
    intermission->seconds = WG_LoadU32(reader);
    intermission->bonus = WG_LoadU32(reader);
    intermission->par_seconds = WG_LoadU16(reader);
    intermission->kill_ratio = WG_LoadU8(reader);
    intermission->secret_ratio = WG_LoadU8(reader);
    intermission->treasure_ratio = WG_LoadU8(reader);
    intermission->special_floor = WG_LoadU8(reader);
}

static void WG_SaveLevel(wg_save_writer_t *writer, const wg_level_t *level)
{
    size_t index;

    WG_SaveBytes(writer, level->tiles, sizeof(level->tiles));
    WG_SaveBytes(writer, level->areas, sizeof(level->areas));
    WG_SaveBytes(writer, level->area_by_player, sizeof(level->area_by_player));
    WG_SaveBytes(writer, level->ambush_tiles, sizeof(level->ambush_tiles));
    for (index = 0U; index < WG_LEVEL_SIZE * WG_LEVEL_SIZE; ++index)
    {
        WG_SaveU16(writer, level->info[index]);
    }
    WG_SaveU32(writer, (uint32_t)level->player_x);
    WG_SaveU32(writer, (uint32_t)level->player_y);
    WG_SaveU32(writer, (uint32_t)level->player_angle_fraction);
    WG_SaveU16(writer, level->player_angle);
    WG_SaveU8(writer, level->player_tile_x);
    WG_SaveU8(writer, level->player_tile_y);
    WG_SaveU8(writer, (uint8_t)level->difficulty);
    WG_SaveU32(writer, (uint32_t)level->player_thrust_speed);
    WG_SaveU16(writer, level->player_health);
    WG_SaveU16(writer, level->player_ammo);
    WG_SaveU8(writer, level->player_keys);
    WG_SaveU8(writer, level->player_lives);
    WG_SaveU16(writer, level->damage_count);
    WG_SaveU16(writer, level->bonus_count);
    WG_SaveU8(writer, level->player_dead);
    WG_SaveU32(writer, (uint32_t)level->killer_x);
    WG_SaveU32(writer, (uint32_t)level->killer_y);
    WG_SaveU8(writer, level->player_weapon);
    WG_SaveU8(writer, level->player_chosen_weapon);
    WG_SaveU8(writer, level->player_best_weapon);
    WG_SaveU8(writer, level->weapon_frame);
    WG_SaveU8(writer, level->attack_frame);
    WG_SaveU8(writer, level->attack_active);
    WG_SaveU32(writer, (uint32_t)level->attack_count);
    WG_SaveU8(writer, level->made_noise);
    WG_SaveU16(writer, level->view_width);
    WG_SaveU8(writer, level->victory_flag);
    WG_SaveU8(writer, level->level_completed);
    WG_SaveU8(writer, level->secret_level);
    WG_SaveU8(writer, level->shareware);
    WG_SaveU8(writer, level->map_number);
    WG_SaveU32(writer, level->score);
    WG_SaveU32(writer, level->next_extra);
    WG_SaveU32(writer, level->time_count);
    WG_SaveU16(writer, level->kill_count);
    WG_SaveU16(writer, level->kill_total);
    WG_SaveU16(writer, level->treasure_count);
    WG_SaveU16(writer, level->treasure_total);
    WG_SaveU32(writer, (uint32_t)level->kill_x);
    WG_SaveU32(writer, (uint32_t)level->kill_y);
    WG_SaveU16(writer, level->secret_count);
    WG_SaveU16(writer, level->secret_total);
    WG_SaveU16(writer, level->pushwall_state);
    WG_SaveU8(writer, level->pushwall_position);
    WG_SaveU8(writer, level->pushwall_x);
    WG_SaveU8(writer, level->pushwall_y);
    WG_SaveU8(writer, level->pushwall_direction);
    WG_SaveU8(writer, level->door_count);
    for (index = 0U; index < level->door_count; ++index)
    {
        const wg_door_t *door = &level->doors[index];
        WG_SaveU16(writer, door->position);
        WG_SaveU16(writer, door->tic_count);
        WG_SaveU8(writer, door->tile_x);
        WG_SaveU8(writer, door->tile_y);
        WG_SaveU8(writer, door->vertical);
        WG_SaveU8(writer, (uint8_t)door->lock);
        WG_SaveU8(writer, (uint8_t)door->action);
    }
    WG_SaveU16(writer, level->static_count);
    for (index = 0U; index < level->static_count; ++index)
    {
        const wg_static_object_t *object = &level->statics[index];
        WG_SaveU8(writer, object->tile_x);
        WG_SaveU8(writer, object->tile_y);
        WG_SaveU8(writer, object->blocking);
        WG_SaveU8(writer, object->removed);
        WG_SaveU16(writer, object->shape);
        WG_SaveU8(writer, (uint8_t)object->item);
    }
    WG_SaveU16(writer, level->actor_count);
    for (index = 0U; index < level->actor_count; ++index)
    {
        const wg_actor_t *actor = &level->actors[index];
        WG_SaveU32(writer, (uint32_t)actor->x);
        WG_SaveU32(writer, (uint32_t)actor->y);
        WG_SaveU16(writer, actor->shape);
        WG_SaveU8(writer, actor->tile_x);
        WG_SaveU8(writer, actor->tile_y);
        WG_SaveU8(writer, actor->direction);
        WG_SaveU8(writer, actor->rotate);
        WG_SaveU8(writer, actor->area_number);
        WG_SaveU16(writer, actor->angle);
        WG_SaveU16(writer, actor->base_shape);
        WG_SaveU16(writer, actor->attack_shape);
        WG_SaveU16(writer, actor->flags);
        WG_SaveU32(writer, (uint32_t)actor->tic_count);
        WG_SaveU32(writer, (uint32_t)actor->reaction_time);
        WG_SaveU32(writer, (uint32_t)actor->speed);
        WG_SaveU32(writer, (uint32_t)actor->distance);
        WG_SaveU32(writer, (uint32_t)actor->hit_points);
        WG_SaveU32(writer, (uint32_t)actor->view_x);
        WG_SaveU32(writer, (uint32_t)actor->trans_x);
        WG_SaveU8(writer, (uint8_t)actor->state);
        WG_SaveU8(writer, (uint8_t)actor->actor_class);
    }
    for (index = 0U; index < WG_LEVEL_SIZE * WG_LEVEL_SIZE; ++index)
    {
        WG_SaveU16(writer, level->actor_at[index]);
    }
    WG_SaveU8(writer, level->random.index);
}

static void WG_LoadLevel(wg_save_reader_t *reader, wg_level_t *level,
                         uint16_t version)
{
    size_t index;

    memset(level, 0, sizeof(*level));
    WG_LoadBytes(reader, level->tiles, sizeof(level->tiles));
    WG_LoadBytes(reader, level->areas, sizeof(level->areas));
    WG_LoadBytes(reader, level->area_by_player, sizeof(level->area_by_player));
    WG_LoadBytes(reader, level->ambush_tiles, sizeof(level->ambush_tiles));
    for (index = 0U; index < WG_LEVEL_SIZE * WG_LEVEL_SIZE; ++index)
    {
        level->info[index] = WG_LoadU16(reader);
    }
    level->player_x = (int32_t)WG_LoadU32(reader);
    level->player_y = (int32_t)WG_LoadU32(reader);
    level->player_angle_fraction = (int32_t)WG_LoadU32(reader);
    level->player_angle = WG_LoadU16(reader);
    level->player_tile_x = WG_LoadU8(reader);
    level->player_tile_y = WG_LoadU8(reader);
    level->difficulty = (wg_difficulty_t)WG_LoadU8(reader);
    level->player_thrust_speed = (int32_t)WG_LoadU32(reader);
    level->player_health = WG_LoadU16(reader);
    level->player_ammo = WG_LoadU16(reader);
    level->player_keys = WG_LoadU8(reader);
    level->player_lives = WG_LoadU8(reader);
    level->damage_count = WG_LoadU16(reader);
    level->bonus_count = WG_LoadU16(reader);
    level->player_dead = WG_LoadU8(reader);
    level->killer_x = (int32_t)WG_LoadU32(reader);
    level->killer_y = (int32_t)WG_LoadU32(reader);
    level->player_weapon = WG_LoadU8(reader);
    level->player_chosen_weapon = WG_LoadU8(reader);
    level->player_best_weapon = WG_LoadU8(reader);
    level->weapon_frame = WG_LoadU8(reader);
    level->attack_frame = WG_LoadU8(reader);
    level->attack_active = WG_LoadU8(reader);
    level->attack_count = (int32_t)WG_LoadU32(reader);
    level->made_noise = WG_LoadU8(reader);
    level->view_width = WG_LoadU16(reader);
    level->victory_flag = WG_LoadU8(reader);
    level->level_completed = WG_LoadU8(reader);
    level->secret_level = WG_LoadU8(reader);
    level->shareware = WG_LoadU8(reader);
    level->map_number = WG_LoadU8(reader);
    level->score = WG_LoadU32(reader);
    level->next_extra = WG_LoadU32(reader);
    level->time_count = WG_LoadU32(reader);
    level->kill_count = WG_LoadU16(reader);
    level->kill_total = WG_LoadU16(reader);
    level->treasure_count = WG_LoadU16(reader);
    level->treasure_total = WG_LoadU16(reader);
    level->kill_x = (int32_t)WG_LoadU32(reader);
    level->kill_y = (int32_t)WG_LoadU32(reader);
    level->secret_count = WG_LoadU16(reader);
    level->secret_total = WG_LoadU16(reader);
    level->pushwall_state = WG_LoadU16(reader);
    level->pushwall_position = WG_LoadU8(reader);
    level->pushwall_x = WG_LoadU8(reader);
    level->pushwall_y = WG_LoadU8(reader);
    level->pushwall_direction = WG_LoadU8(reader);
    level->door_count = WG_LoadU8(reader);
    if (level->door_count > WG_MAX_DOORS)
    {
        reader->valid = 0;
        return;
    }
    for (index = 0U; index < level->door_count; ++index)
    {
        wg_door_t *door = &level->doors[index];
        door->position = WG_LoadU16(reader);
        door->tic_count = WG_LoadU16(reader);
        door->tile_x = WG_LoadU8(reader);
        door->tile_y = WG_LoadU8(reader);
        door->vertical = WG_LoadU8(reader);
        door->lock = (wg_door_lock_t)WG_LoadU8(reader);
        door->action = (wg_door_action_t)WG_LoadU8(reader);
    }
    level->static_count = WG_LoadU16(reader);
    if (level->static_count > WG_MAX_STATICS)
    {
        reader->valid = 0;
        return;
    }
    for (index = 0U; index < level->static_count; ++index)
    {
        wg_static_object_t *object = &level->statics[index];
        object->tile_x = WG_LoadU8(reader);
        object->tile_y = WG_LoadU8(reader);
        object->blocking = WG_LoadU8(reader);
        object->removed = WG_LoadU8(reader);
        object->shape = WG_LoadU16(reader);
        object->item = (wg_item_type_t)WG_LoadU8(reader);
    }
    level->actor_count = WG_LoadU16(reader);
    if (level->actor_count > WG_MAX_ACTORS)
    {
        reader->valid = 0;
        return;
    }
    for (index = 0U; index < level->actor_count; ++index)
    {
        wg_actor_t *actor = &level->actors[index];
        actor->x = (int32_t)WG_LoadU32(reader);
        actor->y = (int32_t)WG_LoadU32(reader);
        actor->shape = WG_LoadU16(reader);
        actor->tile_x = WG_LoadU8(reader);
        actor->tile_y = WG_LoadU8(reader);
        actor->direction = WG_LoadU8(reader);
        actor->rotate = WG_LoadU8(reader);
        actor->area_number = WG_LoadU8(reader);
        actor->angle = WG_LoadU16(reader);
        actor->base_shape = WG_LoadU16(reader);
        actor->attack_shape = WG_LoadU16(reader);
        actor->flags = WG_LoadU16(reader);
        actor->tic_count = (int32_t)WG_LoadU32(reader);
        actor->reaction_time = (int32_t)WG_LoadU32(reader);
        actor->speed = (int32_t)WG_LoadU32(reader);
        actor->distance = (int32_t)WG_LoadU32(reader);
        actor->hit_points = (int32_t)WG_LoadU32(reader);
        actor->view_x = (int32_t)WG_LoadU32(reader);
        actor->trans_x = (int32_t)WG_LoadU32(reader);
        actor->state = (wg_actor_state_t)WG_LoadU8(reader);
        actor->actor_class = (wg_actor_class_t)WG_LoadU8(reader);
        if (actor->tile_x >= WG_LEVEL_SIZE
            || actor->tile_y >= WG_LEVEL_SIZE)
        {
            reader->valid = 0;
        }
    }
    if (!reader->valid)
    {
        return;
    }
    if (version >= 3U)
    {
        for (index = 0U; index < WG_LEVEL_SIZE * WG_LEVEL_SIZE; ++index)
        {
            level->actor_at[index] = WG_LoadU16(reader);
        }
    }
    else
    {
        /* Older portable saves did not retain actorat[]'s stale entries.
           Exact stale pointers cannot be recovered, but reconstruct the
           original wall/door tokens and live actor marks so collision and
           pathing remain correct after loading them. */
        for (index = 0U; index < WG_LEVEL_SIZE * WG_LEVEL_SIZE; ++index)
        {
            uint8_t tile = level->tiles[index];

            if ((tile & 0x80U) != 0U)
            {
                size_t door_index = tile & 0x3fU;

                if (door_index < level->door_count
                    && level->doors[door_index].action != WG_DOOR_OPEN)
                {
                    level->actor_at[index] = tile;
                }
            }
            else if (tile != 0U)
            {
                level->actor_at[index] = (uint16_t)(tile & 0x3fU);
            }
        }
        for (index = 0U; index < level->actor_count; ++index)
        {
            const wg_actor_t *actor = &level->actors[index];
            size_t spot;

            if ((actor->flags & (WG_ACTOR_FLAG_REMOVED
                                 | WG_ACTOR_FLAG_NEVERMARK)) == 0U)
            {
                spot = (size_t)actor->tile_y * WG_LEVEL_SIZE
                       + actor->tile_x;
                if ((actor->flags & WG_ACTOR_FLAG_NONMARK) == 0U
                    || level->actor_at[spot] == 0U)
                {
                    level->actor_at[spot]
                        = (uint16_t)(WG_ACTOR_AT_ACTOR_BASE + index);
                }
            }
        }
    }
    level->random.index = WG_LoadU8(reader);
    level->sound_event_count = 0U;
}

int WG_SaveEncode(uint8_t *data, size_t capacity, size_t *size,
                  const char name[WG_SAVE_NAME_BYTES],
                  wg_game_variant_t variant,
                  const wg_save_state_t *state)
{
    wg_save_writer_t writer;
    unsigned index;
    uint32_t checksum;

    if (data == NULL || size == NULL || name == NULL || state == NULL
        || capacity < WG_SAVE_NAME_BYTES + sizeof(WG_SaveMagic) + 8U
        || state->map_number > UINT8_MAX
        || state->level.door_count > WG_MAX_DOORS
        || state->level.static_count > WG_MAX_STATICS
        || state->level.actor_count > WG_MAX_ACTORS)
    {
        return 0;
    }
    writer.data = data;
    writer.capacity = capacity;
    writer.position = 0U;
    writer.valid = 1;
    WG_SaveBytes(&writer, name, WG_SAVE_NAME_BYTES);
    WG_SaveBytes(&writer, WG_SaveMagic, sizeof(WG_SaveMagic));
    WG_SaveU16(&writer, WG_SAVE_VERSION);
    WG_SaveU8(&writer, (uint8_t)variant);
    WG_SaveU8(&writer, (uint8_t)state->map_number);
    WG_SaveU32(&writer, state->level_start_score);
    for (index = 0U; index < WL_MAX_LEVEL_RATIOS; ++index)
    {
        WG_SaveIntermission(&writer, &state->level_ratios[index]);
    }
    WG_SaveLevel(&writer, &state->level);
    if (!writer.valid || writer.capacity - writer.position < 4U)
    {
        return 0;
    }
    checksum = WG_SaveChecksum(data + WG_SAVE_NAME_BYTES,
                               writer.position - WG_SAVE_NAME_BYTES);
    WG_SaveU32(&writer, checksum);
    if (!writer.valid)
    {
        return 0;
    }
    *size = writer.position;
    return 1;
}

int WG_SaveReadName(const uint8_t *data, size_t size,
                    wg_game_variant_t variant,
                    char name[WG_SAVE_NAME_BYTES])
{
    uint16_t version;

    if (data == NULL
        || size < WG_SAVE_NAME_BYTES + sizeof(WG_SaveMagic) + 4U)
    {
        return 0;
    }
    version = WG_ReadLE16(data + WG_SAVE_NAME_BYTES
                          + sizeof(WG_SaveMagic));
    if (name == NULL
        || memcmp(data + WG_SAVE_NAME_BYTES, WG_SaveMagic,
                  sizeof(WG_SaveMagic)) != 0
        || (version < 1U || version > WG_SAVE_VERSION)
        || data[WG_SAVE_NAME_BYTES + sizeof(WG_SaveMagic) + 2U]
               != (uint8_t)variant)
    {
        return 0;
    }
    memcpy(name, data, WG_SAVE_NAME_BYTES);
    name[WG_SAVE_NAME_BYTES - 1U] = '\0';
    return 1;
}

int WG_SaveDecode(const uint8_t *data, size_t size,
                  wg_game_variant_t variant,
                  char name[WG_SAVE_NAME_BYTES],
                  wg_save_state_t *state)
{
    wg_save_reader_t reader;
    uint8_t magic[sizeof(WG_SaveMagic)];
    uint16_t version;
    uint8_t stored_variant;
    uint32_t stored_checksum;
    uint32_t calculated_checksum;
    unsigned index;

    if (data == NULL || name == NULL || state == NULL
        || size < WG_SAVE_NAME_BYTES + sizeof(WG_SaveMagic) + 12U)
    {
        return 0;
    }
    stored_checksum = WG_ReadLE32(data + size - 4U);
    calculated_checksum = WG_SaveChecksum(
        data + WG_SAVE_NAME_BYTES,
        size - WG_SAVE_NAME_BYTES - 4U);
    if (stored_checksum != calculated_checksum)
    {
        return 0;
    }
    memset(state, 0, sizeof(*state));
    reader.data = data;
    reader.size = size - 4U;
    reader.position = 0U;
    reader.valid = 1;
    WG_LoadBytes(&reader, name, WG_SAVE_NAME_BYTES);
    name[WG_SAVE_NAME_BYTES - 1U] = '\0';
    WG_LoadBytes(&reader, magic, sizeof(magic));
    version = WG_LoadU16(&reader);
    stored_variant = WG_LoadU8(&reader);
    state->map_number = WG_LoadU8(&reader);
    state->level_start_score = WG_LoadU32(&reader);
    if (!reader.valid || memcmp(magic, WG_SaveMagic, sizeof(magic)) != 0
        || (version < 1U || version > WG_SAVE_VERSION)
        || stored_variant != (uint8_t)variant)
    {
        return 0;
    }
    for (index = 0U; index < (version == 1U ? 8U : WL_MAX_LEVEL_RATIOS);
         ++index)
    {
        WG_LoadIntermission(&reader, &state->level_ratios[index]);
    }
    WG_LoadLevel(&reader, &state->level, version);
    return reader.valid && reader.position == reader.size
        && state->level.map_number == state->map_number
        && state->level.difficulty <= WG_DIFFICULTY_HARD;
}

int WG_SaveWriteFile(const char *path,
                     const char name[WG_SAVE_NAME_BYTES],
                     wg_game_variant_t variant,
                     const wg_save_state_t *state)
{
    uint8_t *data;
    size_t size;
    int result;

    data = (uint8_t *)malloc(WG_SAVE_BUFFER_SIZE);
    if (data == NULL)
    {
        return 0;
    }
    result = WG_SaveEncode(data, WG_SAVE_BUFFER_SIZE, &size,
                           name, variant, state)
          && WG_WriteFile(path, data, size);
    free(data);
    return result;
}

int WG_SaveReadFile(const char *path, wg_game_variant_t variant,
                    char name[WG_SAVE_NAME_BYTES],
                    wg_save_state_t *state)
{
    wg_file_buffer_t file;
    int result;

    memset(&file, 0, sizeof(file));
    if (!WG_LoadFile(path, &file))
    {
        return 0;
    }
    result = WG_SaveDecode(file.data, file.size, variant, name, state);
    WG_FreeFile(&file);
    return result;
}
