# 本地多人模式 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 新增本地双人对战模式，两人同屏对抗，90 秒限时，分高者胜，死亡即刻判负。P1 用 WASD+E 射箭，P2 用方向键+/ 射箭。道具池复用 AI 对战模式。

**Architecture:** 复用现有双蛇管线——`GameState.ai` 槽位作为玩家二，新增 `MODE_LOCAL_MULTIPLAYER` 分支，把 AI 决策替换为方向键输入。新增 `stepLocalMultiplayer` 步进函数（参考 `stepBattle` 但两侧均为人控）。渲染侧栏改为双人分数。

**Tech Stack:** C + EasyX + Win32 API + MSBuild

---

## 文件改动概览

| 文件 | 操作 | 职责 |
|------|------|------|
| `include/common.h` | 修改 | 新增 Mode/Result 枚举值、P2_INDEX 常量 |
| `include/input.h` | 修改 | 新增 P2 方向读取声明 |
| `src/input.c` | 修改 | 新增 P2 方向读取实现（方向键） |
| `src/game.c` | 修改 | 新增多人模式默认配置、初始化、步进、时间判定、射箭权限 |
| `src/ui.c` | 修改 | 欢迎菜单加选项、游戏循环处理双人输入 |
| `src/render_easyx.c` | 修改 | 侧栏双人分数、游戏结束 P1/P2 胜负、隐藏不适用信息 |
| `src/main.c` | 修改 | 菜单入口连线（跳过变体和难度选择） |
| `src/common.c` | 修改 | 模式名/结果名字符串 |

---

### Task 1: 更新公共定义 `include/common.h`

**Files:**
- Modify: `include/common.h`

- [ ] **Step 1: 新增 GameMode 枚举值**

在 `MODE_TIME_CHALLENGE` 后添加：

```c
typedef enum GameMode {
    MODE_SINGLE = 0,
    MODE_AI_BATTLE,
    MODE_TIME_CHALLENGE,
    MODE_LOCAL_MULTIPLAYER   // 新增
} GameMode;
```

- [ ] **Step 2: 新增 GameResult 枚举值**

在 `RESULT_PLAYER_DEAD` 后添加：

```c
typedef enum GameResult {
    RESULT_RUNNING = 0,
    RESULT_PLAYER_WIN,
    RESULT_AI_WIN,
    RESULT_DRAW,
    RESULT_TIME_UP,
    RESULT_PLAYER_DEAD,
    RESULT_P1_WIN,          // 新增：本地多人 P1 胜
    RESULT_P2_WIN           // 新增：本地多人 P2 胜
} GameResult;
```

- [ ] **Step 3: 新增 P2 索引常量**

在 `#define AI_INDEX 1` 后添加：

```c
#define PLAYER_INDEX 0
#define AI_INDEX 1
#define P2_INDEX 1   // 本地多人模式玩家二，复用 ai 槽位
```

---

### Task 2: 更新输入模块 `include/input.h` + `src/input.c`

**Files:**
- Modify: `include/input.h`
- Modify: `src/input.c`

- [ ] **Step 1: 声明 P2 方向读取函数**

在 `include/input.h` 的 `Input_readPlayerDirection` 声明后添加：

```c
Direction Input_readPlayer2Direction(void);
```

- [ ] **Step 2: 实现 P2 方向读取**

在 `src/input.c` 的 `Input_readPlayerDirection` 函数后添加：

```c
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
```

使用的是 `isKeyDown`（电平检测，长按持续生效），而非 `keyPressed`（边沿检测，只触发一次）。这样 P2 可以按住方向键连续移动。

---

### Task 3: 更新游戏逻辑 `src/game.c`

**Files:**
- Modify: `src/game.c`

这是改动最大的文件，分为多个步骤。

- [ ] **Step 1: 修改 `Game_applyModeDefaults` 增加多人模式分支**

在 `Game_applyModeDefaults` 函数（约 860 行）的 if-else 链中添加：

```c
} else if (mode == MODE_LOCAL_MULTIPLAYER) {
    config->moveIntervalMs = 130;
    config->timeLimitSeconds = TIME_LIMIT_SECONDS;  // 90 秒
    config->variant = VARIANT_DIVERSE;              // 强制多样
}
```

- [ ] **Step 2: 修改 `Game_init` 增加多人模式初始化**

