#include <stdio.h>
#include <windows.h>

#include "audio.h"
#include "game.h"
#include "input.h"
#include "render.h"
#include "ui.h"

static GameState gState;

static LONG WINAPI handleCrash(EXCEPTION_POINTERS *exceptionInfo)
{
    FILE *file = fopen("snake_crash.log", "a");

    if (file != NULL) {
        fprintf(file, "Unhandled exception: code=0x%08lX address=%p\n",
            exceptionInfo->ExceptionRecord->ExceptionCode,
            exceptionInfo->ExceptionRecord->ExceptionAddress);
        fclose(file);
    }

    MessageBoxA(NULL,
        "The game crashed. Please check snake_crash.log in the running directory.",
        "BUPT Snake",
        MB_OK | MB_ICONERROR);
    return EXCEPTION_EXECUTE_HANDLER;
}

int main(void)
{
    RenderContext render;
    InputContext input;
    GameConfig settings;
    GameConfig config;
    int skinId = 0;
    bool running = true;

    SetUnhandledExceptionFilter(handleCrash);
    Game_makeDefaultConfig(&settings);
    config = settings;
    if (!Render_init(&render, skinId)) {
        MessageBoxA(NULL, "EasyX window initialization failed.", "BUPT Snake", MB_OK | MB_ICONERROR);
        return 1;
    }
    Audio_init(settings.musicEnabled, settings.soundEnabled);
    Input_init(&input);

    while (running) {
        MenuAction action = Ui_runWelcome(&input);

        if (action == MENU_EXIT) {
            running = false;
        } else if (action == MENU_CHANGE_SKIN) {
            Ui_chooseSkin(&input, &render, &skinId);
            settings.skinId = skinId;
        } else if (action == MENU_SETTINGS) {
            Ui_runSettings(&input, &render, &settings);
            skinId = settings.skinId;
        } else {
            GameState *state = &gState;
            MapVariant variant;

            config = settings;
            config.skinId = skinId;

            if (action == MENU_SINGLE) {
                Game_applyModeDefaults(&config, MODE_SINGLE);
            } else if (action == MENU_AI_BATTLE) {
                Game_applyModeDefaults(&config, MODE_AI_BATTLE);
            } else if (action == MENU_TIME_CHALLENGE) {
                Game_applyModeDefaults(&config, MODE_TIME_CHALLENGE);
                config.variant = VARIANT_DIVERSE;
            } else if (action == MENU_MULTIPLAYER) {
                Game_applyModeDefaults(&config, MODE_LOCAL_MULTIPLAYER);
                config.variant = VARIANT_DIVERSE;
                /* 跳过变体和难度选择，直接开始 */
                Render_loadSkin(&render, skinId);
                Game_init(state, &config);
                Ui_runGame(&input, &render, state);
                continue;
            } else {
                continue;
            }

            variant = config.variant;
            if (!Ui_chooseVariant(&input, &variant)) {
                continue;
            }
            config.variant = variant;

            if (config.mode == MODE_AI_BATTLE
                && !Ui_chooseDifficulty(&input, &config.aiDifficulty)) {
                continue;
            }

            Render_loadSkin(&render, skinId);
            Game_init(state, &config);
            Ui_runGame(&input, &render, state);
        }
    }

    Audio_shutdown();
    Render_shutdown();
    return 0;
}
