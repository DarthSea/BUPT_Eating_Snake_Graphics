#include <windows.h>
#include <xinput.h>

#include "audio.h"
#include "game.h"
#include "input.h"
#include "input_gamepad.h"
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

MenuAction Ui_runWelcome(InputContext *input, RenderContext *render)
{
    int selected = 0;
    static int prevMouseX = -1, prevMouseY = -1;

    for (;;) {
        MenuInput menu;

        Input_updateMouse();
        Input_updateGamepads();

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

        /* 鼠标 hover — 仅鼠标移动时更新选择，不影响键盘 */
        {
            float ms = (float)render->windowWidth / 1280.0f;
            if (ms < 0.85f) ms = 0.85f;
            if (ms > 1.5f) ms = 1.5f;
            int btnH = (int)(58 * ms);
            int btnGap = (int)(10 * ms);
            int btnW = (int)(render->windowWidth * 0.48f);
            if (btnW > (int)(560 * ms)) btnW = (int)(560 * ms);
            int btnLeft = (render->windowWidth - btnW) / 2;
            int mouseMoved = (prevMouseX != gMouseX || prevMouseY != gMouseY);
            prevMouseX = gMouseX; prevMouseY = gMouseY;

            for (int i = 0; i < 7; i++) {
                int btnTop;
                if (i < 4) {
                    btnTop = (int)(170 * ms) + i * (btnH + btnGap);
                } else {
                    /* 次要按钮 */
                    int sW = (int)(render->windowWidth * 0.20f);
                    if (sW > (int)(200 * ms)) sW = (int)(200 * ms);
                    int sH = (int)(42 * ms);
                    int sGap = (int)(20 * ms);
                    int total = 3 * sW + 2 * sGap;
                    int subLeft = (render->windowWidth - total) / 2 + (i - 4) * (sW + sGap);
                    btnTop = (int)(170 * ms) + 4 * (btnH + btnGap) + (int)(20 * ms);
                    if (mouseMoved && Input_mouseInRect(subLeft, btnTop, subLeft + sW, btnTop + sH)) {
                        selected = i;
                    }
                    continue;
                }
                if (mouseMoved && Input_mouseInRect(btnLeft, btnTop, btnLeft + btnW, btnTop + btnH)) {
                    selected = i;
                }
            }
            if (Input_mouseLeftClicked()) return (MenuAction)selected;
        }

        /* 手柄 */
        {
            Direction gpadDir = Input_gamepadConnected(0)
                ? Input_gamepadDirection(0) : DIR_NONE;
            if (gpadDir == DIR_UP) selected = wrapIndex(selected - 1, 7);
            if (gpadDir == DIR_DOWN) selected = wrapIndex(selected + 1, 7);
            if (Input_gamepadButtonPressed(0, XINPUT_GAMEPAD_A)
                || Input_gamepadButtonPressed(0, XINPUT_GAMEPAD_START)) {
                return (MenuAction)selected;
            }
            if (Input_gamepadButtonPressed(0, XINPUT_GAMEPAD_B)) return MENU_EXIT;
        }

        Sleep(16);
    }
}

bool Ui_chooseVariant(InputContext *input, RenderContext *render, MapVariant *variant)
{
    int selected = (int)(*variant);

    for (;;) {
        MenuInput menu;

        Input_updateMouse();
        Input_updateGamepads();

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

        /* 鼠标 hover */
        {
            int total = 2 * 260 + 1 * 24;
            int baseLeft = (render->windowWidth - total) / 2;
            for (int i = 0; i < 2; i++) {
                int btnLeft = baseLeft + i * (260 + 24);
                if (Input_mouseInRect(btnLeft, 330, btnLeft + 260, 330 + 56)) {
                    selected = i;
                }
            }
            if (Input_mouseLeftClicked()) {
                *variant = (MapVariant)selected;
                return true;
            }
        }

        /* 手柄 */
        {
            Direction gpadDir = Input_gamepadConnected(0)
                ? Input_gamepadDirection(0) : DIR_NONE;
            if (gpadDir == DIR_LEFT) selected = wrapIndex(selected - 1, 2);
            if (gpadDir == DIR_RIGHT) selected = wrapIndex(selected + 1, 2);
            if (Input_gamepadButtonPressed(0, XINPUT_GAMEPAD_A)) {
                *variant = (MapVariant)selected;
                return true;
            }
            if (Input_gamepadButtonPressed(0, XINPUT_GAMEPAD_B)) return false;
        }

        Sleep(16);
    }
}