在 `Game_init` 函数（约 899 行）中，将 `config->mode == MODE_AI_BATTLE` 的判断扩展为：

```c
if (config->mode == MODE_AI_BATTLE || config->mode == MODE_LOCAL_MULTIPLAYER) {
    Pos playerHead;
    Pos p2Head;

    playerHead.row = mapSize / 2;
    playerHead.col = mapSize / 4;
    p2Head.row = mapSize / 2;
    p2Head.col = mapSize - mapSize / 4 - 1;
    initSnake(&state->player, playerHead, DIR_RIGHT);
    initSnake(&state->ai, p2Head, DIR_LEFT);  // ai 槽位作为 P2
}
```

- [ ] **Step 3: 修改 `maintainItems` 支持多人模式使用战斗道具**

将 `maintainItems` 函数（约 304 行）的条件改为：

```c
static void maintainItems(GameState *state)
{
    if (state->config.mode == MODE_AI_BATTLE || state->config.mode == MODE_LOCAL_MULTIPLAYER) {
        maintainBattleItems(state);
    } else {
        maintainNormalItems(state);
    }
}
```

- [ ] **Step 4: 新增 `Game_prepareP2Start` 函数**

在 `Game_preparePlayerStart` 函数（约 923 行）之后添加。P2 的蛇也需要在游戏开始时根据方向键重新摆放身体：

```c
void Game_prepareP2Start(GameState *state, Direction dir)
{
    Snake *snake = &state->ai;
    Pos head;
    int i;

    if (dir == DIR_NONE || !snake->alive || snake->length <= 0) {
        return;
    }

    head = snake->body[0];
    snake->dir = dir;
    snake->nextDir = dir;
    for (i = 1; i < snake->length; i++) {
        Pos body = head;

        if (dir == DIR_UP) {
            body.row += i;
        } else if (dir == DIR_DOWN) {
            body.row -= i;
        } else if (dir == DIR_LEFT) {
            body.col += i;
        } else if (dir == DIR_RIGHT) {
            body.col -= i;
        }

        if (Game_isInside(state, body)) {
            state->cells[body.row][body.col] = CELL_EMPTY;
            snake->body[i] = body;
        }
    }
}
```

在 `include/game.h` 中添加声明：

```c
void Game_prepareP2Start(GameState *state, Direction dir);
```

- [ ] **Step 5: 新增 `stepLocalMultiplayer` 函数**

在 `stepBattle` 函数（约 805 行）之后添加。这个函数和 `stepBattle` 结构类似，但 P2 的移动方向来自输入而非 AI：

```c
static void stepLocalMultiplayer(GameState *state)
{
    StepPlan p1Plan;
    StepPlan p2Plan;
    bool p1Moves;
    bool p2Moves;

    p1Moves = snakeMovesThisBattleStep(&state->player);
    p2Moves = snakeMovesThisBattleStep(&state->ai);

    p1Plan = p1Moves ? buildStepPlan(state, &state->player) : buildStayPlan(&state->player);
    p2Plan = p2Moves ? buildStepPlan(state, &state->ai) : buildStayPlan(&state->ai);

    addBattleCollisions(state, &p1Plan, &p2Plan);

    applySnakeMove(state, &state->player, &p1Plan);
    applySnakeMove(state, &state->ai, &p2Plan);
    applyItemEffect(state, &state->player, &state->ai, &p1Plan);
    applyItemEffect(state, &state->ai, &state->player, &p2Plan);

    finishIfNeeded(state);
    if (state->result == RESULT_RUNNING) {
        maintainItems(state);
    }
}
```

- [ ] **Step 6: 修改 `Game_update` 增加多人模式分支**

在 `Game_update` 函数（约 981 行）中，时间挑战判定也需要覆盖多人模式：

```c
if (state->config.mode == MODE_TIME_CHALLENGE
    || state->config.mode == MODE_LOCAL_MULTIPLAYER) {
    state->elapsedMs += deltaMs;
    state->remainingSeconds = state->config.timeLimitSeconds - state->elapsedMs / 1000;
    if (state->remainingSeconds <= 0) {
        state->remainingSeconds = 0;
        /* 时间到：比较分数 */
        if (state->player.score > state->ai.score) {
            state->result = RESULT_P1_WIN;
        } else if (state->ai.score > state->player.score) {
            state->result = RESULT_P2_WIN;
        } else {
            state->result = RESULT_DRAW;
        }
        setStatus(state, "Time up");
        return;
    }
}
```

