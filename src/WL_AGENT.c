/* Portable player movement, interaction, combat, and status from WL_AGENT.C. */
#include "WL_AGENT.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "WG_FIXED.h"
#include "WL_ACT1.h"
#include "WL_GAME.h"
#include "WL_MAIN.h"
#include "WL_STATE.h"

enum
{
    WL_STATUS_HEIGHT = 40,
    WL_STATUS_Y = WG_VIDEO_HEIGHT - WL_STATUS_HEIGHT,
    WL_MOVE_SCALE = 150,
    WL_BACK_MOVE_SCALE = 100,
    WL_ANGLE_SCALE = 20
};

#define WL_PLAYER_SIZE WG_MIN_DISTANCE
#define WL_MIN_ACTOR_DISTANCE WG_FIXED_ONE

typedef struct wl_status_chunks
{
    size_t status_bar;
    size_t knife;
    size_t no_key;
    size_t blank_digit;
    size_t zero_digit;
    size_t face_1a;
} wl_status_chunks_t;

static int WL_StatusChunks(wg_game_variant_t variant,
                           wl_status_chunks_t *chunks)
{
    size_t status_bar;

    if (chunks == NULL)
    {
        return 0;
    }
    if (variant == WG_GAME_WOLF3D_FULL_GT_14)
    {
        status_bar = 86U;
    }
    else if (variant == WG_GAME_WOLF3D_SHAREWARE_14)
    {
        /* The supplied Apogee v1.4 WL1 graph is four chunks later than the
           early generated GFXE_WL1.H retained in the source release. */
        status_bar = 98U;
    }
    else
    {
        return 0;
    }
    chunks->status_bar = status_bar;
    chunks->knife = status_bar + 5U;
    chunks->no_key = status_bar + 9U;
    chunks->blank_digit = status_bar + 12U;
    chunks->zero_digit = status_bar + 13U;
    chunks->face_1a = status_bar + 23U;
    return 1;
}

static int WL_DrawPicture(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    const wg_graphics_t *graphics, size_t chunk, int x, int y)
{
    uint8_t *pixels = NULL;
    uint16_t width;
    uint16_t height;
    int result;

    if (!WG_GraphicsDecodePicture(graphics, chunk, &pixels, &width, &height))
    {
        return 0;
    }
    result = x >= 0 && y >= 0
             && x + width <= WG_VIDEO_WIDTH
             && y + height <= WG_VIDEO_HEIGHT;
    if (result)
    {
        WG_VideoBlit(framebuffer, x, y, pixels, width, height);
    }
    free(pixels);
    return result;
}

static int WL_StatusDrawPicture(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    const wg_graphics_t *graphics, size_t chunk, int x, int y)
{
    return WL_DrawPicture(framebuffer, graphics, chunk, x * 8, WL_STATUS_Y + y);
}

static int WL_DrawNumber(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    const wg_graphics_t *graphics, const wl_status_chunks_t *chunks,
    int x, int y, unsigned width, uint32_t number)
{
    char digits[16];
    int length;
    unsigned first;
    unsigned index;

    length = snprintf(digits, sizeof(digits), "%lu", (unsigned long)number);
    if (length < 0 || (size_t)length >= sizeof(digits))
    {
        return 0;
    }
    while ((unsigned)length < width)
    {
        if (!WL_StatusDrawPicture(framebuffer, graphics,
                                  chunks->blank_digit, x++, y))
        {
            return 0;
        }
        --width;
    }
    first = (unsigned)length <= width ? 0U : (unsigned)length - width;
    for (index = first; index < (unsigned)length; ++index)
    {
        if (!WL_StatusDrawPicture(framebuffer, graphics,
                                  chunks->zero_digit
                                  + (unsigned)(digits[index] - '0'),
                                  x++, y))
        {
            return 0;
        }
    }
    return 1;
}

void WL_StatusDefaults(wl_status_t *status)
{
    if (status == NULL)
    {
        return;
    }
    memset(status, 0, sizeof(*status));
    status->health = 100;
    status->ammo = 8;
    status->lives = 3;
    status->weapon = 1;
}

