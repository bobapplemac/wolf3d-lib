#include "WOLF3DGENERIC.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "WG_DATA.h"
#include "WG_FIXED.h"
#include "WG_GRAPHICS.h"
#include "ID_US_1.h"
#include "WL_AGENT.h"
#include "WL_ACT1.h"
#include "WL_GAME.h"
#include "WG_MAPS.h"
#include "WG_PALETTE.h"
#include "ID_PM.h"
#include "WG_PLATFORM.h"
#include "WG_RENDERER.h"
#include "WL_DRAW.h"
#include "WL_MAIN.h"
#include "WL_STATE.h"

uint8_t *WG_ScreenBuffer;
uint8_t WG_Palette[WG_PALETTE_COLORS * 3];

static int wg_initialized;
static int wg_data_loaded;
static wg_data_set_t wg_data_set;

static const char *WG_FindDataPath(int argc, char **argv)
{
    int index;

    for (index = 1; index < argc; ++index)
    {
        if (strcmp(argv[index], "--data") == 0)
        {
            return index + 1 < argc ? argv[index + 1] : NULL;
        }
    }
    return NULL;
}

static int WG_HasArgument(int argc, char **argv, const char *argument)
{
    int index;

    for (index = 1; index < argc; ++index)
    {
        if (strcmp(argv[index], argument) == 0)
        {
            return 1;
        }
    }
    return 0;
}

static int WG_FindUnsignedArgument(int argc, char **argv,
                                   const char *argument,
                                   unsigned default_value,
                                   unsigned maximum, unsigned *value_out)
{
    int index;

    if (argument == NULL || value_out == NULL)
    {
        return 0;
    }
    *value_out = default_value;
    for (index = 1; index < argc; ++index)
    {
        if (strcmp(argv[index], argument) == 0)
        {
            char *end;
            unsigned long value;

            if (index + 1 >= argc)
            {
                return 0;
            }
            end = NULL;
            value = strtoul(argv[index + 1], &end, 10);
            if (end == argv[index + 1] || *end != '\0' || value > maximum)
            {
                return 0;
            }
            *value_out = (unsigned)value;
            return 1;
        }
    }
    return 1;
}

static wg_actor_t *WG_FindGuardViewActor(wg_level_t *level)
{
    size_t index;

    if (level == NULL)
    {
        return NULL;
    }
    for (index = level->actor_count; index > 0U; --index)
    {
        wg_actor_t *actor = &level->actors[index - 1U];

        if (actor->actor_class == WG_ACTOR_GUARD
            && actor->rotate != 0U && actor->tile_x >= 3U)
        {
            return actor;
        }
    }
    return NULL;
}

static int WG_IsBossClass(wg_actor_class_t actor_class)
{
    return actor_class == WG_ACTOR_BOSS
           || actor_class == WG_ACTOR_SCHABBS
           || actor_class == WG_ACTOR_FAKE
           || actor_class == WG_ACTOR_MECHA_HITLER
           || actor_class == WG_ACTOR_GRETEL
           || actor_class == WG_ACTOR_GIFT
           || actor_class == WG_ACTOR_FAT;
}

static wg_actor_t *WG_FindBossViewActor(wg_level_t *level)
{
    size_t index;

    if (level == NULL)
    {
        return NULL;
    }
    for (index = 0; index < level->actor_count; ++index)
    {
        if (WG_IsBossClass(level->actors[index].actor_class))
        {
            return &level->actors[index];
        }
    }
    return NULL;
}

static const wg_actor_t *WG_FindPatrolViewActor(const wg_level_t *level)
{
    size_t index;

    if (level == NULL)
    {
        return NULL;
    }
    for (index = 0; index < level->actor_count; ++index)
    {
        if (level->actors[index].state >= WG_STATE_PATH1
            && level->actors[index].state <= WG_STATE_PATH4)
        {
            return &level->actors[index];
        }
    }
    return NULL;
}

static wg_actor_t *WG_FindDogViewActor(wg_level_t *level)
{
    size_t index;

    if (level == NULL)
    {
        return NULL;
    }
    for (index = 0; index < level->actor_count; ++index)
    {
        if (level->actors[index].actor_class == WG_ACTOR_DOG)
        {
            return &level->actors[index];
        }
    }
    return NULL;
}