在步进调用处（约 998 行），扩展判断：

```c
if (state->config.mode == MODE_AI_BATTLE) {
    stepBattle(state);
} else if (state->config.mode == MODE_LOCAL_MULTIPLAYER) {
    stepLocalMultiplayer(state);
} else {
    stepSingle(state);
}
```

- [ ] **Step 7: 修改 `finishIfNeeded` 支持多人模式胜负判定**

在 `finishIfNeeded` 函数（约 682 行）中，将 `mode == MODE_AI_BATTLE` 扩展为：

```c
if (state->config.mode == MODE_AI_BATTLE || state->config.mode == MODE_LOCAL_MULTIPLAYER) {
    if (!state->player.alive && !state->ai.alive) {
        state->result = RESULT_DRAW;
        setStatus(state, "Draw");
        Game_pushSoundEvent(state, SOUND_COLLISION);
    } else if (!state->player.alive) {
        state->result = (state->config.mode == MODE_LOCAL_MULTIPLAYER)
            ? RESULT_P2_WIN : RESULT_AI_WIN;
        setStatus(state,
            state->config.mode == MODE_LOCAL_MULTIPLAYER ? "P2 wins" : "AI wins");
        Game_pushSoundEvent(state, SOUND_COLLISION);
    } else if (!state->ai.alive) {
        state->result = (state->config.mode == MODE_LOCAL_MULTIPLAYER)
            ? RESULT_P1_WIN : RESULT_PLAYER_WIN;
        setStatus(state,
            state->config.mode == MODE_LOCAL_MULTIPLAYER ? "P1 wins" : "Player wins");
        Game_pushSoundEvent(state, SOUND_COLLISION);
    }
}
```

- [ ] **Step 8: 新增 `Game_setP2Direction` 函数**

在 `Game_setPlayerDirection` 函数（约 956 行）之后添加。P2 的方向设置函数，操作 `ai` 槽位蛇：

```c
void Game_setP2Direction(GameState *state, Direction dir)
{
    if (dir == DIR_NONE || state->result != RESULT_RUNNING) {
        return;
    }
    setSnakeDirectionWithSlow(&state->ai, dir);
}
```

在 `include/game.h` 中添加声明：

```c
void Game_setP2Direction(GameState *state, Direction dir);
```

- [ ] **Step 9: 修改 `fireArrow` 允许多人模式射箭**

`fireArrow` 函数（约 1049 行）第一行把硬编码的 `MODE_AI_BATTLE` 检查打开：

```c
if ((state->config.mode != MODE_AI_BATTLE
     && state->config.mode != MODE_LOCAL_MULTIPLAYER)
    || shooter->bowArrows <= 0 || !shooter->alive) {
    return false;
}
```

- [ ] **Step 10: 新增 `Game_player2FireArrow` 函数**

在 `Game_playerFireArrow` 之后（约 1101 行）添加：

```c
bool Game_player2FireArrow(GameState *state)
{
    return fireArrow(state, P2_INDEX);
}
```

- [ ] **Step 11: 在 `game.h` 中声明 `Game_player2FireArrow`**

在 `include/game.h` 的 `Game_playerFireArrow` 声明后添加：

```c
bool Game_player2FireArrow(GameState *state);
```

- [ ] **Step 12: 修改 `effectiveMoveInterval` 适配多人模式**

`effectiveMoveInterval` 函数（约 352 行）中 `mode != MODE_AI_BATTLE` 的判断需要改为同时排除多人模式（多人模式的速度/减速效果也按 per-snake 处理）：

```c
if (state->config.mode != MODE_AI_BATTLE
    && state->config.mode != MODE_LOCAL_MULTIPLAYER) {
    if (state->player.speedMs > 0) {
        interval = interval * 70 / 100;
    }
    if (state->player.slowMs > 0) {
        interval = interval * 130 / 100;
    }
}
```

并且 speedLevel 部分在多人模式下应强制不生效。在 speedLevel 计算处：

```c
if (state->config.mode != MODE_LOCAL_MULTIPLAYER) {
    if (speedLevel > 0) {
        interval = interval * (100 - speedLevel * 18) / 100;
    } else if (speedLevel < 0) {
        interval = interval * (100 + (-speedLevel) * 22) / 100;
    }
}
```

- [ ] **Step 13: 修改 `Game_adjustSpeed` 在多人模式禁止调速**

