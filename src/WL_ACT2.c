/* Portable actor construction from the original WL_ACT2.C. */
#include "WL_ACT2.h"

#include "WG_FIXED.h"
#include "WL_GAME.h"

#define WG_SPR_GRD_S_1 50U
#define WG_SPR_GRD_W1_1 58U
#define WG_SPR_GRD_DEAD 95U
#define WG_SPR_GRD_SHOOT1 96U
#define WG_SPR_DOG_W1_1 99U
#define WG_SPR_DOG_JUMP1 135U
#define WG_SPR_SS_S_1 138U
#define WG_SPR_SS_W1_1 146U
#define WG_SPR_SS_SHOOT1 184U
#define WG_SPR_MUT_S_1 187U
#define WG_SPR_MUT_W1_1 195U
#define WG_SPR_MUT_SHOOT1 234U
#define WG_SPR_OFC_S_1 238U
#define WG_SPR_OFC_W1_1 246U
#define WG_SPR_OFC_SHOOT1 285U
#define WG_SPR_BLINKY_W1 288U
#define WG_SPR_PINKY_W1 290U
#define WG_SPR_CLYDE_W1 292U
#define WG_SPR_INKY_W1 294U
#define WG_SPR_BOSS_W1 296U
#define WG_SPR_BOSS_SHOOT1 300U
#define WG_SPR_SCHABB_W1 307U
#define WG_SPR_SCHABB_SHOOT1 311U
#define WG_SPR_FAKE_W1 321U
#define WG_SPR_FAKE_SHOOT 325U
#define WG_SPR_MECHA_W1 334U
#define WG_SPR_MECHA_SHOOT1 338U
#define WG_SPR_GIFT_W1 360U
#define WG_SPR_GIFT_SHOOT1 364U
#define WG_SPR_GRETEL_W1 385U
#define WG_SPR_GRETEL_SHOOT1 389U
#define WG_SPR_FAT_W1 396U
#define WG_SPR_FAT_SHOOT1 400U
#define WG_SPR_BJ_W1 408U

/* Spear of Destiny's conditional sprite enum, from WL_DEF.H. */
#define WG_SPR_TRANS_W1 326U
#define WG_SPR_TRANS_SHOOT1 330U
#define WG_SPR_UBER_W1 349U
#define WG_SPR_UBER_SHOOT1 353U
#define WG_SPR_DEATH_W1 362U
#define WG_SPR_DEATH_SHOOT1 366U
#define WG_SPR_SPECTRE_W1 377U
#define WG_SPR_ANGEL_W1 385U
#define WG_SPR_ANGEL_SHOOT1 389U

#define WG_SPEED_PATROL 512
#define WG_SPEED_DOG 1500

static int32_t WL_StartHitPoints(wg_difficulty_t difficulty,
                                 wg_actor_class_t actor_class)
{
    static const int32_t boss_hit_points[4][7] =
    {
        {850, 850, 200, 800, 850, 850, 850},
        {950, 950, 300, 950, 950, 950, 950},
        {1050, 1550, 400, 1050, 1050, 1050, 1050},
        {1200, 2400, 500, 1200, 1200, 1200, 1200}
    };
    static const int32_t spear_hit_points[4][6] =
    {
        {5, 1450, 850, 1050, 950, 1250},
        {10, 1550, 950, 1150, 1050, 1350},
        {15, 1650, 1050, 1250, 1150, 1450},
        {25, 2000, 1200, 1400, 1300, 1600}
    };

    if (difficulty > WG_DIFFICULTY_HARD)
    {
        return 0;
    }
    switch (actor_class)
    {
    case WG_ACTOR_GUARD:
        return 25;
    case WG_ACTOR_OFFICER:
        return 50;
    case WG_ACTOR_SS:
        return 100;
    case WG_ACTOR_DOG:
        return 1;
    case WG_ACTOR_MUTANT:
        return difficulty == WG_DIFFICULTY_BABY ? 45
               : difficulty == WG_DIFFICULTY_HARD ? 65 : 55;
    case WG_ACTOR_GHOST:
        return 25;
    case WG_ACTOR_BOSS:
        return boss_hit_points[difficulty][0];
    case WG_ACTOR_SCHABBS:
        return boss_hit_points[difficulty][1];
    case WG_ACTOR_FAKE:
        return boss_hit_points[difficulty][2];
    case WG_ACTOR_MECHA_HITLER:
        return boss_hit_points[difficulty][3];
    case WG_ACTOR_GRETEL:
        return boss_hit_points[difficulty][4];
    case WG_ACTOR_GIFT:
        return boss_hit_points[difficulty][5];
    case WG_ACTOR_FAT:
        return boss_hit_points[difficulty][6];
    case WG_ACTOR_SPECTRE:
        return spear_hit_points[difficulty][0];
    case WG_ACTOR_ANGEL:
        return spear_hit_points[difficulty][1];
    case WG_ACTOR_TRANS:
        return spear_hit_points[difficulty][2];
    case WG_ACTOR_UBER:
        return spear_hit_points[difficulty][3];
    case WG_ACTOR_WILL:
        return spear_hit_points[difficulty][4];
    case WG_ACTOR_DEATH:
        return spear_hit_points[difficulty][5];
    default:
        return 0;
    }
}

