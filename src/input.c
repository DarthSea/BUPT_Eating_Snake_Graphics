#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <graphics.h>

#include "input.h"

/* 检查虚拟键是否当前处于按下状态 */
static bool isKeyDown(int virtualKey)
{
    return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
}

/* 检测按键从释放到按下的沿触发（避免每帧重复触发） */
static bool keyPressed(InputContext *input, int virtualKey)
{
    bool down;
    bool pressed;

    if (virtualKey < 0 || virtualKey >= 256) {
        return false;
    }

    down = isKeyDown(virtualKey);
    pressed = down && !input->previousKeyDown[virtualKey];
    input->previousKeyDown[virtualKey] = down;

    return pressed;
}

/* 清空输入上下文（重置 keyDown 历史） */
void Input_init(InputContext *input)
{
    memset(input, 0, sizeof(*input));
}

/* 读取玩家一方向：WASD 按键映射到上下左右 */
Direction Input_readPlayerDirection(void)
{
    if (isKeyDown('W')) {
        return DIR_UP;
    }
    if (isKeyDown('S')) {
        return DIR_DOWN;
    }
    if (isKeyDown('A')) {
        return DIR_LEFT;
    }
    if (isKeyDown('D')) {
        return DIR_RIGHT;
    }

    return DIR_NONE;
}

/* 读取玩家二方向：方向键映射到上下左右 */
Direction Input_readPlayer2Direction(void)
{
    if (isKeyDown(VK_UP)) {
        return DIR_UP;
    }
    if (isKeyDown(VK_DOWN)) {
        return DIR_DOWN;
    }
    if (isKeyDown(VK_LEFT)) {
        return DIR_LEFT;
    }
    if (isKeyDown(VK_RIGHT)) {
        return DIR_RIGHT;
    }

    return DIR_NONE;
}

/* 读取菜单操作：移动、确认、取消、射箭、调速等所有菜单输入 */
void Input_readMenu(InputContext *input, MenuInput *out)
{
    bool upPressed;
    bool downPressed;
    bool leftPressed;
    bool rightPressed;

    memset(out, 0, sizeof(*out));

    upPressed = keyPressed(input, 'W');
    upPressed = keyPressed(input, VK_UP) || upPressed;
    downPressed = keyPressed(input, 'S');
    downPressed = keyPressed(input, VK_DOWN) || downPressed;
    leftPressed = keyPressed(input, 'A');
    leftPressed = keyPressed(input, VK_LEFT) || leftPressed;
    rightPressed = keyPressed(input, 'D');
    rightPressed = keyPressed(input, VK_RIGHT) || rightPressed;

    if (upPressed) {
        out->move = -1;
    } else if (downPressed) {
        out->move = 1;
    }

    out->left = leftPressed;
    out->right = rightPressed;
    out->confirm = keyPressed(input, VK_RETURN);
    out->confirm = keyPressed(input, VK_SPACE) || out->confirm;
    out->cancel = keyPressed(input, VK_ESCAPE);
    out->restart = keyPressed(input, 'R');
    out->pause = keyPressed(input, 'P');
    out->fire = keyPressed(input, 'E');
    out->speedUp = keyPressed(input, '1') || keyPressed(input, VK_NUMPAD1);
    out->speedDown = keyPressed(input, '2') || keyPressed(input, VK_NUMPAD2);
    out->p2Fire = keyPressed(input, VK_OEM_2);
}

/* ================================================================
 * Mouse input
 * ================================================================ */

int gMouseX = 0;
int gMouseY = 0;
static bool gMouseLeftPressed = false;
static bool gMouseRightPressed = false;
static bool gMouseLeftDown = false;
static bool gMouseRightDown = false;

/* 更新鼠标全局状态：坐标、左右键按下/释放沿 */
void Input_updateMouse(void)
{
    gMouseLeftPressed = false;
    gMouseRightPressed = false;
    while (MouseHit()) {
        MOUSEMSG msg = GetMouseMsg();
        gMouseX = msg.x;
        gMouseY = msg.y;
        if (msg.uMsg == WM_LBUTTONDOWN) { gMouseLeftPressed = true; gMouseLeftDown = true; }
        if (msg.uMsg == WM_LBUTTONUP) { gMouseLeftDown = false; }
        if (msg.uMsg == WM_RBUTTONDOWN) { gMouseRightPressed = true; gMouseRightDown = true; }
        if (msg.uMsg == WM_RBUTTONUP) { gMouseRightDown = false; }
    }
}

/* 判断鼠标坐标是否在指定矩形区域内 */
bool Input_mouseInRect(int left, int top, int right, int bottom)
{
    return gMouseX >= left && gMouseX <= right && gMouseY >= top && gMouseY <= bottom;
}

bool Input_mouseLeftClicked(void) { return gMouseLeftPressed; }
bool Input_mouseRightClicked(void) { return gMouseRightPressed; }

/* 根据鼠标与蛇头相对位置计算方向：用于鼠标操控模式 */
Direction Input_readMouseDirection(Pos snakeHead, int startRow, int startCol, int cellSize)
{
    int boardX = gMouseX - BOARD_LEFT;
    int boardY = gMouseY - BOARD_TOP;
    if (boardX < 0 || boardY < 0) return DIR_NONE;
    int headScreenX = BOARD_LEFT + (snakeHead.col - startCol) * cellSize + cellSize / 2;
    int headScreenY = BOARD_TOP + (snakeHead.row - startRow) * cellSize + cellSize / 2;
    int dx = gMouseX - headScreenX;
    int dy = gMouseY - headScreenY;
    int threshold = cellSize / 2;
    if (abs(dx) < threshold && abs(dy) < threshold) return DIR_NONE;
    if (abs(dx) > abs(dy)) {
        return dx > 0 ? DIR_RIGHT : DIR_LEFT;
    } else {
        return dy < 0 ? DIR_UP : DIR_DOWN;
    }
}