```c
void Game_adjustSpeed(GameState *state, int delta)
{
    if (state->config.mode == MODE_LOCAL_MULTIPLAYER) {
        return;  // 多人模式不支持调速
    }
    state->speedLevel += delta;
    // ... 原有逻辑
}
```

- [ ] **Step 14: 修改 `scoreForItem` 多人模式不使用 speedLevel 加成**

在 `scoreForItem` 函数（约 607 行）中：

```c
static int scoreForItem(const GameState *state, CellType item)
{
    int baseScore = Game_itemScore(item);
    int multiplier;
    int score;

    if (!Game_isFood(item)) {
        return baseScore;
    }

    if (state->config.mode == MODE_LOCAL_MULTIPLAYER) {
        multiplier = 100;  // 多人模式固定倍率
    } else {
        multiplier = 100 + state->speedLevel * 25;
        if (multiplier < 40) {
            multiplier = 40;
        }
    }

    score = baseScore * multiplier / 100;
    return score < 1 ? 1 : score;
}
```

---

### Task 4: 更新 UI 流程 `src/ui.c`

**Files:**
- Modify: `src/ui.c`

- [ ] **Step 1: 欢迎菜单增加第 7 项**

在 `Ui_runWelcome` 函数（约 21 行）中，`wrapIndex` 的菜单项数量从 6 改为 7，并调整枚举对应关系。注意 `MenuAction` 枚举也需要同步更新。

先改 `include/ui.h` 的 `MenuAction` 枚举：

```c
typedef enum MenuAction {
    MENU_SINGLE = 0,
    MENU_AI_BATTLE,
    MENU_TIME_CHALLENGE,
    MENU_MULTIPLAYER,     // 新增
    MENU_CHANGE_SKIN,
    MENU_SETTINGS,
    MENU_EXIT
} MenuAction;
```

`Ui_runWelcome` 中 `wrapIndex` 的计数改为 7。

- [ ] **Step 2: 修改 `runOneRound` 处理多人模式**

在 `runOneRound` 函数（约 211 行）中，核心改动如下：

```c
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

        /* P1 方向：WASD */
        dir = Input_readPlayerDirection();

        /* P2 方向：方向键（仅多人模式） */
        if (isMulti) {
            dir2 = Input_readPlayer2Direction();
        } else {
            dir2 = DIR_NONE;
        }

        if (waitingForStart) {
            if (isMulti) {
                /* 多人模式：双方都按下方向键后开始 */
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
        /* 多人模式禁用调速 */
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
```

注意：`Game_setP2Direction` 在 Task 3 Step 8 中已实现。

- [ ] **Step 3: 更新 `MenuInput` 添加 P2 射箭字段**

在 `include/input.h` 的 `MenuInput` 结构体末尾添加：

```c
bool p2Fire;
```

在 `src/input.c` 的 `Input_readMenu` 中添加 P2 射箭键检测（用 `/` 键，即 `VK_OEM_2`）：

```c
out->p2Fire = keyPressed(input, VK_OEM_2);  // '/' 键
```

---

### Task 5: 更新渲染层 `src/render_easyx.c`

**Files:**
- Modify: `src/render_easyx.c`

- [ ] **Step 1: 修改 `Render_drawWelcome` 菜单项为 7 个**

将 `OPTIONS` 数组（约 732 行）扩展：

```c
static const TCHAR *OPTIONS[] = {
    _T("单人模式"),
    _T("AI 对战模式"),
    _T("限时挑战模式"),
    _T("本地多人模式"),    // 新增
    _T("更换时装"),
    _T("设置"),
    _T("退出游戏")
};
```

循环中的 `6` 改为 `7`。

- [ ] **Step 2: 修改 `Render_drawGame` 侧栏显示双人信息**

多人模式下侧栏需要显示 P1 和 P2 各自的信息。在侧栏渲染部分（约 852 行后），根据模式分支显示不同内容。

多人模式侧栏内容（替换现有的玩家得分区）：