static uint16_t WL_StandingShape(wg_actor_class_t actor_class)
{
    switch (actor_class)
    {
    case WG_ACTOR_GUARD:
        return WG_SPR_GRD_S_1;
    case WG_ACTOR_OFFICER:
        return WG_SPR_OFC_S_1;
    case WG_ACTOR_SS:
        return WG_SPR_SS_S_1;
    case WG_ACTOR_DOG:
        /* The map format has standing-dog codes although DOS SpawnStand
         * omits its dog case. Use the dog's path pose without advancing. */
        return WG_SPR_DOG_W1_1;
    case WG_ACTOR_MUTANT:
        return WG_SPR_MUT_S_1;
    default:
        return UINT16_MAX;
    }
}

static uint16_t WL_PatrolShape(wg_actor_class_t actor_class)
{
    switch (actor_class)
    {
    case WG_ACTOR_GUARD:
        return WG_SPR_GRD_W1_1;
    case WG_ACTOR_OFFICER:
        return WG_SPR_OFC_W1_1;
    case WG_ACTOR_SS:
        return WG_SPR_SS_W1_1;
    case WG_ACTOR_DOG:
        return WG_SPR_DOG_W1_1;
    case WG_ACTOR_MUTANT:
        return WG_SPR_MUT_W1_1;
    default:
        return UINT16_MAX;
    }
}

static uint16_t WL_AttackShape(wg_actor_class_t actor_class)
{
    switch (actor_class)
    {
    case WG_ACTOR_GUARD:
        return WG_SPR_GRD_SHOOT1;
    case WG_ACTOR_OFFICER:
        return WG_SPR_OFC_SHOOT1;
    case WG_ACTOR_SS:
        return WG_SPR_SS_SHOOT1;
    case WG_ACTOR_MUTANT:
        return WG_SPR_MUT_SHOOT1;
    case WG_ACTOR_DOG:
        return WG_SPR_DOG_JUMP1;
    default:
        return 0U;
    }
}

static uint16_t WL_BossAttackShape(wg_actor_class_t actor_class)
{
    switch (actor_class)
    {
    case WG_ACTOR_BOSS:
        return WG_SPR_BOSS_SHOOT1;
    case WG_ACTOR_SCHABBS:
        return WG_SPR_SCHABB_SHOOT1;
    case WG_ACTOR_FAKE:
        return WG_SPR_FAKE_SHOOT;
    case WG_ACTOR_MECHA_HITLER:
        return WG_SPR_MECHA_SHOOT1;
    case WG_ACTOR_GIFT:
        return WG_SPR_GIFT_SHOOT1;
    case WG_ACTOR_GRETEL:
        return WG_SPR_GRETEL_SHOOT1;
    case WG_ACTOR_FAT:
        return WG_SPR_FAT_SHOOT1;
    case WG_ACTOR_TRANS:
        return WG_SPR_TRANS_SHOOT1;
    case WG_ACTOR_UBER:
        return WG_SPR_UBER_SHOOT1;
    case WG_ACTOR_WILL:
        return 341U;
    case WG_ACTOR_DEATH:
        return WG_SPR_DEATH_SHOOT1;
    case WG_ACTOR_ANGEL:
        return WG_SPR_ANGEL_SHOOT1;
    default:
        return 0U;
    }
}

