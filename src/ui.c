#include <windows.h>

#include "audio.h"
#include "game.h"
#include "input.h"
#include "render.h"
#include "ui.h"

static int wrapIndex(int value, int count)
{
    if (value < 0) {
        return count - 1;
    }
    if (value >= count) {
        return 0;
    }

    return value;
}

MenuAction Ui_runWelcome(InputContext *input)
{
    int selected = 0;

    for (;;) {
        MenuInput menu;

        Render_drawWelcome(selected);
        Input_readMenu(input, &menu);
        if (menu.move != 0) {
            selected = wrapIndex(selected + menu.move, 7);
        }
        if (menu.confirm) {
            return (MenuAction)selected;
        }
        if (menu.cancel) {
            return MENU_EXIT;
        }

        Sleep(16);
    }
}

bool Ui_chooseVariant(InputContext *input, MapVariant *variant)
{
    int selected = (int)(*variant);

    for (;;) {
        MenuInput menu;

        Render_drawVariantMenu((MapVariant)selected);
        Input_readMenu(input, &menu);
        if (menu.move != 0 || menu.left || menu.right) {
            int step = menu.move != 0 ? menu.move : (menu.left ? -1 : 1);
            selected = wrapIndex(selected + step, 2);
        }
        if (menu.confirm) {
            *variant = (MapVariant)selected;
            return true;
        }
        if (menu.cancel) {
            return false;
        }

        Sleep(16);
    }
}

bool Ui_chooseDifficulty(InputContext *input, AiDifficulty *difficulty)
{
    int selected = (int)(*difficulty);

    for (;;) {
        MenuInput menu;

        Render_drawDifficultyMenu((AiDifficulty)selected);
        Input_readMenu(input, &menu);
        if (menu.move != 0 || menu.left || menu.right) {
            int step = menu.move != 0 ? menu.move : (menu.left ? -1 : 1);
            selected = wrapIndex(selected + step, 3);
        }
        if (menu.confirm) {
            *difficulty = (AiDifficulty)selected;
            return true;
        }
        if (menu.cancel) {
            return false;
        }

        Sleep(16);
    }
}

bool Ui_chooseSkin(InputContext *input, RenderContext *render, int *skinId)
{
    int selected = *skinId;

    for (;;) {
        MenuInput menu;

        Render_drawSkinMenu(selected);
        Input_readMenu(input, &menu);
        if (menu.move != 0) {
            selected = wrapIndex(selected + menu.move, Render_skinCount());
        }
        if (menu.left || menu.right) {
            selected = wrapIndex(selected + (menu.left ? -1 : 1), Render_skinCount());
        }
        if (menu.confirm) {
            *skinId = selected;
            Render_loadSkin(render, selected);
            return true;
        }
        if (menu.cancel) {
            return false;
        }

        Sleep(16);
    }
}

static int nextMapSize(int mapSize, int step)
{
    static const int sizes[] = { 20, 50, 100 };
    int index = 0;
    int i;

    for (i = 0; i < 3; i++) {
        if (sizes[i] == mapSize) {
            index = i;
            break;
        }
    }

    return sizes[wrapIndex(index + step, 3)];
}

static DisplayResolution nextResolution(DisplayResolution resolution, int step)
{
    int selected = (int)resolution;

    if (selected < 0 || selected > (int)RESOLUTION_QHD) {
        selected = (int)RESOLUTION_HD_PLUS;
    }

    return (DisplayResolution)wrapIndex(selected + step, 4);
}

bool Ui_runSettings(InputContext *input, RenderContext *render, GameConfig *settings)
{
    int selectedRow = 0;

    settings->mapSize = Game_validMapSize(settings->mapSize);
    settings->resolution = nextResolution(settings->resolution, 0);
    for (;;) {
        MenuInput menu;
        int step;

        Render_drawSettings(settings, selectedRow);
        Input_readMenu(input, &menu);
        if (menu.move != 0) {
            selectedRow = wrapIndex(selectedRow + menu.move, 6);
        }

        step = menu.left ? -1 : (menu.right ? 1 : 0);
        if (step != 0) {
            if (selectedRow == 0) {
                settings->enableStepGrowth = !settings->enableStepGrowth;
                settings->growthInterval = settings->enableStepGrowth
                    ? DEFAULT_GROWTH_INTERVAL
                    : 0;
            } else if (selectedRow == 1) {
                settings->mapSize = nextMapSize(settings->mapSize, step == 0 ? 1 : step);
            } else if (selectedRow == 2) {
                settings->resolution = nextResolution(settings->resolution, step);
                Render_applyDisplayMode(render, settings->resolution, settings->fullscreen);
            } else if (selectedRow == 3) {
                settings->fullscreen = !settings->fullscreen;
                Render_applyDisplayMode(render, settings->resolution, settings->fullscreen);
            } else if (selectedRow == 4) {
                settings->musicEnabled = !settings->musicEnabled;
                Audio_setMusicEnabled(settings->musicEnabled);
            } else if (selectedRow == 5) {
                settings->soundEnabled = !settings->soundEnabled;
                Audio_setSoundEnabled(settings->soundEnabled);
            }
        }

        if (menu.cancel || menu.confirm) {
            return true;
        }
        if (menu.restart) {
            return true;
        }

        Sleep(16);
    }
}