```c
if (state->config.mode == MODE_LOCAL_MULTIPLAYER) {
    /* P1 得分区 */
    drawTextAt(x + 20, BOARD_TOP + 168, _T("玩家一 (WASD)"), 20, RGB(247, 202, 99));
    _stprintf_s(buffer, 128, _T("得分：%d"), state->player.score);
    drawTextAt(x + 20, BOARD_TOP + 198, buffer, 28, RGB(247, 202, 99));
    _stprintf_s(buffer, 128, _T("长度：%d"), state->player.length);
    drawTextAt(x + 20, BOARD_TOP + 238, buffer, 18, RGB(225, 235, 224));
    _stprintf_s(buffer, 128, _T("弓箭：%d  护盾：%d"),
        state->player.bowArrows, state->player.shieldCharges);
    drawTextAt(x + 20, BOARD_TOP + 264, buffer, 18, RGB(204, 214, 222));

    /* 分隔线 */
    setlinecolor(RGB(64, 76, 82));
    line(x + 20, BOARD_TOP + 300, x + SIDE_PANEL_WIDTH - 44, BOARD_TOP + 300);

    /* P2 得分区 */
    drawTextAt(x + 20, BOARD_TOP + 316, _T("玩家二 (方向键)"), 20, RGB(86, 206, 144));
    _stprintf_s(buffer, 128, _T("得分：%d"), state->ai.score);
    drawTextAt(x + 20, BOARD_TOP + 346, buffer, 28, RGB(86, 206, 144));
    _stprintf_s(buffer, 128, _T("长度：%d"), state->ai.length);
    drawTextAt(x + 20, BOARD_TOP + 386, buffer, 18, RGB(225, 235, 224));
    _stprintf_s(buffer, 128, _T("弓箭：%d  护盾：%d"),
        state->ai.bowArrows, state->ai.shieldCharges);
    drawTextAt(x + 20, BOARD_TOP + 412, buffer, 18, RGB(204, 214, 222));

    /* 剩余时间 */
    _stprintf_s(buffer, 128, _T("剩余时间：%d s"), state->remainingSeconds);
    drawTextAt(x + 20, BOARD_TOP + 460, buffer, 22, RGB(103, 196, 224));
}
```

多人模式下不显示：速度档、皮肤名、E 射箭 1 加速 2 减速 提示。

底部提示文字改为：

```c
if (state->config.mode == MODE_LOCAL_MULTIPLAYER) {
    drawTextAt(x + 20, BOARD_TOP + boardSize - 36,
        _T("P1: E射箭  P2: /射箭"), 16, RGB(160, 197, 181));
}
```

- [ ] **Step 3: 修改 `Render_drawGameOver` 支持 P1/P2 胜负**

在 `Render_drawGameOver` 函数（约 903 行）中增加多人模式的标题判定：

```c
if (state->result == RESULT_P1_WIN) {
    title = _T("玩家一 胜利");
} else if (state->result == RESULT_P2_WIN) {
    title = _T("玩家二 胜利");
} else if (state->result == RESULT_PLAYER_WIN) {
    title = _T("玩家胜利");
} else if (state->result == RESULT_AI_WIN) {
    title = _T("AI 胜利");
} else if (state->result == RESULT_DRAW) {
    title = _T("平局");
} else if (state->result == RESULT_TIME_UP) {
    title = _T("时间到");
} else {
    title = _T("游戏结束");
}
```

分数显示改为：

```c
_stprintf_s(score, 128, _T("玩家一：%d"), state->player.score);
drawCenteredText(0, 215, gWindowWidth, 260, score, 28, RGB(247, 202, 99));
if (state->config.mode == MODE_AI_BATTLE || state->config.mode == MODE_LOCAL_MULTIPLAYER) {
    const TCHAR *p2Label = (state->config.mode == MODE_LOCAL_MULTIPLAYER)
        ? _T("玩家二") : _T("AI");
    _stprintf_s(score, 128, _T("%s：%d"), p2Label, state->ai.score);
    drawCenteredText(0, 260, gWindowWidth, 305, score, 24,
        state->config.mode == MODE_LOCAL_MULTIPLAYER ? RGB(86, 206, 144) : RGB(242, 122, 119));
}
```

- [ ] **Step 4: 修改等待开始提示文字**

在 `Render_drawGame` 的等待提示处（约 889 行），多人模式显示不同提示：

```c
const TCHAR *text;
if (waitingForStart) {
    text = (state->config.mode == MODE_LOCAL_MULTIPLAYER)
        ? _T("P1: W/A/S/D  P2: 方向键  开始")
        : _T("按 W/A/S/D 开始");
} else {
    text = _T("暂停");
}
```

---

### Task 6: 更新菜单入口 `src/main.c`