static int WL_SpawnActor(struct wg_level *level, uint8_t tile_x,
                         uint8_t tile_y, uint8_t map_direction,
                         wg_actor_class_t actor_class, uint16_t shape,
                         int patrol)
{
    wg_actor_t *actor;
    uint16_t sprite_offset;
    int destination_x = tile_x;
    int destination_y = tile_y;

    if (level == NULL || tile_x >= WG_LEVEL_SIZE || tile_y >= WG_LEVEL_SIZE
        || map_direction > 3U || level->actor_count >= WG_MAX_ACTORS)
    {
        return 0;
    }
    if (patrol)
    {
        switch (map_direction)
        {
        case 0:
            ++destination_x;
            break;
        case 1:
            --destination_y;
            break;
        case 2:
            --destination_x;
            break;
        default:
            ++destination_y;
            break;
        }
    }
    if (destination_x < 0 || destination_x >= WG_LEVEL_SIZE
        || destination_y < 0 || destination_y >= WG_LEVEL_SIZE)
    {
        return 0;
    }
    actor = &level->actors[level->actor_count++];
    sprite_offset = WG_DataVariantFamily(level->variant)
                            == WG_GAME_FAMILY_SPEAR ? 4U : 0U;
    actor->x = (int32_t)tile_x * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    actor->y = (int32_t)tile_y * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    actor->tile_x = (uint8_t)destination_x;
    actor->tile_y = (uint8_t)destination_y;
    actor->direction = (uint8_t)(map_direction * 2U);
    actor->shape = (uint16_t)(shape + sprite_offset);
    actor->base_shape = (uint16_t)(WL_PatrolShape(actor_class)
                                   + sprite_offset);
    actor->attack_shape = (uint16_t)(WL_AttackShape(actor_class)
                                     + sprite_offset);
    actor->rotate = 1U;
    actor->area_number = level->areas[(size_t)tile_y * WG_LEVEL_SIZE + tile_x];
    actor->angle = 0U;
    actor->flags = WG_ACTOR_FLAG_SHOOTABLE;
    if (patrol)
    {
        /* Original SpawnPatrol actors are active from creation.  Standing
           actors remain dormant until their area connects or they are seen. */
        actor->flags |= WG_ACTOR_FLAG_ACTIVE;
    }
    if (!patrol
        && level->ambush_tiles[(size_t)tile_y * WG_LEVEL_SIZE + tile_x])
    {
        actor->flags |= WG_ACTOR_FLAG_AMBUSH;
        level->tiles[(size_t)tile_y * WG_LEVEL_SIZE + tile_x] = 0U;
    }
    actor->tic_count = patrol ? WG_RandomNext(&level->random) % 20U : 0;
    actor->reaction_time = 0;
    actor->speed = actor_class == WG_ACTOR_DOG
                       ? WG_SPEED_DOG : WG_SPEED_PATROL;
    actor->distance = patrol ? WG_FIXED_ONE : 0;
    actor->hit_points = WL_StartHitPoints(level->difficulty, actor_class);
    actor->state = patrol ? WG_STATE_PATH1 : WG_STATE_STAND;
    actor->actor_class = actor_class;
    return 1;
}

int WL_SpawnStand(struct wg_level *level, wg_actor_class_t actor_class,
                  uint8_t tile_x, uint8_t tile_y, uint8_t map_direction)
{
    uint16_t shape = WL_StandingShape(actor_class);

    if (level == NULL || shape == UINT16_MAX
        || !WL_SpawnActor(level, tile_x, tile_y, map_direction,
                          actor_class, shape, 0))
    {
        return 0;
    }
    return 1;
}