void WL_TakeDamage(struct wg_level *level, unsigned points)
{
    unsigned damage;

    if (level == NULL || level->player_dead)
    {
        return;
    }
    damage = level->difficulty == WG_DIFFICULTY_BABY ? points / 4U : points;
    if (damage >= level->player_health)
    {
        level->player_health = 0U;
        level->player_dead = 1U;
    }
    else
    {
        level->player_health = (uint16_t)(level->player_health - damage);
    }
    if (damage > (unsigned)UINT16_MAX - level->damage_count)
    {
        level->damage_count = UINT16_MAX;
    }
    else
    {
        level->damage_count = (uint16_t)(level->damage_count + damage);
    }
}

static void WL_HealSelf(wg_level_t *level, unsigned points)
{
    unsigned health = level->player_health + points;

    level->player_health = (uint16_t)(health > 100U ? 100U : health);
}

static void WL_GiveExtraMan(wg_level_t *level)
{
    if (level->player_lives < 9U)
    {
        ++level->player_lives;
    }
}

void WL_GivePoints(struct wg_level *level, uint32_t points)
{
    if (level == NULL)
    {
        return;
    }
    if (level->next_extra == 0U)
    {
        level->next_extra = 40000U;
    }
    if (UINT32_MAX - level->score < points)
    {
        level->score = UINT32_MAX;
    }
    else
    {
        level->score += points;
    }
    while (level->score >= level->next_extra)
    {
        if (UINT32_MAX - level->next_extra < 40000U)
        {
            level->next_extra = UINT32_MAX;
        }
        else
        {
            level->next_extra += 40000U;
        }
        WL_GiveExtraMan(level);
        if (level->next_extra == UINT32_MAX)
        {
            break;
        }
    }
}

static void WL_GiveAmmo(wg_level_t *level, unsigned ammo)
{
    unsigned total;

    if (level->player_ammo == 0U && level->attack_frame == 0U)
    {
        level->player_weapon = level->player_chosen_weapon;
    }
    total = level->player_ammo + ammo;
    level->player_ammo = (uint16_t)(total > 99U ? 99U : total);
}

static void WL_GiveWeapon(wg_level_t *level, wg_weapon_t weapon)
{
    WL_GiveAmmo(level, 6U);
    if (level->player_best_weapon < (uint8_t)weapon)
    {
        level->player_best_weapon = (uint8_t)weapon;
        level->player_weapon = (uint8_t)weapon;
        level->player_chosen_weapon = (uint8_t)weapon;
    }
}

int WL_GetBonus(struct wg_level *level, size_t static_index)
{
    wg_static_object_t *object;

    if (level == NULL || static_index >= level->static_count)
    {
        return 0;
    }
    object = &level->statics[static_index];
    if (object->removed != 0U || object->item == WG_ITEM_NONE)
    {
        return 0;
    }
    switch (object->item)
    {
    case WG_ITEM_FIRSTAID:
        if (level->player_health == 100U)
        {
            return 0;
        }
        WL_HealSelf(level, 25U);
        break;
    case WG_ITEM_KEY1:
    case WG_ITEM_KEY2:
        level->player_keys |= (uint8_t)(1U << (object->item - WG_ITEM_KEY1));
        break;
    case WG_ITEM_CROSS:
        WL_GivePoints(level, 100U);
        ++level->treasure_count;
        break;
    case WG_ITEM_CHALICE:
        WL_GivePoints(level, 500U);
        ++level->treasure_count;
        break;
    case WG_ITEM_BIBLE:
        WL_GivePoints(level, 1000U);
        ++level->treasure_count;
        break;
    case WG_ITEM_CROWN:
        WL_GivePoints(level, 5000U);
        ++level->treasure_count;
        break;
    case WG_ITEM_CLIP:
        if (level->player_ammo == 99U)
        {
            return 0;
        }
        WL_GiveAmmo(level, 8U);
        break;
    case WG_ITEM_CLIP2:
        if (level->player_ammo == 99U)
        {
            return 0;
        }
        WL_GiveAmmo(level, 4U);
        break;
    case WG_ITEM_MACHINEGUN:
        WL_GiveWeapon(level, WG_WEAPON_MACHINEGUN);
        break;
    case WG_ITEM_CHAINGUN:
        WL_GiveWeapon(level, WG_WEAPON_CHAINGUN);
        break;
    case WG_ITEM_FULLHEAL:
        WL_HealSelf(level, 99U);
        WL_GiveAmmo(level, 25U);
        WL_GiveExtraMan(level);
        ++level->treasure_count;
        break;
    case WG_ITEM_FOOD:
        if (level->player_health == 100U)
        {
            return 0;
        }
        WL_HealSelf(level, 10U);
        break;
    case WG_ITEM_ALPO:
        if (level->player_health == 100U)
        {
            return 0;
        }
        WL_HealSelf(level, 4U);
        break;
    case WG_ITEM_GIBS:
        if (level->player_health > 10U)
        {
            return 0;
        }
        WL_HealSelf(level, 1U);
        break;
    default:
        return 0;
    }
    level->bonus_count = 18U;
    object->removed = 1U;
    object->blocking = 0U;
    return 1;
}

