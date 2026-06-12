#include <string.h>
#include <windows.h>

#include "input.h"

static bool isKeyDown(int virtualKey)
{
    return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
}

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

void Input_init(InputContext *input)
{
    memset(input, 0, sizeof(*input));
}

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