bool Ui_chooseControls(InputContext *input, RenderContext *render, GameConfig *config)
{
    int p1Sel = (int)config->p1ControlMethod;
    int p2Sel = (int)config->p2ControlMethod;

    for (;;) {
        MenuInput menu;

        Input_updateMouse();
        Input_updateGamepads();

        /* P1 输入 */
        Input_readMenu(input, &menu);
        if (menu.move != 0) p1Sel = (p1Sel + menu.move + 5) % 5;
        if (menu.confirm) break;

        /* P2 输入：方向键 */
        {
            Direction p2dir = Input_readPlayer2Direction();
            if (p2dir == DIR_UP) p2Sel = (p2Sel - 1 + 5) % 5;
            if (p2dir == DIR_DOWN) p2Sel = (p2Sel + 1) % 5;
        }

        Render_drawControlSelect(render, p1Sel, p2Sel, config);
        Sleep(16);
    }

    config->p1ControlMethod = (ControlMethod)p1Sel;
    config->p2ControlMethod = (ControlMethod)p2Sel;
    return true;
}

bool Ui_chooseMapSize(InputContext *input, RenderContext *render, int *mapSize, bool isMulti)
{
    int sel = (*mapSize == 50) ? 1 : (*mapSize >= 100 ? 2 : 0);
    int maxOpt = isMulti ? 2 : 3; /* 多人只给20和50 */

    for (;;) {
        MenuInput menu;
        Input_updateMouse();
        Input_updateGamepads();

        Render_drawMapSizeMenu(sel, isMulti);
        Input_readMenu(input, &menu);
        if (menu.move != 0) sel = wrapIndex(sel + menu.move, maxOpt);
        if (menu.confirm) { *mapSize = (sel == 0) ? 20 : (sel == 1) ? 50 : 100; return true; }
        if (menu.cancel) return false;

        /* 手柄 */
        if (Input_gamepadConnected(0)) {
            Direction d = Input_gamepadDirection(0);
            if (d == DIR_UP) sel = wrapIndex(sel - 1, maxOpt);
            if (d == DIR_DOWN) sel = wrapIndex(sel + 1, maxOpt);
            if (Input_gamepadButtonPressed(0, 0x1000)) { *mapSize = (sel == 0) ? 20 : (sel == 1) ? 50 : 100; return true; }
        }
        Sleep(16);
    }
}

bool Ui_chooseControlsSingle(InputContext *input, RenderContext *render, GameConfig *config)
{
    int sel = (int)config->p1ControlMethod;

    for (;;) {
        MenuInput menu;

        Input_updateMouse();
        Input_updateGamepads();

        Render_drawControlSelectSingle(render, sel);

        /* 键盘 */
        Input_readMenu(input, &menu);
        if (menu.move != 0) sel = (sel + menu.move + 5) % 5;
        if (menu.confirm) { config->p1ControlMethod = (ControlMethod)sel; return true; }
        if (menu.cancel) return false;

        /* 鼠标 hover */
        {
            int colX = (render->windowWidth - 210) / 2;
            for (int i = 0; i < 5; i++) {
                int y = 195 + i * 52;
                if (Input_mouseInRect(colX, y, colX + 210, y + 42)) {
                    sel = i;
                }
            }
            if (Input_mouseLeftClicked()) {
                config->p1ControlMethod = (ControlMethod)sel;
                return true;
            }
        }

        /* 游戏手柄 */
        {
            Direction gpadDir = Input_gamepadConnected(0)
                ? Input_gamepadDirection(0) : DIR_NONE;
            if (gpadDir == DIR_UP) sel = (sel - 1 + 5) % 5;
            if (gpadDir == DIR_DOWN) sel = (sel + 1) % 5;
            if (Input_gamepadButtonPressed(0, XINPUT_GAMEPAD_A)) {
                config->p1ControlMethod = (ControlMethod)sel;
                return true;
            }
            if (Input_gamepadButtonPressed(0, XINPUT_GAMEPAD_B)) return false;
        }

        Sleep(16);
    }
}

