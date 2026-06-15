# UI 美化实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将游戏 UI 从旧版深色风格升级为清爽现代暗色风格，加入完整粒子系统和过渡动画。

**Architecture:** 主力修改 `render_easyx.c`（配色定义+组件重绘+粒子系统+动画），辅助修改 `common.h`（粒子结构体、颜色宏）、`game.c`（粒子触发钩子、性能监控）。分 4 个 Pass 逐步推进，每 Pass 独立可编译。

**Tech Stack:** C + EasyX + Win32 GDI

---

## Task 1: 配色系统 + 颜色宏定义

**Files:**
- Modify: `include/common.h`

- [ ] **Step 1: 新增颜色宏**

在 `#define SIDE_PANEL_WIDTH 280` 后添加：

```c
/* UI 配色系统 — 清爽现代暗色 */
#define COLOR_BG         RGB(26, 31, 43)
#define COLOR_CARD       RGB(38, 45, 58)
#define COLOR_CARD_HOVER RGB(48, 56, 70)
#define COLOR_BOARD      RGB(42, 48, 64)
#define COLOR_WALL_COLOR RGB(58, 64, 80)
#define COLOR_ACCENT     RGB(88, 166, 255)
#define COLOR_POSITIVE   RGB(124, 255, 107)
#define COLOR_DANGER     RGB(255, 64, 64)
#define COLOR_WARNING     RGB(255, 200, 55)
#define COLOR_TEXT       RGB(232, 236, 240)
#define COLOR_TEXT_DIM   RGB(106, 116, 132)
#define COLOR_SCORE      RGB(255, 202, 99)
#define COLOR_BORDER     RGB(64, 76, 90)
#define COLOR_PANEL      RGB(38, 45, 58)
#define COLOR_GRID       RGB(50, 58, 72)
```

- [ ] **Step 2: 新增粒子结构体**

在 `RandomEventState` 之后添加：

```c
#define MAX_PARTICLES 200

typedef struct Particle {
    bool active;
    float x, y;
    float vx, vy;
    int lifeMs;
    COLORREF color;
    int radius;
} Particle;
```

- [ ] **Step 3: 在 `GameState` 中添加粒子数组**

在 `RandomEventState event;` 之后添加：

```c
Particle particles[MAX_PARTICLES];
```

---

## Task 2: Pass 1 — 静态美化（配色/字体/圆角/卡片）

**Files:**
- Modify: `src/render_easyx.c`

> 将所有硬编码颜色替换为 `COLOR_*` 宏，修改组件绘制代码。
> 改动量大但属机械替换，此 Task 覆盖全部静态美化。

- [ ] **Step 1: 全局替换底色**

搜索所有 `RGB(13, 19, 24)` → `COLOR_BG`
搜索所有 `RGB(24, 34, 42)` → `COLOR_CARD`
搜索所有 `RGB(28, 39, 48)` → `COLOR_CARD`
搜索所有 `RGB(16, 22, 27)` → `COLOR_BG`

- [ ] **Step 2: 修改菜单按钮绘制 `drawMenuButton`**

当前按钮：圆角矩形，选中态金色边框 + 高亮底色。
改为：
- 普通态：`COLOR_CARD` 底色，`COLOR_TEXT` 文字 18 号
- 选中态：`COLOR_CARD_HOVER` 底色 + `COLOR_ACCENT` 左边框 3px + 文字变 `COLOR_ACCENT`
- 宽度改为 `gWindowWidth / 2 + 60` 居中，高度 52px

```c
static void drawMenuButton(int index, int selected, const TCHAR *label)
{
    int btnWidth = gWindowWidth / 2 + 60;
    int btnHeight = 52;
    int left = (gWindowWidth - btnWidth) / 2;
    int top = 220 + index * (btnHeight + 10);
    bool isSel = (index == selected);

    setfillcolor(isSel ? COLOR_CARD_HOVER : COLOR_CARD);
    solidroundrect(left, top, left + btnWidth, top + btnHeight, 10, 10);

    if (isSel) {
        setfillcolor(COLOR_ACCENT);
        solidrectangle(left, top, left + 3, top + btnHeight);
    }

    setTextColor(isSel ? COLOR_ACCENT : COLOR_TEXT, 18);
    drawTextAt(left + 22, top + 15, label, 18,
        isSel ? COLOR_ACCENT : COLOR_TEXT);
}
```

- [ ] **Step 3: 修改信息面板背景**

`Render_drawGame` 中侧栏背景：
- 底色 `COLOR_PANEL`
- 右侧面板从 `render->windowWidth - 24` 改为到窗口边缘，圆角左边 10px