**Files:**
- Modify: `src/main.c`

- [ ] **Step 1: 添加多人模式菜单处理分支**

在 `main` 函数（约 67 行）的 if-else 链中，在 `MENU_TIME_CHALLENGE` 分支之后添加：

```c
} else if (action == MENU_MULTIPLAYER) {
    Game_applyModeDefaults(&config, MODE_LOCAL_MULTIPLAYER);
    config.variant = VARIANT_DIVERSE;  // 强制多样
    /* 跳过变体选择和难度选择，直接开始 */
    Render_loadSkin(&render, skinId);
    Game_init(state, &config);
    Ui_runGame(&input, &render, state);
}
```

注意：多人模式跳过 `Ui_chooseVariant` 和 `Ui_chooseDifficulty`，因为始终使用多样模式且无 AI。

---

### Task 7: 更新通用字符串 `src/common.c`

**Files:**
- Modify: `src/common.c`

- [ ] **Step 1: 更新 `Common_modeName`**

在 switch 中添加：

```c
case MODE_LOCAL_MULTIPLAYER:
    return "Local Multiplayer";
```

- [ ] **Step 2: 更新 `Common_resultName`**

在 switch 中添加：

```c
case RESULT_P1_WIN:
    return "P1 Wins";
case RESULT_P2_WIN:
    return "P2 Wins";
```

---

### Task 8: 编译与冒烟测试

- [ ] **Step 1: 编译**

```powershell
& 'E:\VS\MSBuild\Current\Bin\MSBuild.exe' BUPT_Eating_Snake_Graphics.vcxproj /p:Configuration=Debug /p:Platform=x64 /m
```

预期：0 错误，0 警告。

- [ ] **Step 2: 启动冒烟测试**

运行生成的 exe，确认：
- 欢迎菜单显示 7 个选项，第 4 项为「本地多人模式」
- 选择后直接进入游戏（不经过变体和难度选择）
- 两蛇出现在地图左右两侧
- P1 按 W/A/S/D 移动
- P2 按方向键移动
- 侧栏显示两个玩家的分数、长度、弓箭、护盾
- 按 P 暂停，Esc 退出
- 程序运行 10 秒不闪退，无 `snake_crash.log`

- [ ] **Step 3: 功能测试清单**

1. **双人移动测试**：P1 WASD、P2 方向键各自独立控制方向
2. **碰撞死亡测试**：撞墙/撞对方/撞自己 → 立即判负
3. **时间到判负测试**：等待 90 秒 → 比较分数
4. **弓箭测试**：P1 吃弓箭按 E 射 → P2 受击（无护盾时）
5. **弓箭测试**：P2 吃弓箭按 / 射 → P1 受击
6. **护盾测试**：吃护盾后挡一次弓箭/尖刺
7. **尖刺陷阱测试**：无护盾踩尖刺 → 死亡
8. **减速时钟测试**：吃时钟后对方移动变慢
9. **分辨率适配**：切换 1280x720 / 1920x1080 / 2560x1440，侧栏和棋盘不变形
10. **全屏适配**：全屏模式下双人游戏正常
11. **地图尺寸适配**：20x20 / 50x50 / 100x100 均正常运行
12. **反方向输入**：按反方向不死亡
13. **重新开始**：游戏结束后重新开始正常
14. **返回菜单**：游戏结束后返回菜单，再进单人/AI 模式正常

---

### Task 9: 回归测试

- [ ] **Step 1: 单人模式回归**

- 进入单人模式 → 常规/多样均正常
- 速度调整 1/2 正常
- 游戏结束判定正常

- [ ] **Step 2: AI 对战模式回归**

- 进入 AI 对战 → 选择难度 → 低/中/高 AI 正常
- AI 射箭正常
- AI 道具拾取正常
- 游戏结束判定正常

- [ ] **Step 3: 限时挑战模式回归**

- 进入限时挑战 → 计时正常 → 时间到正常结束
- 道具正常生成
- 分数正常

- [ ] **Step 4: 设置界面回归**

- 设置各项选项正常
- 时装切换正常
- 全屏切换正常
- 分辨率切换正常

- [ ] **Step 5: 编译**

```powershell
& 'E:\VS\MSBuild\Current\Bin\MSBuild.exe' BUPT_Eating_Snake_Graphics.vcxproj /p:Configuration=Debug /p:Platform=x64 /m
```

预期：0 错误，0 警告。