static wg_actor_t *WG_FindActorClass(wg_level_t *level,
                                     wg_actor_class_t actor_class)
{
    size_t index;

    if (level == NULL)
    {
        return NULL;
    }
    for (index = 0; index < level->actor_count; ++index)
    {
        if (level->actors[index].actor_class == actor_class)
        {
            return &level->actors[index];
        }
    }
    return NULL;
}

static int WG_SetActorViewPose(wg_level_t *level, const wg_actor_t *actor,
                               int distance)
{
    static const int directions_x[] = { -1, 1, 0, 0 };
    static const int directions_y[] = { 0, 0, -1, 1 };
    static const uint16_t angles[] = { 0U, 180U, 270U, 90U };
    size_t direction;

    if (level == NULL || actor == NULL || distance <= 0)
    {
        return 0;
    }
    for (direction = 0; direction < 4U; ++direction)
    {
        int step;
        int clear = 1;

        for (step = 1; step <= distance; ++step)
        {
            int x = (int)actor->tile_x + directions_x[direction] * step;
            int y = (int)actor->tile_y + directions_y[direction] * step;

            if (x < 0 || x >= WG_LEVEL_SIZE || y < 0 || y >= WG_LEVEL_SIZE
                || level->tiles[(size_t)y * WG_LEVEL_SIZE + (size_t)x] != 0U)
            {
                clear = 0;
                break;
            }
        }
        if (clear)
        {
            level->player_x = actor->x
                              + directions_x[direction] * distance
                                * WG_FIXED_ONE;
            level->player_y = actor->y
                              + directions_y[direction] * distance
                                * WG_FIXED_ONE;
            level->player_angle = angles[direction];
            level->player_tile_x = (uint8_t)(level->player_x / WG_FIXED_ONE);
            level->player_tile_y = (uint8_t)(level->player_y / WG_FIXED_ONE);
            return 1;
        }
    }
    return 0;
}

static int WG_SetPushWallView(wg_level_t *level)
{
    static const int direction_x[] = { 1, 0, -1, 0 };
    static const int direction_y[] = { 0, -1, 0, 1 };
    static const uint16_t angles[] = { 0U, 90U, 180U, 270U };
    size_t index;

    if (level == NULL)
    {
        return 0;
    }
    for (index = 0U; index < WG_LEVEL_SIZE * WG_LEVEL_SIZE; ++index)
    {
        int wall_x;
        int wall_y;
        size_t direction;

        if (level->info[index] != 98U)
        {
            continue;
        }
        wall_x = (int)(index % WG_LEVEL_SIZE);
        wall_y = (int)(index / WG_LEVEL_SIZE);
        for (direction = 0U; direction < 4U; ++direction)
        {
            int target_x = wall_x + direction_x[direction];
            int target_y = wall_y + direction_y[direction];
            int near_x = wall_x - direction_x[direction];
            int near_y = wall_y - direction_y[direction];
            int view_x = wall_x - direction_x[direction] * 2;
            int view_y = wall_y - direction_y[direction] * 2;

            if (target_x <= 0 || target_x + 1 >= WG_LEVEL_SIZE
                || target_y <= 0 || target_y + 1 >= WG_LEVEL_SIZE
                || view_x <= 0 || view_x + 1 >= WG_LEVEL_SIZE
                || view_y <= 0 || view_y + 1 >= WG_LEVEL_SIZE
                || level->tiles[(size_t)target_y * WG_LEVEL_SIZE
                                + (size_t)target_x] != 0U
                || level->tiles[(size_t)near_y * WG_LEVEL_SIZE
                                + (size_t)near_x] != 0U
                || level->tiles[(size_t)view_y * WG_LEVEL_SIZE
                                + (size_t)view_x] != 0U)
            {
                continue;
            }
            level->player_tile_x = (uint8_t)view_x;
            level->player_tile_y = (uint8_t)view_y;
            level->player_x = view_x * WG_FIXED_ONE + WG_FIXED_ONE / 2;
            level->player_y = view_y * WG_FIXED_ONE + WG_FIXED_ONE / 2;
            level->player_angle = angles[direction];
            return WL_PushWall(level, (uint8_t)wall_x, (uint8_t)wall_y,
                               (uint8_t)(direction * 2U))
                   && WL_MovePushWalls(level, 63U);
        }
    }
    return 0;
}

