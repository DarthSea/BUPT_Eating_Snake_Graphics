#include "input_gamepad.h"
#include <string.h>
#include <xinput.h>

#pragma comment(lib, "Xinput.lib")

typedef struct GamepadState {
    XINPUT_STATE state;
    WORD prevButtons;
    bool prevConnected;
} GamepadState;

static GamepadState gGamepads[2];

bool Input_gamepadConnected(int slot)
{
    if (slot < 0 || slot > 1) return false;
    memset(&gGamepads[slot].state, 0, sizeof(XINPUT_STATE));
    return XInputGetState((DWORD)slot, &gGamepads[slot].state) == ERROR_SUCCESS;
}

bool Input_gamepadButtonPressed(int slot, WORD buttonMask)
{
    GamepadState *g = &gGamepads[slot];
    bool now = (g->state.Gamepad.wButtons & buttonMask) != 0;
    bool prev = (g->prevButtons & buttonMask) != 0;
    return now && !prev;
}

Direction Input_gamepadDirection(int slot)
{
    GamepadState *g = &gGamepads[slot];
    SHORT lx = g->state.Gamepad.sThumbLX;
    SHORT ly = g->state.Gamepad.sThumbLY;
    int deadZone = 10000;

    if (abs(lx) < deadZone && abs(ly) < deadZone) return DIR_NONE;
    if (abs(lx) > abs(ly)) {
        return lx > 0 ? DIR_RIGHT : DIR_LEFT;
    } else {
        return ly < 0 ? DIR_UP : DIR_DOWN;
    }
}

/* 每帧调用一次，保存上一帧按钮状态 */
void Input_updateGamepads(void)
{
    int i;
    for (i = 0; i < 2; i++) {
        GamepadState *g = &gGamepads[i];
        g->prevButtons = g->state.Gamepad.wButtons;
        XInputGetState((DWORD)i, &g->state);
    }
}