unsigned WL_CollectPlayerTileBonuses(struct wg_level *level)
{
    size_t index;
    unsigned collected = 0U;

    if (level == NULL)
    {
        return 0U;
    }
    for (index = 0U; index < level->static_count; ++index)
    {
        const wg_static_object_t *object = &level->statics[index];

        if (object->tile_x == level->player_tile_x
            && object->tile_y == level->player_tile_y
            && WL_GetBonus(level, index))
        {
            ++collected;
        }
    }
    return collected;
}

static int WL_TileBlocksPlayer(const wg_level_t *level, int tile_x,
                               int tile_y)
{
    uint8_t tile;

    if (tile_x < 0 || tile_x >= WG_LEVEL_SIZE
        || tile_y < 0 || tile_y >= WG_LEVEL_SIZE)
    {
        return 1;
    }
    tile = level->tiles[(size_t)tile_y * WG_LEVEL_SIZE + (size_t)tile_x];
    if (tile == 0U)
    {
        return 0;
    }
    if ((tile & 0x80U) != 0U)
    {
        size_t door_index = tile & 0x3fU;

        return door_index >= level->door_count
               || level->doors[door_index].position != 0xffffU;
    }
    return 1;
}

static int WL_FixedTile(int32_t value)
{
    if (value >= 0)
    {
        return value / WG_FIXED_ONE;
    }
    return -(int)((-(int64_t)value + WG_FIXED_ONE - 1) / WG_FIXED_ONE);
}

int WL_TryMove(const struct wg_level *level, int32_t x, int32_t y)
{
    int low_x;
    int low_y;
    int high_x;
    int high_y;
    int tile_x;
    int tile_y;
    size_t index;

    if (level == NULL)
    {
        return 0;
    }
    low_x = WL_FixedTile(x - WL_PLAYER_SIZE);
    low_y = WL_FixedTile(y - WL_PLAYER_SIZE);
    high_x = WL_FixedTile(x + WL_PLAYER_SIZE);
    high_y = WL_FixedTile(y + WL_PLAYER_SIZE);
    for (tile_y = low_y; tile_y <= high_y; ++tile_y)
    {
        for (tile_x = low_x; tile_x <= high_x; ++tile_x)
        {
            if (WL_TileBlocksPlayer(level, tile_x, tile_y))
            {
                return 0;
            }
        }
    }
    for (index = 0U; index < level->static_count; ++index)
    {
        const wg_static_object_t *object = &level->statics[index];

        if (object->removed == 0U && object->blocking != 0U
            && object->tile_x >= low_x && object->tile_x <= high_x
            && object->tile_y >= low_y && object->tile_y <= high_y)
        {
            return 0;
        }
    }
    for (index = 0U; index < level->actor_count; ++index)
    {
        const wg_actor_t *actor = &level->actors[index];
        int32_t delta_x;
        int32_t delta_y;

        if ((actor->flags & WG_ACTOR_FLAG_SHOOTABLE) == 0U
            || (actor->flags & WG_ACTOR_FLAG_REMOVED) != 0U)
        {
            continue;
        }
        delta_x = x - actor->x;
        delta_y = y - actor->y;
        if (delta_x >= -WL_MIN_ACTOR_DISTANCE
            && delta_x <= WL_MIN_ACTOR_DISTANCE
            && delta_y >= -WL_MIN_ACTOR_DISTANCE
            && delta_y <= WL_MIN_ACTOR_DISTANCE)
        {
            return 0;
        }
    }
    return 1;
}

