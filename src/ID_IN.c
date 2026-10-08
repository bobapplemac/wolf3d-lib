/* Portable joystick portion of the original ID_IN.C input manager. */
#include "ID_IN.h"

#include <string.h>

/* IN_SetupJoy divided each calibrated axis into outer thirds around a large
   central dead zone. INL_GetJoyDelta then scaled either outer third to 0..127. */
#define ID_IN_JOY_THRESHOLD 21845
#define ID_IN_JOY_POSITIVE_RANGE (INT16_MAX - ID_IN_JOY_THRESHOLD)
#define ID_IN_JOY_NEGATIVE_RANGE (32768 - ID_IN_JOY_THRESHOLD)

static id_in_joystick_t ID_IN_Joysticks[ID_IN_MAX_JOYSTICKS];

void ID_IN_ResetJoysticks(void)
{
    memset(ID_IN_Joysticks, 0, sizeof(ID_IN_Joysticks));
}

int ID_IN_SetJoystick(unsigned joystick, int connected,
                      int16_t x, int16_t y, uint32_t buttons)
{
    id_in_joystick_t *state;

    if (joystick >= ID_IN_MAX_JOYSTICKS)
    {
        return 0;
    }
    state = &ID_IN_Joysticks[joystick];
    state->connected = connected != 0;
    state->x = state->connected ? x : 0;
    state->y = state->connected ? y : 0;
    state->buttons = state->connected
                         ? (uint8_t)(buttons & 0x0fU) : 0U;
    return 1;
}

int ID_IN_JoystickPresent(unsigned joystick)
{
    return joystick < ID_IN_MAX_JOYSTICKS
        && ID_IN_Joysticks[joystick].connected != 0U;
}

uint8_t ID_IN_JoyButtons(unsigned joystick)
{
    return ID_IN_JoystickPresent(joystick)
               ? ID_IN_Joysticks[joystick].buttons : 0U;
}

static int ID_IN_ScaleJoyAxis(int value)
{
    int scaled;

    if (value < -ID_IN_JOY_THRESHOLD)
    {
        scaled = (-value - ID_IN_JOY_THRESHOLD) * 127
                 / ID_IN_JOY_NEGATIVE_RANGE;
        return scaled > 127 ? -127 : -scaled;
    }
    if (value > ID_IN_JOY_THRESHOLD)
    {
        scaled = (value - ID_IN_JOY_THRESHOLD) * 127
                 / ID_IN_JOY_POSITIVE_RANGE;
        return scaled > 127 ? 127 : scaled;
    }
    return 0;
}

void ID_IN_GetJoyDelta(unsigned joystick, int *x, int *y)
{
    if (x == NULL || y == NULL)
    {
        return;
    }
    if (!ID_IN_JoystickPresent(joystick))
    {
        *x = 0;
        *y = 0;
        return;
    }
    *x = ID_IN_ScaleJoyAxis(ID_IN_Joysticks[joystick].x);
    *y = ID_IN_ScaleJoyAxis(ID_IN_Joysticks[joystick].y);
}
