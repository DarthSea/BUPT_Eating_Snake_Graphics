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

MenuAction Ui_runWelcome(InputContext *input, RenderContext *render);
bool Ui_chooseVariant(InputContext *input, RenderContext *render, MapVariant *variant);
bool Ui_chooseDifficulty(InputContext *input, RenderContext *render, AiDifficulty *difficulty);
bool Ui_chooseSkin(InputContext *input, RenderContext *render, int *skinId);
bool Ui_runSettings(InputContext *input, RenderContext *render, GameConfig *settings);
bool Ui_chooseMapSize(InputContext *input, RenderContext *render, int *mapSize, bool isMulti);
bool Ui_chooseControls(InputContext *input, RenderContext *render, GameConfig *config);
bool Ui_chooseControlsSingle(InputContext *input, RenderContext *render, GameConfig *config);
bool Ui_chooseControlsP2(InputContext *input, RenderContext *render, GameConfig *config);
bool Ui_runGame(InputContext *input, RenderContext *render, GameState *state);

#endif