int WL_SpawnPatrol(struct wg_level *level, wg_actor_class_t actor_class,
                   uint8_t tile_x, uint8_t tile_y, uint8_t map_direction)
{
    uint16_t shape = WL_PatrolShape(actor_class);

    if (level == NULL || shape == UINT16_MAX
        || !WL_SpawnActor(level, tile_x, tile_y, map_direction,
                          actor_class, shape, 1))
    {
        return 0;
    }
    return 1;
}

int WL_SpawnDeadGuard(struct wg_level *level, uint8_t tile_x, uint8_t tile_y)
{
    wg_actor_t *actor;

    if (level == NULL || tile_x >= WG_LEVEL_SIZE || tile_y >= WG_LEVEL_SIZE
        || level->actor_count >= WG_MAX_ACTORS)
    {
        return 0;
    }
    actor = &level->actors[level->actor_count++];
    actor->x = (int32_t)tile_x * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    actor->y = (int32_t)tile_y * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    actor->tile_x = tile_x;
    actor->tile_y = tile_y;
    actor->direction = 0U;
    actor->shape = (uint16_t)(WG_SPR_GRD_DEAD
        + (WG_DataVariantFamily(level->variant) == WG_GAME_FAMILY_SPEAR
               ? 4U : 0U));
    actor->base_shape = actor->shape;
    actor->attack_shape = 0U;
    actor->rotate = 0U;
    actor->area_number = level->areas[(size_t)tile_y * WG_LEVEL_SIZE + tile_x];
    actor->angle = 0U;
    actor->flags = 0U;
    actor->tic_count = 0;
    actor->reaction_time = 0;
    actor->speed = 0;
    actor->distance = 0;
    actor->hit_points = 0;
    actor->state = WG_STATE_NONE;
    actor->actor_class = WG_ACTOR_INERT;
    return 1;
}

int WL_SpawnBoss(struct wg_level *level, wg_actor_class_t actor_class,
                 uint8_t tile_x, uint8_t tile_y)
{
    wg_actor_t *actor;
    uint16_t shape;
    uint8_t direction;

    switch (actor_class)
    {
    case WG_ACTOR_BOSS:
        shape = WG_SPR_BOSS_W1;
        direction = 6U;
        break;
    case WG_ACTOR_SCHABBS:
        shape = WG_SPR_SCHABB_W1;
        direction = 6U;
        break;
    case WG_ACTOR_FAKE:
        shape = WG_SPR_FAKE_W1;
        direction = 2U;
        break;
    case WG_ACTOR_MECHA_HITLER:
        shape = WG_SPR_MECHA_W1;
        direction = 6U;
        break;
    case WG_ACTOR_GRETEL:
        shape = WG_SPR_GRETEL_W1;
        direction = 2U;
        break;
    case WG_ACTOR_GIFT:
        shape = WG_SPR_GIFT_W1;
        direction = 2U;
        break;
    case WG_ACTOR_FAT:
        shape = WG_SPR_FAT_W1;
        direction = 6U;
        break;
    case WG_ACTOR_SPECTRE:
        shape = WG_SPR_SPECTRE_W1;
        direction = 0U;
        break;
    case WG_ACTOR_ANGEL:
        shape = WG_SPR_ANGEL_W1;
        direction = 0U;
        break;
    case WG_ACTOR_TRANS:
        shape = WG_SPR_TRANS_W1;
        direction = 0U;
        break;
    case WG_ACTOR_UBER:
        shape = WG_SPR_UBER_W1;
        direction = 0U;
        break;
    case WG_ACTOR_WILL:
        shape = 337U;
        direction = 0U;
        break;
    case WG_ACTOR_DEATH:
        shape = WG_SPR_DEATH_W1;
        direction = 0U;
        break;
    default:
        return 0;
    }
    if (level == NULL || tile_x >= WG_LEVEL_SIZE || tile_y >= WG_LEVEL_SIZE
        || level->actor_count >= WG_MAX_ACTORS)
    {
        return 0;
    }
    actor = &level->actors[level->actor_count++];
    actor->x = (int32_t)tile_x * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    actor->y = (int32_t)tile_y * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    actor->tile_x = tile_x;
    actor->tile_y = tile_y;
    actor->direction = direction;
    actor->shape = shape;
    actor->base_shape = shape;
    actor->attack_shape = WL_BossAttackShape(actor_class);
    actor->rotate = 0U;
    actor->area_number = level->areas[(size_t)tile_y * WG_LEVEL_SIZE + tile_x];
    actor->angle = 0U;
    actor->flags = WG_ACTOR_FLAG_SHOOTABLE | WG_ACTOR_FLAG_AMBUSH;
    actor->tic_count = actor_class == WG_ACTOR_SPECTRE ? 10 : 0;
    actor->reaction_time = 0;
    actor->speed = WG_SPEED_PATROL;
    actor->distance = 0;
    actor->hit_points = WL_StartHitPoints(level->difficulty, actor_class);
    actor->state = WG_STATE_STAND;
    actor->actor_class = actor_class;
    return 1;
}

