#ifndef BUPT_SNAKE_INPUT_GAMEPAD_H
#define BUPT_SNAKE_INPUT_GAMEPAD_H

#include "common.h"
#include <xinput.h>

bool Input_gamepadConnected(int slot);
bool Input_gamepadButtonPressed(int slot, WORD buttonMask);
Direction Input_gamepadDirection(int slot);
void Input_updateGamepads(void);

#endif