int WL_ClipMove(struct wg_level *level, int32_t x_move, int32_t y_move)
{
    int32_t base_x;
    int32_t base_y;

    if (level == NULL)
    {
        return 0;
    }
    base_x = level->player_x;
    base_y = level->player_y;
    if (WL_TryMove(level, base_x + x_move, base_y + y_move))
    {
        level->player_x = base_x + x_move;
        level->player_y = base_y + y_move;
        return 1;
    }
    if (WL_TryMove(level, base_x + x_move, base_y))
    {
        level->player_x = base_x + x_move;
        return 1;
    }
    if (WL_TryMove(level, base_x, base_y + y_move))
    {
        level->player_y = base_y + y_move;
        return 1;
    }
    return 1;
}

int WL_Thrust(struct wg_level *level, const struct wg_view_tables *tables,
              uint16_t angle, int32_t speed)
{
    const int32_t *cosine;
    int32_t move_speed;
    int32_t x_move;
    int32_t y_move;
    size_t tile_index;

    if (level == NULL || tables == NULL || angle >= WG_ANGLES || speed < 0)
    {
        return 0;
    }
    if (INT32_MAX - level->player_thrust_speed < speed)
    {
        level->player_thrust_speed = INT32_MAX;
    }
    else
    {
        level->player_thrust_speed += speed;
    }
    move_speed = speed >= WG_MIN_DISTANCE * 2
                     ? WG_MIN_DISTANCE * 2 - 1 : speed;
    cosine = WG_ViewCosineTable(tables);
    x_move = WG_FixedMul(move_speed, cosine[angle]);
    y_move = -WG_FixedMul(move_speed, tables->sine[angle]);
    if (!WL_ClipMove(level, x_move, y_move))
    {
        return 0;
    }
    level->player_tile_x = (uint8_t)WL_FixedTile(level->player_x);
    level->player_tile_y = (uint8_t)WL_FixedTile(level->player_y);
    tile_index = (size_t)level->player_tile_y * WG_LEVEL_SIZE
                 + level->player_tile_x;
    if (level->areas[tile_index] < WG_NUM_AREAS)
    {
        (void)WL_UpdateAreaConnectivity(level);
    }
    if (level->info[tile_index] == 99U)
    {
        level->victory_flag = 1U;
    }
    (void)WL_CollectPlayerTileBonuses(level);
    return 1;
}

static int32_t WL_ScaledControl(int control, int scale)
{
    int64_t magnitude = control < 0 ? -(int64_t)control : control;
    int64_t speed = magnitude * scale;

    return speed > INT32_MAX ? INT32_MAX : (int32_t)speed;
}