- [ ] **Step 4: 修改棋盘颜色**

- `drawBoardBackground`: 地面 `COLOR_BOARD`
- `gridColor`: `COLOR_GRID`
- 墙壁颜色改为 `COLOR_WALL_COLOR`

- [ ] **Step 5: 修改游戏结束画面**

- 遮罩改为 `COLOR_BG` + 70% 透明度（用纯色近似）
- 中间卡片：圆角 14px `COLOR_CARD`
- 按钮样式与菜单一致

- [ ] **Step 6: 修改设置界面**

设置行 `drawSettingsRow`: 底色 `COLOR_CARD`，选中态 `COLOR_ACCENT` 左边框

- [ ] **Step 7: 编译验证**

```powershell
& 'E:\VS\MSBuild\Current\Bin\MSBuild.exe' BUPT_Eating_Snake_Graphics.vcxproj /p:Configuration=Debug /p:Platform=x64 /m
```

---

## Task 3: Pass 2 — 动画与过渡

**Files:**
- Modify: `src/render_easyx.c`
- Modify: `src/ui.c`

- [ ] **Step 1: 分数弹跳动效**

在 `Render_drawGame` 中，检测分数变化时触发动效：
- 添加静态变量 `lastDisplayedScore`，当 `state->player.score != lastDisplayedScore` 时
- 得分数字用增大字号 (28→36) 绘制，持续 300ms 后恢复

```c
static int scoreAnimTimer = 0;
static int scoreAnimValue = 0;

// 在绘制分数处：
if (state->player.score != scoreAnimValue) {
    scoreAnimValue = state->player.score;
    scoreAnimTimer = 300;
}
int scoreSize = (scoreAnimTimer > 0) ? 34 : 28;
COLORREF scoreColor = (scoreAnimTimer > 200) ? RGB(255,255,255) : COLOR_SCORE;
// 使用 scoreSize 和 scoreColor 绘制分数
```

- [ ] **Step 2: 事件预警脉冲**

预警文字 "!!! 即将轰炸 !!!" 使用交替字号：
- Timer 变量 `warnPulsePhase`，每 500ms 切换
- 字号在 26 和 30 之间交替

- [ ] **Step 3: 暂停覆盖淡入**

暂停时先画半透明遮罩再写文字：
```c
setfillcolor(COLOR_BG);
// EasyX 无原生透明度，用纯色暗底近似
solidrectangle(left, top, right, bottom);
setfillcolor(COLOR_CARD);
solidrectangle(left+4, top+4, right-4, bottom-4); // 内层卡片
drawCenteredText(...);
```

- [ ] **Step 4: 蛇死亡闪烁**

在 `Ui_runGame` 的 `runOneRound` 中，游戏结束后：
- 调用新函数 `Render_flashDeath(render, state)` 绘制 3 帧红色闪烁
- 每帧：正常渲染 + 全屏半透明红色覆盖 + Sleep(200)

- [ ] **Step 5: 编译验证**

---

## Task 4: Pass 3 — 粒子系统

**Files:**
- Modify: `src/render_easyx.c`（粒子核心）
- Modify: `src/game.c`（触发钩子）

- [ ] **Step 1: 粒子初始化和更新函数**

在 `render_easyx.c` 中添加：

```c
static Particle gParticles[MAX_PARTICLES];

static void particlesInit(void)
{
    memset(gParticles, 0, sizeof(gParticles));
}

static void particlesUpdate(int deltaMs)
{
    int i;
    for (i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &gParticles[i];
        if (!p->active) continue;
        float dt = deltaMs / 1000.0f;
        p->x += p->vx * dt;
        p->y += p->vy * dt;
        p->lifeMs -= deltaMs;
        if (p->lifeMs <= 0) { p->active = false; continue; }
        if (p->lifeMs < 500) p->radius = (p->radius * p->lifeMs) / 500;
        if (p->radius < 1) p->radius = 1;
    }
}

static void particlesDraw(void)
{
    int i;
    for (i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &gParticles[i];
        if (!p->active) continue;
        setfillcolor(p->color);
        solidcircle((int)p->x, (int)p->y, p->radius);
    }
}

static void spawnParticles(float cx, float cy, int count, COLORREF color, int lifeMs)
{
    int i;
    for (i = 0; i < MAX_PARTICLES && count > 0; i++) {
        if (!gParticles[i].active) {
            gParticles[i].active = true;
            gParticles[i].x = cx;
            gParticles[i].y = cy;
            gParticles[i].vx = (float)(rand() % 200 - 100);
            gParticles[i].vy = (float)(rand() % 200 - 100);
            gParticles[i].lifeMs = lifeMs + rand() % 300;
            gParticles[i].color = color;
            gParticles[i].radius = 2 + rand() % 4;
            count--;
        }
    }
}
```

