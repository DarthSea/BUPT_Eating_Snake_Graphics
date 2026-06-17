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

/* 检测指定槽位（0/1）的手柄是否已连接 */
bool Input_gamepadConnected(int slot)
{
    if (slot < 0 || slot > 1) return false;
    memset(&gGamepads[slot].state, 0, sizeof(XINPUT_STATE));
    return XInputGetState((DWORD)slot, &gGamepads[slot].state) == ERROR_SUCCESS;
}

/* 检测手柄按钮是否从释放变为按下（上升沿检测） */
bool Input_gamepadButtonPressed(int slot, WORD buttonMask)
{
    GamepadState *g = &gGamepads[slot];
    bool now = (g->state.Gamepad.wButtons & buttonMask) != 0;
    bool prev = (g->prevButtons & buttonMask) != 0;
    return now && !prev;
}

/* 读取手柄左摇杆方向：带死区，优先取绝对值更大的轴 */
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