int WL_ControlMovement(struct wg_level *level,
                       const struct wg_view_tables *tables,
                       int control_x, int control_y, int strafe)
{
    int angle;

    if (level == NULL || tables == NULL)
    {
        return 0;
    }
    level->player_thrust_speed = 0;
    if (strafe != 0)
    {
        if (control_x > 0)
        {
            angle = (int)level->player_angle - WG_ANGLE_QUADRANT;
            if (angle < 0)
            {
                angle += WG_ANGLES;
            }
            if (!WL_Thrust(level, tables, (uint16_t)angle,
                           WL_ScaledControl(control_x, WL_MOVE_SCALE)))
            {
                return 0;
            }
        }
        else if (control_x < 0)
        {
            angle = (int)level->player_angle + WG_ANGLE_QUADRANT;
            if (angle >= WG_ANGLES)
            {
                angle -= WG_ANGLES;
            }
            if (!WL_Thrust(level, tables, (uint16_t)angle,
                           WL_ScaledControl(control_x, WL_MOVE_SCALE)))
            {
                return 0;
            }
        }
    }
    else
    {
        int angle_units;

        level->player_angle_fraction += control_x;
        angle_units = level->player_angle_fraction / WL_ANGLE_SCALE;
        level->player_angle_fraction -= angle_units * WL_ANGLE_SCALE;
        angle = (int)level->player_angle - angle_units;
        while (angle >= WG_ANGLES)
        {
            angle -= WG_ANGLES;
        }
        while (angle < 0)
        {
            angle += WG_ANGLES;
        }
        level->player_angle = (uint16_t)angle;
    }
    if (control_y < 0
        && !WL_Thrust(level, tables, level->player_angle,
                      WL_ScaledControl(control_y, WL_MOVE_SCALE)))
    {
        return 0;
    }
    if (control_y > 0)
    {
        angle = (int)level->player_angle + WG_ANGLES / 2;
        if (angle >= WG_ANGLES)
        {
            angle -= WG_ANGLES;
        }
        if (!WL_Thrust(level, tables, (uint16_t)angle,
                       WL_ScaledControl(control_y, WL_BACK_MOVE_SCALE)))
        {
            return 0;
        }
    }
    return 1;
}

int WL_CmdUse(struct wg_level *level)
{
    int check_x;
    int check_y;
    uint8_t direction;
    int elevator_ok;
    size_t index;
    uint8_t tile;

    if (level == NULL || level->player_angle >= WG_ANGLES)
    {
        return 0;
    }
    if (level->player_angle < WG_ANGLES / 8
        || level->player_angle > 7 * WG_ANGLES / 8)
    {
        check_x = level->player_tile_x + 1;
        check_y = level->player_tile_y;
        direction = 0U;
        elevator_ok = 1;
    }
    else if (level->player_angle < 3 * WG_ANGLES / 8)
    {
        check_x = level->player_tile_x;
        check_y = level->player_tile_y - 1;
        direction = 2U;
        elevator_ok = 0;
    }
    else if (level->player_angle < 5 * WG_ANGLES / 8)
    {
        check_x = level->player_tile_x - 1;
        check_y = level->player_tile_y;
        direction = 4U;
        elevator_ok = 1;
    }
    else
    {
        check_x = level->player_tile_x;
        check_y = level->player_tile_y + 1;
        direction = 6U;
        elevator_ok = 0;
    }
    if (check_x < 0 || check_x >= WG_LEVEL_SIZE
        || check_y < 0 || check_y >= WG_LEVEL_SIZE)
    {
        return 0;
    }
    index = (size_t)check_y * WG_LEVEL_SIZE + (size_t)check_x;
    if (level->info[index] == 98U)
    {
        return WL_PushWall(level, (uint8_t)check_x, (uint8_t)check_y,
                           direction);
    }
    tile = level->tiles[index];
    if (tile == 21U && elevator_ok)
    {
        ++level->tiles[index];
        index = (size_t)level->player_tile_y * WG_LEVEL_SIZE
                + level->player_tile_x;
        level->secret_level = (uint8_t)(level->areas[index] == 0U);
        level->level_completed = 1U;
        return 1;
    }
    if ((tile & 0x80U) != 0U)
    {
        return WL_OperateDoor(level, tile & 0x3fU);
    }
    return 0;
}

