/* Portable fixed-tic play loop derived from WL_PLAY.C and WL_AGENT.C. */
#include "WL_PLAY.h"

#include <string.h>

#include "WL_ACT1.h"
#include "WL_AGENT.h"
#include "WL_GAME.h"
#include "WL_STATE.h"
#include "ID_VL.h"
#include "WG_PALETTE.h"

#define WL_BASE_MOVE 35
#define WL_RUN_MOVE 70
#define WL_MOUSE_ADJUSTMENT 5
#define WL_NUM_RED_SHIFTS 6U
#define WL_RED_STEPS 8U
#define WL_NUM_WHITE_SHIFTS 3U
#define WL_WHITE_STEPS 20U
#define WL_WHITE_TICS 6U

int WL_DrawPaused(uint8_t *framebuffer, const struct wg_graphics *graphics)
{
    size_t chunk;

    if (framebuffer == NULL || graphics == NULL)
    {
        return 0;
    }
    /* The supplied Apogee v1.4 WL1 graph uses the later layout also used by
       its status bar: PAUSEDPIC is chunk 145 rather than the source release's
       early generated-header values. */
    chunk = graphics->variant == WG_GAME_WOLF3D_SHAREWARE_14 ? 145U : 133U;
    /* Original LatchDrawPic(20-4, 80-2*8, PAUSEDPIC): its x unit was
       eight pixels, hence the centered 128,64 destination below. */
    return WG_VideoDrawPicture(framebuffer, graphics, chunk, 128, 64);
}

void WL_UpdatePaletteShifts(struct wg_level *level,
                            uint8_t palette[256 * 3])
{
    uint8_t shifted[256 * 3];
    unsigned red = 0U;
    unsigned white = 0U;
    size_t color;

    if (level == NULL || palette == NULL)
    {
        return;
    }
    if (level->bonus_count != 0U)
    {
        white = level->bonus_count / WL_WHITE_TICS + 1U;
        if (white > WL_NUM_WHITE_SHIFTS)
        {
            white = WL_NUM_WHITE_SHIFTS;
        }
        --level->bonus_count;
    }
    if (level->damage_count != 0U)
    {
        red = level->damage_count / 10U + 1U;
        if (red > WL_NUM_RED_SHIFTS)
        {
            red = WL_NUM_RED_SHIFTS;
        }
        --level->damage_count;
    }
    if (red == 0U && white == 0U)
    {
        memcpy(palette, WG_WolfPalette, 256U * 3U);
        return;
    }
    for (color = 0U; color < 256U; ++color)
    {
        size_t component = color * 3U;
        int base_red = WG_WolfPaletteVGA[component];
        int base_green = WG_WolfPaletteVGA[component + 1U];
        int base_blue = WG_WolfPaletteVGA[component + 2U];

        if (red != 0U)
        {
            shifted[component] = (uint8_t)(base_red
                + (64 - base_red) * (int)red / (int)WL_RED_STEPS);
            shifted[component + 1U] = (uint8_t)(base_green
                - base_green * (int)red / (int)WL_RED_STEPS);
            shifted[component + 2U] = (uint8_t)(base_blue
                - base_blue * (int)red / (int)WL_RED_STEPS);
        }
        else
        {
            shifted[component] = (uint8_t)(base_red
                + (64 - base_red) * (int)white / (int)WL_WHITE_STEPS);
            shifted[component + 1U] = (uint8_t)(base_green
                + (62 - base_green) * (int)white / (int)WL_WHITE_STEPS);
            shifted[component + 2U] = (uint8_t)(base_blue
                - base_blue * (int)white / (int)WL_WHITE_STEPS);
        }
    }
    WG_PaletteFromVGA(shifted, palette);
}

size_t WL_MusicChunkForMap(unsigned map_number)
{
    static const uint8_t songs[60] =
    {
        3U, 11U, 9U, 12U, 3U, 11U, 9U, 12U, 2U, 0U,
        8U, 18U, 17U, 4U, 8U, 18U, 4U, 17U, 2U, 1U,
        6U, 20U, 22U, 21U, 6U, 20U, 22U, 21U, 19U, 26U,
        3U, 11U, 9U, 12U, 3U, 11U, 9U, 12U, 2U, 0U,
        8U, 18U, 17U, 4U, 8U, 18U, 4U, 17U, 2U, 1U,
        6U, 20U, 22U, 21U, 6U, 20U, 22U, 21U, 19U, 15U
    };

    return 261U + songs[map_number < 60U ? map_number : 0U];
}

void WL_PlayStateReset(wl_play_state_t *state)
{
    if (state != NULL)
    {
        memset(state, 0, sizeof(*state));
    }
}