bool Ui_chooseDifficulty(InputContext *input, RenderContext *render, AiDifficulty *difficulty)
{
    int selected = (int)(*difficulty);

    for (;;) {
        MenuInput menu;

        Input_updateMouse();
        Input_updateGamepads();

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

        /* 鼠标 hover */
        {
            int total = 3 * 260 + 2 * 24;
            int baseLeft = (render->windowWidth - total) / 2;
            for (int i = 0; i < 3; i++) {
                int btnLeft = baseLeft + i * (260 + 24);
                if (Input_mouseInRect(btnLeft, 330, btnLeft + 260, 330 + 56)) {
                    selected = i;
                }
            }
            if (Input_mouseLeftClicked()) {
                *difficulty = (AiDifficulty)selected;
                return true;
            }
        }

        /* 手柄 */
        {
            Direction gpadDir = Input_gamepadConnected(0)
                ? Input_gamepadDirection(0) : DIR_NONE;
            if (gpadDir == DIR_LEFT) selected = wrapIndex(selected - 1, 3);
            if (gpadDir == DIR_RIGHT) selected = wrapIndex(selected + 1, 3);
            if (Input_gamepadButtonPressed(0, XINPUT_GAMEPAD_A)) {
                *difficulty = (AiDifficulty)selected;
                return true;
            }
            if (Input_gamepadButtonPressed(0, XINPUT_GAMEPAD_B)) return false;
        }

        Sleep(16);
    }
}