在 `Render_init` 中调用 `particlesInit()`。

- [ ] **Step 2: 渲染循环中集成粒子**

在 `Render_drawGame` 的 `FlushBatchDraw()` 之前添加：
```c
particlesUpdate(deltaMs);  // 需要传入 deltaMs，改为在游戏循环中调用
particlesDraw();
```

更好的方式：在 `runOneRound` 的渲染调用前，先更新粒子。

- [ ] **Step 3: 在 game.c 中添加粒子触发钩子**

新增公开函数，在 `game.h` 声明：
```c
void Game_triggerEatParticles(GameState *state, Pos cell, bool bonus);
void Game_triggerDeathParticles(GameState *state, const Snake *snake);
void Game_triggerBombParticles(GameState *state);
```

在 `applyItemEffect` 中调用 `Game_triggerEatParticles`。
在 `applySnakeMove` / `finishIfNeeded` 死亡时调用 `Game_triggerDeathParticles`。

- [ ] **Step 4: 菜单背景飘浮粒子**

在 `Render_drawWelcome` 等菜单渲染函数中添加：
```c
// 每帧生成 1-2 个飘浮粒子
static int menuParticleTimer = 0;
menuParticleTimer += deltaMs;  // 需要从 game loop 传入
if (menuParticleTimer > 200) {
    menuParticleTimer = 0;
    float x = (float)(rand() % gWindowWidth);
    float y = (float)(gWindowHeight + 10);
    // 生成飘浮粒子：vy 负值向上，vx 小范围左右
}
```

- [ ] **Step 5: 编译验证**

---

## Task 5: Pass 4 — 事件 UI 强化 + 性能降级

**Files:**
- Modify: `src/render_easyx.c`
- Modify: `src/game.c`

- [ ] **Step 1: 轰炸区边框**

在轰炸区地面染色后，额外绘制边框：
```c
setlinecolor(state->event.bombActive ? COLOR_DANGER : COLOR_WARNING);
setlinestyle(PS_SOLID, 2);
// 对每个 zone 绘制 rectangle 边框
```

- [ ] **Step 2: 侧栏事件进度条**

在事件信息下绘制进度条：
```c
if (state->event.activeEvent != EVENT_NONE) {
    int barWidth = SIDE_PANEL_WIDTH - 44;
    int filled = barWidth * (12000 - state->event.eventTimerMs) / 12000;
    if (filled < 0) filled = 0; if (filled > barWidth) filled = barWidth;
    COLORREF barColor = (state->event.eventTimerMs < 3000) ? COLOR_DANGER : COLOR_ACCENT;
    setfillcolor(COLOR_BG);
    solidrectangle(x + 20, barY, x + 20 + barWidth, barY + 6);
    setfillcolor(barColor);
    solidrectangle(x + 20, barY, x + 20 + filled, barY + 6);
}
```

- [ ] **Step 3: 箭矢飞行拖尾**

在 `drawArrow` 中，在箭头身后绘制 2 个渐淡圆点：
```c
// 箭头尾部位置 tailX, tailY 后面再画 2 个点
setfillcolor(RGB(255, 200, 100));
solidcircle(tailX, tailY, cellSize / 8);
setfillcolor(RGB(255, 150, 60));
// 第二个点更远更淡
```

- [ ] **Step 4: 性能监控与自动降级**

在 `game.c` 中添加：
```c
#define PERF_CHECK_INTERVAL_MS 10000
static int perfFrameTimeAccum = 0;
static int perfFrameCount = 0;
static bool gUseParticles = true;
static bool gUseTrails = true;

// 在 Game_update 中：
perfFrameTimeAccum += deltaMs;
perfFrameCount++;
if (perfFrameTimeAccum >= PERF_CHECK_INTERVAL_MS) {
    int avgMs = perfFrameTimeAccum / perfFrameCount;
    if (avgMs > 40) {
        gUseParticles = false;
        gUseTrails = false;
    }
    perfFrameTimeAccum = 0;
    perfFrameCount = 0;
}
```

- [ ] **Step 5: 编译 + 全模式冒烟测试**

---

## Verification

1. 编译 0/0
2. 进入菜单确认圆角卡片按钮 + 蓝色选中态
3. 进入游戏确认侧栏新样式 + 棋盘新配色
4. 吃食物确认分数弹跳动画
5. 等待随机事件确认轰炸区边框 + 进度条
6. 死亡确认闪烁效果 + 粒子
7. 100×100 地图确认性能降级触发