int WL_DemoOpen(wl_demo_t *demo, const uint8_t *data, size_t size)
{
    size_t length;

    if (demo == NULL || data == NULL || size < 4U)
    {
        return 0;
    }
    /* PlayDemo read a 16-bit length, then skipped the fourth header byte. */
    length = (size_t)data[1] | (size_t)data[2] << 8U;
    if (length < 4U || length > size || (length - 4U) % 3U != 0U)
    {
        return 0;
    }
    demo->commands = data + 4U;
    demo->command_count = (length - 4U) / 3U;
    demo->position = 0U;
    demo->map_number = data[0];
    return demo->command_count != 0U;
}

int WL_DemoNext(wl_demo_t *demo, wl_demo_command_t *command)
{
    const uint8_t *source;

    if (demo == NULL || command == NULL || demo->commands == NULL
        || demo->position >= demo->command_count)
    {
        return 0;
    }
    source = demo->commands + demo->position * 3U;
    command->buttons = source[0];
    command->control_x = (int8_t)source[1];
    command->control_y = (int8_t)source[2];
    ++demo->position;
    return 1;
}

static int WL_PlayFrame(struct wg_level *level,
                        const struct wg_view_tables *tables,
                        wl_play_state_t *state, const wl_input_t *input,
                        unsigned tics, int demo_control,
                        int raw_control_x, int raw_control_y)
{
    int speed;
    int control_x;
    int control_y;
    int attack_pressed;
    int attack_started = 0;
    int use_pressed;

    if (level == NULL || tables == NULL || state == NULL || input == NULL
        || tics == 0U)
    {
        return 0;
    }

    level->time_count += tics;

    attack_pressed = input->attack && !state->attack_held;
    use_pressed = input->use && !state->use_held;
    state->attack_held = input->attack != 0U;
    state->use_held = input->use != 0U;

    if (!WL_MoveDoors(level, tics) || !WL_MovePushWalls(level, tics))
    {
        return 0;
    }

    if (!level->player_dead && !level->victory_flag)
    {
        if (!level->attack_active)
        {
            if (input->weapon != 0U)
            {
                (void)WL_SelectWeapon(level, input->weapon - 1U);
            }
            if (use_pressed)
            {
                (void)WL_CmdUse(level);
            }
            if (attack_pressed)
            {
                attack_started = WL_StartAttack(level);
            }
        }

        if (demo_control)
        {
            control_x = raw_control_x * (int)tics;
            control_y = raw_control_y * (int)tics;
        }
        else
        {
            speed = (input->run ? WL_RUN_MOVE : WL_BASE_MOVE) * (int)tics;
            control_x = ((input->right != 0U) - (input->left != 0U)) * speed;
            control_y = ((input->down != 0U) - (input->up != 0U)) * speed;
            control_x += input->mouse_x * 10 / (13 - WL_MOUSE_ADJUSTMENT);
            control_y += input->mouse_y * 20 / (13 - WL_MOUSE_ADJUSTMENT);
        }
        if (control_x > 100 * (int)tics)
        {
            control_x = 100 * (int)tics;
        }
        else if (control_x < -100 * (int)tics)
        {
            control_x = -100 * (int)tics;
        }
        if (control_y > 100 * (int)tics)
        {
            control_y = 100 * (int)tics;
        }
        else if (control_y < -100 * (int)tics)
        {
            control_y = -100 * (int)tics;
        }
        if (!WL_ControlMovement(level, tables, control_x, control_y,
                                input->strafe != 0U)
            || (!attack_started
                && !WL_TickPlayerAttack(level, tics, input->attack != 0U)))
        {
            return 0;
        }
    }

    return WL_TickActors(level, tics);
}

int WL_PlayDemoCommand(struct wg_level *level,
                       const struct wg_view_tables *tables,
                       wl_play_state_t *state,
                       const wl_demo_command_t *command)
{
    wl_input_t input;
    unsigned weapon;

    if (command == NULL)
    {
        return 0;
    }
    memset(&input, 0, sizeof(input));
    input.attack = (uint8_t)((command->buttons & 0x01U) != 0U);
    input.strafe = (uint8_t)((command->buttons & 0x02U) != 0U);
    input.run = (uint8_t)((command->buttons & 0x04U) != 0U);
    input.use = (uint8_t)((command->buttons & 0x08U) != 0U);
    for (weapon = 0U; weapon < 4U; ++weapon)
    {
        if ((command->buttons & (uint8_t)(0x10U << weapon)) != 0U)
        {
            input.weapon = (uint8_t)(weapon + 1U);
            break;
        }
    }
    return WL_PlayFrame(level, tables, state, &input, WL_DEMO_TICS, 1,
                        command->control_x, command->control_y);
}

int WL_PlayTick(struct wg_level *level,
                const struct wg_view_tables *tables,
                wl_play_state_t *state, const wl_input_t *input)
{
    return WL_PlayFrame(level, tables, state, input, 1U, 0, 0, 0);
}