static int WL_PlayerAttackTarget(const wg_level_t *level, size_t *target_index,
                                 int knife)
{
    int32_t closest_distance = INT32_MAX;
    size_t closest = 0U;
    size_t index;
    int found = 0;

    for (index = 0U; index < level->actor_count; ++index)
    {
        const wg_actor_t *actor = &level->actors[index];
        int32_t screen_delta = actor->view_x - (WG_VIDEO_WIDTH / 2 - 1);

        if ((actor->flags & (WG_ACTOR_FLAG_SHOOTABLE
                             | WG_ACTOR_FLAG_VISIBLE))
                != (WG_ACTOR_FLAG_SHOOTABLE | WG_ACTOR_FLAG_VISIBLE)
            || screen_delta <= -(WG_VIDEO_WIDTH / 10)
            || screen_delta >= WG_VIDEO_WIDTH / 10
            || actor->trans_x >= closest_distance)
        {
            continue;
        }
        closest_distance = actor->trans_x;
        closest = index;
        found = 1;
    }
    if (!found || (knife && closest_distance > INT32_C(0x18000)))
    {
        return 0;
    }
    *target_index = closest;
    return 1;
}

int WL_KnifeAttack(struct wg_level *level)
{
    size_t target_index;

    if (level == NULL)
    {
        return 0;
    }
    if (WL_PlayerAttackTarget(level, &target_index, 1))
    {
        (void)WL_DamageActor(level, target_index,
                             WG_RandomNext(&level->random) >> 4);
    }
    return 1;
}

int WL_GunAttack(struct wg_level *level)
{
    size_t target_index;
    wg_actor_t *target;
    unsigned distance_x;
    unsigned distance_y;
    unsigned distance;
    unsigned damage;

    if (level == NULL)
    {
        return 0;
    }
    level->made_noise = 1U;
    if (!WL_PlayerAttackTarget(level, &target_index, 0))
    {
        return 1;
    }
    target = &level->actors[target_index];
    if (!WL_CheckLine(level, target))
    {
        return 1;
    }
    distance_x = target->tile_x > level->player_tile_x
                     ? target->tile_x - level->player_tile_x
                     : level->player_tile_x - target->tile_x;
    distance_y = target->tile_y > level->player_tile_y
                     ? target->tile_y - level->player_tile_y
                     : level->player_tile_y - target->tile_y;
    distance = distance_x > distance_y ? distance_x : distance_y;
    if (distance < 2U)
    {
        damage = WG_RandomNext(&level->random) / 4U;
    }
    else if (distance < 4U)
    {
        damage = WG_RandomNext(&level->random) / 6U;
    }
    else
    {
        if (WG_RandomNext(&level->random) / 12U < distance)
        {
            return 1;
        }
        damage = WG_RandomNext(&level->random) / 6U;
    }
    (void)WL_DamageActor(level, target_index, damage);
    return 1;
}

typedef struct wl_attack_info
{
    int8_t tics;
    int8_t attack;
    int8_t frame;
} wl_attack_info_t;

static const wl_attack_info_t wl_attack_info[4][4] =
{
    {{6, 0, 1}, {6, 2, 2}, {6, 0, 3}, {6, -1, 4}},
    {{6, 0, 1}, {6, 1, 2}, {6, 0, 3}, {6, -1, 4}},
    {{6, 0, 1}, {6, 1, 2}, {6, 3, 3}, {6, -1, 4}},
    {{6, 0, 1}, {6, 1, 2}, {6, 4, 3}, {6, -1, 4}}
};

int WL_StartAttack(struct wg_level *level)
{
    if (level == NULL || level->player_dead || level->victory_flag
        || level->attack_active || level->player_weapon > WG_WEAPON_CHAINGUN
        || level->player_chosen_weapon > WG_WEAPON_CHAINGUN)
    {
        return 0;
    }
    level->attack_active = 1U;
    level->attack_frame = 0U;
    level->attack_count = wl_attack_info[level->player_weapon][0].tics;
    level->weapon_frame = (uint8_t)wl_attack_info[level->player_weapon][0].frame;
    return 1;
}