bool Ui_chooseSkin(InputContext *input, RenderContext *render, int *skinId)
{
    int selected = *skinId;
    int count = Render_skinCount();

    for (;;) {
        MenuInput menu;

        Input_updateMouse();
        Input_updateGamepads();

        Render_drawSkinMenu(selected);
        Input_readMenu(input, &menu);
        if (menu.move != 0) {
            selected = wrapIndex(selected + menu.move, count);
        }
        if (menu.left || menu.right) {
            selected = wrapIndex(selected + (menu.left ? -1 : 1), count);
        }
        if (menu.confirm) {
            *skinId = selected;
            Render_loadSkin(render, selected);
            return true;
        }
        if (menu.cancel) {
            return false;
        }

        /* 鼠标 hover（使用菜单按钮位置） */
        {
            int btnLeft = (render->windowWidth - 390) / 2;
            for (int i = 0; i < count; i++) {
                int btnTop = 190 + i * 58;
                if (Input_mouseInRect(btnLeft, btnTop, btnLeft + 390, btnTop + 50)) {
                    selected = i;
                }
            }
            if (Input_mouseLeftClicked()) {
                *skinId = selected;
                Render_loadSkin(render, selected);
                return true;
            }
        }

        /* 手柄 */
        {
            Direction gpadDir = Input_gamepadConnected(0)
                ? Input_gamepadDirection(0) : DIR_NONE;
            if (gpadDir == DIR_UP) selected = wrapIndex(selected - 1, count);
            if (gpadDir == DIR_DOWN) selected = wrapIndex(selected + 1, count);
            if (Input_gamepadButtonPressed(0, XINPUT_GAMEPAD_A)) {
                *skinId = selected;
                Render_loadSkin(render, selected);
                return true;
            }
            if (Input_gamepadButtonPressed(0, XINPUT_GAMEPAD_B)) return false;
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

        Input_updateMouse();
        Input_updateGamepads();

        Render_drawSettings(settings, selectedRow);
        Input_readMenu(input, &menu);
        if (menu.move != 0) {
            selectedRow = wrapIndex(selectedRow + menu.move, 6);
        }

        step = menu.left ? -1 : (menu.right ? 1 : 0);

        /* 手柄左右切换设置值 */
        {
            Direction gpadDir = Input_gamepadConnected(0)
                ? Input_gamepadDirection(0) : DIR_NONE;
            if (gpadDir == DIR_LEFT) step = -1;
            if (gpadDir == DIR_RIGHT) step = 1;
            if (gpadDir == DIR_UP) selectedRow = wrapIndex(selectedRow - 1, 6);
            if (gpadDir == DIR_DOWN) selectedRow = wrapIndex(selectedRow + 1, 6);
            if (Input_gamepadButtonPressed(0, XINPUT_GAMEPAD_A)) return true;
            if (Input_gamepadButtonPressed(0, XINPUT_GAMEPAD_B)) return true;
        }

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

        /* 鼠标 hover */
        {
            int rowWidth = render->windowWidth - 420;
            for (int i = 0; i < 6; i++) {
                int rowTop = 200 + i * 62;
                if (Input_mouseInRect(210, rowTop, 210 + rowWidth, rowTop + 48)) {
                    selectedRow = i;
                }
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

static void computeViewport(RenderContext *render, GameState *state,
    int *outStartRow, int *outStartCol, int *outCellSize)
{
    int mapSize = Game_validMapSize(state->config.mapSize);
    int cellSize = Render_cellSizeForMap(render, mapSize);
    int visibleCells = render->boardPixelSize / cellSize;
    if (visibleCells > mapSize) visibleCells = mapSize;
    if (visibleCells < 1) visibleCells = 1;

    if (state->player.length > 0) {
        Pos head = state->player.body[0];
        int startRow = head.row - visibleCells / 2;
        int startCol = head.col - visibleCells / 2;
        if (startRow < 0) startRow = 0;
        if (startRow + visibleCells > mapSize) startRow = mapSize - visibleCells;
        if (startRow < 0) startRow = 0;
        if (startCol < 0) startCol = 0;
        if (startCol + visibleCells > mapSize) startCol = mapSize - visibleCells;
        if (startCol < 0) startCol = 0;
        *outStartRow = startRow;
        *outStartCol = startCol;
    } else {
        *outStartRow = 0;
        *outStartCol = 0;
    }
    *outCellSize = cellSize;
}

static Direction readP1Direction(GameState *state, RenderContext *render)
{
    if (state->config.p1ControlMethod == CONTROL_MOUSE) {
        int startRow, startCol, cellSize;
        computeViewport(render, state, &startRow, &startCol, &cellSize);
        Pos head;
        head.row = state->player.length > 0 ? state->player.body[0].row : 0;
        head.col = state->player.length > 0 ? state->player.body[0].col : 0;
        return Input_readMouseDirection(head, startRow, startCol, cellSize);
    } else if (state->config.p1ControlMethod == CONTROL_GAMEPAD_1) {
        return Input_gamepadDirection(0);
    } else if (state->config.p1ControlMethod == CONTROL_GAMEPAD_2) {
        return Input_gamepadDirection(1);
    } else {
        return Input_readPlayerDirection();
    }
}

static Direction readP2Direction(GameState *state, RenderContext *render)
{
    if (state->config.p2ControlMethod == CONTROL_MOUSE) {
        int startRow, startCol, cellSize;
        computeViewport(render, state, &startRow, &startCol, &cellSize);
        Pos head;
        head.row = state->ai.length > 0 ? state->ai.body[0].row : 0;
        head.col = state->ai.length > 0 ? state->ai.body[0].col : 0;
        return Input_readMouseDirection(head, startRow, startCol, cellSize);
    } else if (state->config.p2ControlMethod == CONTROL_GAMEPAD_1) {
        return Input_gamepadDirection(0);
    } else if (state->config.p2ControlMethod == CONTROL_GAMEPAD_2) {
        return Input_gamepadDirection(1);
    } else if (state->config.p2ControlMethod == CONTROL_KEYBOARD_ARROWS) {
        return Input_readPlayer2Direction();
    } else {
        return Input_readPlayerDirection();
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

        Input_updateMouse();
        Input_updateGamepads();

        dir = readP1Direction(state, render);
        if (isMulti) {
            dir2 = readP2Direction(state, render);
        } else {
            dir2 = DIR_NONE;
        }
        if (waitingForStart) {
            bool p1Started = false;
            bool p2Started = false;

            /* P1 开始条件 */
            if (state->config.p1ControlMethod == CONTROL_KEYBOARD_WASD
                || state->config.p1ControlMethod == CONTROL_KEYBOARD_ARROWS) {
                if (dir != DIR_NONE) p1Started = true;
            }
            if (state->config.p1ControlMethod == CONTROL_MOUSE) {
                if (Input_mouseLeftClicked()) p1Started = true;
            }
            if (state->config.p1ControlMethod >= CONTROL_GAMEPAD_1) {
                int slot = (int)(state->config.p1ControlMethod - CONTROL_GAMEPAD_1);
                if (Input_gamepadButtonPressed(slot, XINPUT_GAMEPAD_RIGHT_SHOULDER)
                    || Input_gamepadDirection(slot) != DIR_NONE) {
                    p1Started = true;
                }
            }

            /* P2 开始条件 */
            if (isMulti) {
                if (state->config.p2ControlMethod == CONTROL_KEYBOARD_WASD
                    || state->config.p2ControlMethod == CONTROL_KEYBOARD_ARROWS) {
                    if (dir2 != DIR_NONE) p2Started = true;
                }
                if (state->config.p2ControlMethod == CONTROL_MOUSE) {
                    if (Input_mouseLeftClicked()) p2Started = true;
                }
                if (state->config.p2ControlMethod >= CONTROL_GAMEPAD_1) {
                    int slot = (int)(state->config.p2ControlMethod - CONTROL_GAMEPAD_1);
                    if (Input_gamepadButtonPressed(slot, XINPUT_GAMEPAD_RIGHT_SHOULDER)
                        || Input_gamepadDirection(slot) != DIR_NONE) {
                        p2Started = true;
                    }
                }
            }

            bool started = isMulti ? (p1Started && p2Started) : p1Started;

            if (started) {
                waitingForStart = false;
                Game_preparePlayerStart(state, dir != DIR_NONE ? dir : DIR_RIGHT);
                if (isMulti) Game_prepareP2Start(state, dir2 != DIR_NONE ? dir2 : DIR_LEFT);
                Game_setPlayerDirection(state, dir != DIR_NONE ? dir : DIR_RIGHT);
                if (isMulti) Game_setP2Direction(state, dir2 != DIR_NONE ? dir2 : DIR_LEFT);
                Audio_playEvent(SOUND_START);
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
        /* 手柄 RB 射箭 P1 */
        if (!waitingForStart
            && state->config.p1ControlMethod >= CONTROL_GAMEPAD_1) {
            int slot = (int)(state->config.p1ControlMethod - CONTROL_GAMEPAD_1);
            if (Input_gamepadButtonPressed(slot, XINPUT_GAMEPAD_RIGHT_SHOULDER)) {
                Game_playerFireArrow(state);
                playPendingSounds(state);
            }
        }
        /* 手柄 RB 射箭 P2 */
        if (!waitingForStart && isMulti
            && state->config.p2ControlMethod >= CONTROL_GAMEPAD_1) {
            int slot = (int)(state->config.p2ControlMethod - CONTROL_GAMEPAD_1);
            if (Input_gamepadButtonPressed(slot, XINPUT_GAMEPAD_RIGHT_SHOULDER)) {
                Game_player2FireArrow(state);
                playPendingSounds(state);
            }
        }
        /* 鼠标左键射箭 */
        if (!waitingForStart && Input_mouseLeftClicked()) {
            if (state->config.p1ControlMethod == CONTROL_MOUSE) {
                Game_playerFireArrow(state);
                playPendingSounds(state);
            } else if (isMulti && state->config.p2ControlMethod == CONTROL_MOUSE) {
                Game_player2FireArrow(state);
                playPendingSounds(state);
            }
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

        Render_particlesUpdate(deltaMs);
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

            Input_updateMouse();
            Input_updateGamepads();

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

            /* 鼠标 hover */
            {
                int total = 2 * 260 + 1 * 24;
                int baseLeft = (render->windowWidth - total) / 2;
                for (int i = 0; i < 2; i++) {
                    int btnLeft = baseLeft + i * (260 + 24);
                    if (Input_mouseInRect(btnLeft, 330, btnLeft + 260, 330 + 56)) {
                        selectedAction = i;
                    }
                }
                if (Input_mouseLeftClicked()) {
                    if (selectedAction == 0) {
                        Game_init(state, &config);
                        break;
                    } else {
                        return true;
                    }
                }
            }

            /* 手柄 */
            {
                Direction gpadDir = Input_gamepadConnected(0)
                    ? Input_gamepadDirection(0) : DIR_NONE;
                if (gpadDir == DIR_LEFT) selectedAction = wrapIndex(selectedAction - 1, 2);
                if (gpadDir == DIR_RIGHT) selectedAction = wrapIndex(selectedAction + 1, 2);
                if (Input_gamepadButtonPressed(0, XINPUT_GAMEPAD_A)) {
                    if (selectedAction == 0) {
                        Game_init(state, &config);
                        break;
                    } else {
                        return true;
                    }
                }
                if (Input_gamepadButtonPressed(0, XINPUT_GAMEPAD_B)) return true;
            }

            Sleep(16);
        }
    }
}