int WL_SpawnGhost(struct wg_level *level, wg_ghost_kind_t ghost_kind,
                  uint8_t tile_x, uint8_t tile_y)
{
    static const uint16_t shapes[] =
    {
        WG_SPR_BLINKY_W1,
        WG_SPR_CLYDE_W1,
        WG_SPR_PINKY_W1,
        WG_SPR_INKY_W1
    };
    wg_actor_t *actor;

    if (level == NULL || ghost_kind < WG_GHOST_BLINKY
        || ghost_kind > WG_GHOST_INKY || tile_x >= WG_LEVEL_SIZE
        || tile_y >= WG_LEVEL_SIZE || level->actor_count >= WG_MAX_ACTORS)
    {
        return 0;
    }
    actor = &level->actors[level->actor_count++];
    actor->x = (int32_t)tile_x * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    actor->y = (int32_t)tile_y * WG_FIXED_ONE + WG_FIXED_ONE / 2;
    actor->tile_x = tile_x;
    actor->tile_y = tile_y;
    actor->direction = 0U;
    actor->shape = shapes[ghost_kind];
    actor->base_shape = shapes[ghost_kind];
    actor->attack_shape = 0U;
    actor->rotate = 0U;
    actor->area_number = level->areas[(size_t)tile_y * WG_LEVEL_SIZE + tile_x];
    actor->angle = 0U;
    actor->flags = WG_ACTOR_FLAG_AMBUSH;
    actor->tic_count = WG_RandomNext(&level->random) % 10U;
    actor->reaction_time = 0;
    actor->speed = WG_SPEED_DOG;
    actor->distance = WG_FIXED_ONE;
    actor->hit_points = WL_StartHitPoints(level->difficulty, WG_ACTOR_GHOST);
    actor->state = WG_STATE_GHOST1;
    actor->actor_class = WG_ACTOR_GHOST;
    return 1;
}

int WL_SpawnBJVictory(struct wg_level *level)
{
    wg_actor_t *actor;
    uint8_t tile_y;

    if (level == NULL || level->actor_count >= WG_MAX_ACTORS
        || level->player_tile_y >= WG_LEVEL_SIZE - 1U)
    {
        return 0;
    }
    tile_y = (uint8_t)(level->player_tile_y + 1U);
    actor = &level->actors[level->actor_count++];
    actor->x = level->player_x;
    actor->y = level->player_y;
    actor->tile_x = level->player_tile_x;
    actor->tile_y = tile_y;
    actor->direction = 2U;
    actor->shape = WG_SPR_BJ_W1;
    actor->base_shape = WG_SPR_BJ_W1;
    actor->attack_shape = 0U;
    actor->rotate = 0U;
    actor->area_number = level->areas[(size_t)tile_y * WG_LEVEL_SIZE
                                      + level->player_tile_x];
    actor->angle = 0U;
    actor->flags = WG_ACTOR_FLAG_ACTIVE;
    actor->tic_count = 12;
    /* The original stores the six remaining path tiles in temp1. */
    actor->reaction_time = 6;
    actor->speed = 2048;
    actor->distance = WG_FIXED_ONE;
    actor->hit_points = 0;
    actor->view_x = 0;
    actor->trans_x = 0;
    actor->state = WG_STATE_BJ_RUN1;
    actor->actor_class = WG_ACTOR_BJ;
    return 1;
}
