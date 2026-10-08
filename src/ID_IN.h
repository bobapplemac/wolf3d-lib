#ifndef ID_IN_H
#define ID_IN_H

#include <stdint.h>

#define ID_IN_MAX_JOYSTICKS 2U
#define ID_IN_JOYSTICK_BUTTONS 4U

typedef struct id_in_joystick
{
    int16_t x;
    int16_t y;
    uint8_t buttons;
    uint8_t connected;
} id_in_joystick_t;

void ID_IN_ResetJoysticks(void);
int ID_IN_SetJoystick(unsigned joystick, int connected,
                      int16_t x, int16_t y, uint32_t buttons);
int ID_IN_JoystickPresent(unsigned joystick);
uint8_t ID_IN_JoyButtons(unsigned joystick);
void ID_IN_GetJoyDelta(unsigned joystick, int *x, int *y);

#endif
