#include <stdio.h>
#include <windows.h>

#include "audio.h"
#include "game.h"
#include "input.h"
#include "render.h"
#include "ui.h"

static GameState gState;

/* 全局异常捕获：崩溃时写入日志并弹窗提示 */
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

/* 游戏入口：初始化渲染/音频/输入，循环主菜单到游戏流程 */
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
        MenuAction action = Ui_runWelcome(&input, &render);

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
                config.p1ControlMethod = CONTROL_KEYBOARD_WASD;
                Ui_chooseControlsSingle(&input, &render, &config);
            } else if (action == MENU_AI_BATTLE) {
                Game_applyModeDefaults(&config, MODE_AI_BATTLE);
                config.p1ControlMethod = CONTROL_KEYBOARD_WASD;
                if (!Ui_chooseControlsSingle(&input, &render, &config)) {
                    continue;
                }
            } else if (action == MENU_TIME_CHALLENGE) {
                Game_applyModeDefaults(&config, MODE_TIME_CHALLENGE);
                config.variant = VARIANT_DIVERSE;
                config.p1ControlMethod = CONTROL_KEYBOARD_WASD;
                Ui_chooseControlsSingle(&input, &render, &config);
            } else if (action == MENU_MULTIPLAYER) {
                Game_applyModeDefaults(&config, MODE_LOCAL_MULTIPLAYER);
                config.variant = VARIANT_DIVERSE;

                /* P1 选择操控方式 */
                config.p1ControlMethod = CONTROL_KEYBOARD_WASD;
                if (!Ui_chooseControlsSingle(&input, &render, &config)) {
                    continue;
                }

                /* P2 选择操控方式 */
                config.p2ControlMethod = CONTROL_KEYBOARD_ARROWS;
                if (!Ui_chooseControlsP2(&input, &render, &config)) {
                    continue;
                }

                /* 地图大小选择 */
                {
                    int mapSz = config.mapSize > 50 ? 50 : config.mapSize;
                    if (!Ui_chooseMapSize(&input, &render, &mapSz, true)) {
                        continue;
                    }
                    config.mapSize = mapSz;
                }

                Render_loadSkin(&render, skinId);
                Game_init(state, &config);
                Ui_runGame(&input, &render, state);
                continue;
            } else {
                continue;
            }

            {
                int mapSz = config.mapSize;
                if (!Ui_chooseMapSize(&input, &render, &mapSz,
                    config.mode == MODE_LOCAL_MULTIPLAYER)) {
                    continue;
                }
                config.mapSize = mapSz;
            }

            variant = config.variant;
            if (!Ui_chooseVariant(&input, &render, &variant)) {
                continue;
            }
            config.variant = variant;

            if (config.mode == MODE_AI_BATTLE
                && !Ui_chooseDifficulty(&input, &render, &config.aiDifficulty)) {
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