int WL_SelectWeapon(struct wg_level *level, unsigned weapon)
{
    if (level == NULL || weapon > WG_WEAPON_CHAINGUN
        || weapon > level->player_best_weapon || level->player_ammo == 0U
        || level->attack_active || level->player_dead || level->victory_flag)
    {
        return 0;
    }
    level->player_weapon = (uint8_t)weapon;
    level->player_chosen_weapon = (uint8_t)weapon;
    level->weapon_frame = 0U;
    return 1;
}

int WL_TickPlayerAttack(struct wg_level *level, unsigned tics,
                        int attack_held)
{
    if (level == NULL || tics > (unsigned)INT32_MAX)
    {
        return 0;
    }
    if (!level->attack_active)
    {
        return 1;
    }
    level->attack_count -= (int32_t)tics;
    while (level->attack_count <= 0)
    {
        const wl_attack_info_t *current =
            &wl_attack_info[level->player_weapon][level->attack_frame];

        switch (current->attack)
        {
        case -1:
            level->attack_active = 0U;
            level->player_weapon = level->player_ammo == 0U
                                       ? WG_WEAPON_KNIFE
                                       : level->player_chosen_weapon;
            level->attack_frame = 0U;
            level->weapon_frame = 0U;
            return 1;
        case 4:
            if (level->player_ammo == 0U)
            {
                break;
            }
            if (attack_held)
            {
                level->attack_frame = (uint8_t)(level->attack_frame - 2U);
            }
            /* fall through */
        case 1:
            if (level->player_ammo == 0U)
            {
                ++level->attack_frame;
                break;
            }
            if (!WL_GunAttack(level))
            {
                return 0;
            }
            --level->player_ammo;
            break;
        case 2:
            if (!WL_KnifeAttack(level))
            {
                return 0;
            }
            break;
        case 3:
            if (level->player_ammo != 0U && attack_held)
            {
                level->attack_frame = (uint8_t)(level->attack_frame - 2U);
            }
            break;
        default:
            break;
        }
        level->attack_count += current->tics;
        ++level->attack_frame;
        level->weapon_frame = (uint8_t)
            wl_attack_info[level->player_weapon][level->attack_frame].frame;
    }
    return 1;
}

int WL_DrawStatusBar(
    uint8_t framebuffer[WG_VIDEO_WIDTH * WG_VIDEO_HEIGHT],
    const wg_graphics_t *graphics, const wl_status_t *status)
{
    wl_status_chunks_t chunks;
    size_t face;

    if (framebuffer == NULL || graphics == NULL || status == NULL
        || status->health > 100U || status->weapon > 3U
        || status->face_frame > 2U
        || !WL_StatusChunks(graphics->variant, &chunks)
        || !WL_DrawPicture(framebuffer, graphics, chunks.status_bar,
                           0, WL_STATUS_Y))
    {
        return 0;
    }

    face = chunks.face_1a
           + 3U * ((100U - status->health) / 16U)
           + status->face_frame;
    return WL_StatusDrawPicture(framebuffer, graphics, face, 17, 4)
           && WL_DrawNumber(framebuffer, graphics, &chunks,
                            21, 16, 3, status->health)
           && WL_DrawNumber(framebuffer, graphics, &chunks,
                            14, 16, 1, status->lives)
           && WL_DrawNumber(framebuffer, graphics, &chunks,
                            2, 16, 2, (uint32_t)status->map + 1U)
           && WL_DrawNumber(framebuffer, graphics, &chunks,
                            27, 16, 2, status->ammo)
           && WL_StatusDrawPicture(framebuffer, graphics,
                                   (status->keys & 1U) != 0U
                                   ? chunks.no_key + 1U : chunks.no_key,
                                   30, 4)
           && WL_StatusDrawPicture(framebuffer, graphics,
                                   (status->keys & 2U) != 0U
                                   ? chunks.no_key + 2U : chunks.no_key,
                                   30, 20)
           && WL_StatusDrawPicture(framebuffer, graphics,
                                   chunks.knife + status->weapon, 32, 8)
           && WL_DrawNumber(framebuffer, graphics, &chunks,
                            6, 16, 6, status->score);
}
