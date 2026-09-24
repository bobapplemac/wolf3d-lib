/* Portable fixed-tic play loop derived from WL_PLAY.C and WL_AGENT.C. */
#include "WL_PLAY.h"

#include <string.h>

#include "WL_ACT1.h"
#include "WL_AGENT.h"
#include "WL_GAME.h"
#include "WL_STATE.h"

#define WL_BASE_MOVE 35
#define WL_RUN_MOVE 70

void WL_PlayStateReset(wl_play_state_t *state)
{
    if (state != NULL)
    {
        memset(state, 0, sizeof(*state));
    }
}

int WL_PlayTick(struct wg_level *level,
                const struct wg_view_tables *tables,
                wl_play_state_t *state, const wl_input_t *input)
{
    int speed;
    int control_x;
    int control_y;
    int attack_pressed;
    int attack_started = 0;
    int use_pressed;

    if (level == NULL || tables == NULL || state == NULL || input == NULL)
    {
        return 0;
    }

    attack_pressed = input->attack && !state->attack_held;
    use_pressed = input->use && !state->use_held;
    state->attack_held = input->attack != 0U;
    state->use_held = input->use != 0U;

    /* WL_PLAY.C updates doors and pushwalls before walking the actor list. */
    if (!WL_MoveDoors(level, 1U) || !WL_MovePushWalls(level, 1U))
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

        speed = input->run ? WL_RUN_MOVE : WL_BASE_MOVE;
        control_x = ((input->right != 0U) - (input->left != 0U)) * speed;
        control_y = ((input->down != 0U) - (input->up != 0U)) * speed;
        if (!WL_ControlMovement(level, tables, control_x, control_y,
                                input->strafe != 0U)
            || (!attack_started
                && !WL_TickPlayerAttack(level, 1U, input->attack != 0U)))
        {
            return 0;
        }
    }

    return WL_TickActors(level, 1U);
}
