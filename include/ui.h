#ifndef BUPT_SNAKE_UI_H
#define BUPT_SNAKE_UI_H

#include <stdbool.h>
#include "common.h"
#include "input.h"
#include "render.h"

typedef enum MenuAction {
    MENU_SINGLE = 0,
    MENU_AI_BATTLE,
    MENU_TIME_CHALLENGE,
    MENU_MULTIPLAYER,
    MENU_CHANGE_SKIN,
    MENU_SETTINGS,
    MENU_EXIT
} MenuAction;

MenuAction Ui_runWelcome(InputContext *input);
bool Ui_chooseVariant(InputContext *input, MapVariant *variant);
bool Ui_chooseDifficulty(InputContext *input, AiDifficulty *difficulty);
bool Ui_chooseSkin(InputContext *input, RenderContext *render, int *skinId);
bool Ui_runSettings(InputContext *input, RenderContext *render, GameConfig *settings);
bool Ui_runGame(InputContext *input, RenderContext *render, GameState *state);

#endif
