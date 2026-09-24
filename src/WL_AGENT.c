/* Portable player combat and status bar from the original WL_AGENT.C. */
#include "WL_AGENT.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "WL_GAME.h"
#include "WL_STATE.h"

enum
{
    WL_STATUS_HEIGHT = 40,
    WL_STATUS_Y = WG_VIDEO_HEIGHT - WL_STATUS_HEIGHT
};

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