static void playPendingSounds(GameState *state)
{
    SoundEvent events[MAX_SOUND_EVENTS];
    int count = Game_consumeSoundEvents(state, events, MAX_SOUND_EVENTS);
    int i;

    for (i = 0; i < count; i++) {
        Audio_playEvent(events[i]);
    }
}

static bool runOneRound(InputContext *input, RenderContext *render, GameState *state)
{
    bool paused = false;
    bool waitingForStart = true;
    DWORD lastTick = GetTickCount();
    bool isMulti = (state->config.mode == MODE_LOCAL_MULTIPLAYER);

    while (!Game_isFinished(state)) {
        DWORD now = GetTickCount();
        int deltaMs = (int)(now - lastTick);
        Direction dir;
        Direction dir2;
        MenuInput menu;

        lastTick = now;
        dir = Input_readPlayerDirection();
        if (isMulti) {
            dir2 = Input_readPlayer2Direction();
        } else {
            dir2 = DIR_NONE;
        }
        if (waitingForStart) {
            if (isMulti) {
                if (dir != DIR_NONE && dir2 != DIR_NONE) {
                    waitingForStart = false;
                    Game_preparePlayerStart(state, dir);
                    Game_prepareP2Start(state, dir2);
                    Game_setPlayerDirection(state, dir);
                    Game_setP2Direction(state, dir2);
                    Audio_playEvent(SOUND_START);
                }
            } else {
                if (dir != DIR_NONE) {
                    waitingForStart = false;
                    Game_preparePlayerStart(state, dir);
                    Game_setPlayerDirection(state, dir);
                    Audio_playEvent(SOUND_START);
                }
            }
        } else {
            Game_setPlayerDirection(state, dir);
            if (isMulti) {
                Game_setP2Direction(state, dir2);
            }
        }

        Input_readMenu(input, &menu);
        if (menu.pause) {
            paused = !paused;
        }
        if (!waitingForStart && menu.fire) {
            Game_playerFireArrow(state);
            playPendingSounds(state);
        }
        if (isMulti && !waitingForStart && menu.p2Fire) {
            Game_player2FireArrow(state);
            playPendingSounds(state);
        }
        if (!isMulti && !waitingForStart && menu.speedUp) {
            Game_adjustSpeed(state, 1);
        }
        if (!isMulti && !waitingForStart && menu.speedDown) {
            Game_adjustSpeed(state, -1);
        }
        if (menu.cancel) {
            return false;
        }
        if (!paused && !waitingForStart) {
            Game_update(state, deltaMs);
            playPendingSounds(state);
        }

        Render_drawGame(render, state, paused, waitingForStart);
        Sleep(10);
    }

    return true;
}

bool Ui_runGame(InputContext *input, RenderContext *render, GameState *state)
{
    GameConfig config = state->config;

    for (;;) {
        int selectedAction = 0;

        if (!runOneRound(input, render, state)) {
            return true;
        }

        for (;;) {
            MenuInput menu;

            Render_drawGameOver(state, selectedAction);
            Input_readMenu(input, &menu);
            if (menu.move != 0 || menu.left || menu.right) {
                int step = menu.move != 0 ? menu.move : (menu.left ? -1 : 1);
                selectedAction = wrapIndex(selectedAction + step, 2);
            }
            if (menu.restart || (menu.confirm && selectedAction == 0)) {
                Game_init(state, &config);
                break;
            }
            if (menu.cancel || (menu.confirm && selectedAction == 1)) {
                return true;
            }

            Sleep(16);
        }
    }
}