static int WG_LoadTitleScreen(const char *data_path)
{
    wg_graphics_t graphics;
    char title[160];
    int title_length;

    if (!WG_DataOpen(&wg_data_set, data_path))
    {
        WG_ReportError("The selected directory is not a supported Wolf3D v1.4 data set.");
        return 0;
    }
    if (!WG_GraphicsOpen(&graphics, &wg_data_set))
    {
        WG_ReportError("Unable to open the Wolf3D graphics resources.");
        WG_DataClose(&wg_data_set);
        return 0;
    }
    if (!WG_GraphicsDecodeTitle(&graphics, wg_data_set.variant, WG_ScreenBuffer))
    {
        WG_ReportError("Unable to decode the Wolf3D title screen.");
        WG_GraphicsClose(&graphics);
        WG_DataClose(&wg_data_set);
        return 0;
    }
    WG_GraphicsClose(&graphics);

    memcpy(WG_Palette, WG_WolfPalette, sizeof(WG_Palette));
    title_length = snprintf(title, sizeof(title), "wolf3dgeneric - %s",
                            WG_DataVariantName(wg_data_set.variant));
    if (title_length >= 0 && (size_t)title_length < sizeof(title))
    {
        WG_SetWindowTitle(title);
    }
    wg_data_loaded = 1;
    return 1;
}

