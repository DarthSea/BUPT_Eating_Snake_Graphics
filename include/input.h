#ifndef BUPT_SNAKE_INPUT_H
#define BUPT_SNAKE_INPUT_H

#include <stdbool.h>
#include "common.h"

typedef struct InputContext {
    bool previousKeyDown[256];
} InputContext;

typedef struct MenuInput {
    int move;
    bool left;
    bool right;
    bool confirm;
    bool cancel;
    bool restart;
    bool pause;
    bool fire;
    bool speedUp;
    bool speedDown;
    bool p2Fire;
} MenuInput;

void Input_init(InputContext *input);
Direction Input_readPlayerDirection(void);
Direction Input_readPlayer2Direction(void);
void Input_readMenu(InputContext *input, MenuInput *out);

#endif