static int WG_LoadInitialPlayView(unsigned map_number, int open_doors,
                                  int guard_view, int boss_view,
                                  int patrol_view, int alert_view,
                                  int chase_view, int fire_view, int bite_view,
                                  int boss_fire_view,
                                  int needle_view,
                                  int rocket_view,
                                  int flame_view,
                                  int pushwall_view,
                                  int death_view,
                                  int boss_death_view,
                                  unsigned actor_tics)
{
    wg_maps_t maps;
    wg_map_t map;
    wg_level_t level;
    wg_pages_t pages;
    wg_wall_cache_t walls;
    wg_view_tables_t view;
    wg_graphics_t graphics;
    wl_status_t status;
    wg_wall_hit_t hits[WG_MAX_VIEW_WIDTH];
    uint8_t visible_tiles[WG_LEVEL_SIZE * WG_LEVEL_SIZE];
    int success = 0;

    memset(&maps, 0, sizeof(maps));
    memset(&map, 0, sizeof(map));
    memset(&pages, 0, sizeof(pages));
    memset(&walls, 0, sizeof(walls));
    memset(&view, 0, sizeof(view));
    memset(&graphics, 0, sizeof(graphics));
    if (!WG_MapsOpen(&maps, &wg_data_set)
        || !WG_MapsLoad(&maps, map_number, &map)
        || !WG_LevelBuild(&map, &level)
        || !WG_PagesOpen(&pages, &wg_data_set)
        || !WG_WallCacheLoad(&walls, &pages)
        || !WG_GraphicsOpen(&graphics, &wg_data_set))
    {
        goto cleanup;
    }
    if (open_doors)
    {
        uint8_t door;

        for (door = 0; door < level.door_count; ++door)
        {
            level.doors[door].position = 0xffffU;
        }
    }
    if (!chase_view && !fire_view && !bite_view && !boss_fire_view
        && !needle_view
        && !rocket_view
        && !flame_view
        && !pushwall_view
        && !death_view
        && !boss_death_view
        && !WL_TickActors(&level, actor_tics))
    {
        goto cleanup;
    }
    if (guard_view || alert_view || chase_view || fire_view || death_view)
    {
        wg_actor_t *actor = WG_FindGuardViewActor(&level);
        size_t player_tile;

        if (actor == NULL)
        {
            goto cleanup;
        }
        level.player_x = actor->x - 3 * WG_FIXED_ONE;
        level.player_y = actor->y;
        level.player_angle = 0U;
        level.player_tile_x = (uint8_t)(level.player_x / WG_FIXED_ONE);
        level.player_tile_y = (uint8_t)(level.player_y / WG_FIXED_ONE);
        player_tile = (size_t)actor->tile_y * WG_LEVEL_SIZE
                      + actor->tile_x - 3U;
        if (level.tiles[player_tile] != 0U)
        {
            goto cleanup;
        }
    }
    if (boss_view || boss_fire_view || boss_death_view)
    {
        wg_actor_t *actor = WG_FindBossViewActor(&level);

        if (!WG_SetActorViewPose(&level, actor, 3))
        {
            goto cleanup;
        }
    }
    if (patrol_view)
    {
        const wg_actor_t *actor = WG_FindPatrolViewActor(&level);

        if (!WG_SetActorViewPose(&level, actor, 3))
        {
            goto cleanup;
        }
    }
    if ((alert_view || chase_view)
        && (!WL_TickAwareness(&level, 1U, 0)
            || !WL_TickAwareness(&level, 64U, 0)))
    {
        goto cleanup;
    }
    if (chase_view && !WL_TickActors(&level, actor_tics))
    {
        goto cleanup;
    }
    if (fire_view)
    {
        wg_actor_t *actor = WG_FindGuardViewActor(&level);

        if (actor == NULL || actor->attack_shape == 0U)
        {
            goto cleanup;
        }
        actor->state = WG_STATE_SHOOT2;
        actor->tic_count = 1;
        actor->shape = (uint16_t)(actor->attack_shape + 1U);
        actor->flags |= WG_ACTOR_FLAG_ATTACK_MODE | WG_ACTOR_FLAG_VISIBLE;
        if (!WL_TickActors(&level, 1U))
        {
            goto cleanup;
        }
    }
    if (bite_view)
    {
        wg_actor_t *actor = WG_FindDogViewActor(&level);

        if (!WG_SetActorViewPose(&level, actor, 2)
            || actor->attack_shape == 0U)
        {
            goto cleanup;
        }
        actor->state = WG_STATE_DOG_JUMP2;
        actor->tic_count = 1;
        actor->shape = (uint16_t)(actor->attack_shape + 1U);
        actor->flags |= WG_ACTOR_FLAG_ATTACK_MODE;
        WG_RandomSeed(&level.random, 0U);
        if (!WL_TickActors(&level, 1U))
        {
            goto cleanup;
        }
    }
    if (boss_fire_view)
    {
        wg_actor_t *actor = WG_FindBossViewActor(&level);

        if (actor == NULL || actor->attack_shape == 0U)
        {
            goto cleanup;
        }
        actor->state = WG_STATE_SHOOT2;
        actor->tic_count = 1;
        actor->shape = (uint16_t)(actor->attack_shape + 1U);
        actor->flags |= WG_ACTOR_FLAG_ATTACK_MODE | WG_ACTOR_FLAG_VISIBLE;
        WG_RandomSeed(&level.random, 0U);
        if (!WL_TickActors(&level, 1U))
        {
            goto cleanup;
        }
    }
    if (needle_view)
    {
        wg_actor_t *actor = WG_FindActorClass(&level, WG_ACTOR_SCHABBS);
        int32_t actor_x;
        int32_t actor_y;
        uint8_t actor_tile_x;
        uint8_t actor_tile_y;

        if (!WG_SetActorViewPose(&level, actor, 2)
            || actor->attack_shape == 0U)
        {
            goto cleanup;
        }
        actor_x = actor->x;
        actor_y = actor->y;
        actor_tile_x = actor->tile_x;
        actor_tile_y = actor->tile_y;
        actor->state = WG_STATE_SHOOT2;
        actor->tic_count = 1;
        actor->shape = (uint16_t)(actor->attack_shape + 1U);
        actor->flags |= WG_ACTOR_FLAG_ATTACK_MODE;
        WG_RandomSeed(&level.random, 0U);
        if (!WL_TickActors(&level, 1U))
        {
            goto cleanup;
        }
        actor->x = actor_x;
        actor->y = actor_y;
        actor->tile_x = actor_tile_x;
        actor->tile_y = actor_tile_y;
        actor->state = WG_STATE_SHOOT2;
        actor->tic_count = 100;
        actor->shape = (uint16_t)(actor->attack_shape + 1U);
        if (!WL_TickActors(&level, 8U))
        {
            goto cleanup;
        }
    }
    if (rocket_view)
    {
        wg_actor_t *actor = WG_FindActorClass(&level, WG_ACTOR_GIFT);
        int32_t actor_x;
        int32_t actor_y;
        uint8_t actor_tile_x;
        uint8_t actor_tile_y;

        if (!WG_SetActorViewPose(&level, actor, 3)
            || actor->attack_shape == 0U)
        {
            goto cleanup;
        }
        actor_x = actor->x;
        actor_y = actor->y;
        actor_tile_x = actor->tile_x;
        actor_tile_y = actor->tile_y;
        actor->state = WG_STATE_SHOOT2;
        actor->tic_count = 1;
        actor->shape = (uint16_t)(actor->attack_shape + 1U);
        actor->flags |= WG_ACTOR_FLAG_ATTACK_MODE;
        WG_RandomSeed(&level.random, 0U);
        if (!WL_TickActors(&level, 1U))
        {
            goto cleanup;
        }
        actor->x = actor_x;
        actor->y = actor_y;
        actor->tile_x = actor_tile_x;
        actor->tile_y = actor_tile_y;
        actor->state = WG_STATE_SHOOT2;
        actor->tic_count = 100;
        actor->shape = (uint16_t)(actor->attack_shape + 1U);
        if (!WL_TickActors(&level, 10U))
        {
            goto cleanup;
        }
    }
    if (flame_view)
    {
        wg_actor_t *actor = WG_FindActorClass(&level, WG_ACTOR_FAKE);
        unsigned burst;

        if (!WG_SetActorViewPose(&level, actor, 2)
            || actor->attack_shape == 0U)
        {
            goto cleanup;
        }
        actor->state = WG_STATE_SHOOT1;
        actor->tic_count = 1;
        actor->shape = actor->attack_shape;
        actor->flags |= WG_ACTOR_FLAG_ATTACK_MODE;
        if (!WL_TickActors(&level, 1U))
        {
            goto cleanup;
        }
        for (burst = 0U; burst < 3U; ++burst)
        {
            if (!WL_TickActors(&level, 8U))
            {
                goto cleanup;
            }
        }
    }
    if (pushwall_view && !WG_SetPushWallView(&level))
    {
        goto cleanup;
    }
    if (death_view)
    {
        wg_actor_t *actor = WG_FindGuardViewActor(&level);
        size_t actor_index;

        if (actor == NULL)
        {
            goto cleanup;
        }
        actor_index = (size_t)(actor - level.actors);
        if (!WL_DamageActor(&level, actor_index,
                            (unsigned)actor->hit_points)
            || !WL_TickActors(&level, 30U))
        {
            goto cleanup;
        }
    }
    if (boss_death_view)
    {
        wg_actor_t *actor = WG_FindBossViewActor(&level);
        size_t actor_index;

        if (actor == NULL)
        {
            goto cleanup;
        }
        actor_index = (size_t)(actor - level.actors);
        if (!WL_DamageActor(&level, actor_index,
                            (unsigned)actor->hit_points)
            || !WL_TickActors(&level, 30U))
        {
            goto cleanup;
        }
    }
    WG_ViewBuildTrigTables(&view);
    if (!WG_ViewCalculateProjection(&view, WG_MAX_VIEW_WIDTH,
                                    WG_FOCAL_LENGTH)
        || !WG_RenderStaticView(WG_ScreenBuffer, &level, &view, &walls,
                                0, 0, level.player_x, level.player_y,
                                level.player_angle, hits, visible_tiles)
        || !WL_DrawScaleds(WG_ScreenBuffer, &pages, &level, &view, hits,
                           visible_tiles,
                           level.player_x, level.player_y,
                           level.player_angle)
        || !WL_DrawPlayerWeapon(WG_ScreenBuffer, &pages, 1, 0))
    {
        goto cleanup;
    }
    WL_StatusDefaults(&status);
    status.health = level.player_health;
    if (!WL_DrawStatusBar(WG_ScreenBuffer, &graphics, &status))
    {
        goto cleanup;
    }
    success = 1;

cleanup:
    WG_GraphicsClose(&graphics);
    WG_WallCacheFree(&walls);
    WG_PagesClose(&pages);
    WG_MapFree(&map);
    WG_MapsClose(&maps);
    if (!success)
    {
        WG_ReportError("Unable to render the initial Wolf3D play view.");
    }
    return success;
}

wg_result_t wolf3dgeneric_Create(int argc, char **argv)
{
    size_t framebuffer_size;
    const char *data_path;
    unsigned map_number;
    unsigned actor_tics;

    if (wg_initialized || argc < 0 || (argc > 0 && argv == NULL)
        || !WG_FindUnsignedArgument(argc, argv, "--map", 0U, 99U,
                                    &map_number)
        || !WG_FindUnsignedArgument(argc, argv, "--actor-tics", 0U,
                                    10000U, &actor_tics))
    {
        return WG_RESULT_INVALID_ARGUMENT;
    }

    framebuffer_size = (size_t)WG_SCREEN_WIDTH * (size_t)WG_SCREEN_HEIGHT;
    WG_ScreenBuffer = (uint8_t *)calloc(framebuffer_size, sizeof(*WG_ScreenBuffer));
    if (WG_ScreenBuffer == NULL)
    {
        WG_ReportError("Unable to allocate the Wolf3D framebuffer.");
        return WG_RESULT_PLATFORM_ERROR;
    }

    memset(WG_Palette, 0, sizeof(WG_Palette));
    if (!WG_Init())
    {
        free(WG_ScreenBuffer);
        WG_ScreenBuffer = NULL;
        return WG_RESULT_PLATFORM_ERROR;
    }

    WG_SetWindowTitle("wolf3dgeneric bootstrap");
    wg_initialized = 1;
    data_path = WG_FindDataPath(argc, argv);
    if (data_path != NULL && !WG_LoadTitleScreen(data_path))
    {
        wolf3dgeneric_Shutdown();
        return WG_RESULT_PLATFORM_ERROR;
    }
    if (data_path != NULL && WG_HasArgument(argc, argv, "--play-view")
        && !WG_LoadInitialPlayView(
            map_number,
            WG_HasArgument(argc, argv, "--open-doors"),
            WG_HasArgument(argc, argv, "--guard-view"),
            WG_HasArgument(argc, argv, "--boss-view"),
            WG_HasArgument(argc, argv, "--patrol-view"),
            WG_HasArgument(argc, argv, "--alert-view"),
            WG_HasArgument(argc, argv, "--chase-view"),
            WG_HasArgument(argc, argv, "--fire-view"),
            WG_HasArgument(argc, argv, "--bite-view"),
            WG_HasArgument(argc, argv, "--boss-fire-view"),
            WG_HasArgument(argc, argv, "--needle-view"),
            WG_HasArgument(argc, argv, "--rocket-view"),
            WG_HasArgument(argc, argv, "--flame-view"),
            WG_HasArgument(argc, argv, "--pushwall-view"),
            WG_HasArgument(argc, argv, "--death-view"),
            WG_HasArgument(argc, argv, "--boss-death-view"), actor_tics))
    {
        wolf3dgeneric_Shutdown();
        return WG_RESULT_PLATFORM_ERROR;
    }
    return WG_RESULT_OK;
}

wg_result_t wolf3dgeneric_Run(void)
{
    wg_event_t event;

    if (!wg_initialized)
    {
        return WG_RESULT_INVALID_ARGUMENT;
    }

    WG_Present(WG_ScreenBuffer, WG_Palette);
    if (!wg_data_loaded || !WG_IsInteractive())
    {
        return WG_RESULT_NOT_IMPLEMENTED;
    }

    for (;;)
    {
        while (WG_PollEvent(&event))
        {
            if (event.type == WG_EVENT_QUIT)
            {
                return WG_RESULT_QUIT;
            }
        }
        WG_Present(WG_ScreenBuffer, WG_Palette);
        WG_SleepMs(10);
    }
}

void wolf3dgeneric_Shutdown(void)
{
    if (!wg_initialized)
    {
        return;
    }

    WG_Shutdown();
    WG_DataClose(&wg_data_set);
    free(WG_ScreenBuffer);
    WG_ScreenBuffer = NULL;
    wg_data_loaded = 0;
    wg_initialized = 0;
}
