#include <stdio.h>
#include <string.h>
#include <io.h>
#include <windows.h>

#include "game.h"
#include "render.h"

typedef struct SkinInfo {
    const TCHAR *folder;
    const TCHAR *name;
} SkinInfo;

typedef struct ResolutionInfo {
    DisplayResolution id;
    int windowWidth;
    int windowHeight;
    int boardPixelSize;
    const TCHAR *name;
} ResolutionInfo;

static const SkinInfo SKINS[] = {
    { _T("default"), _T("像素森林") },
    { _T("neon"), _T("霓虹夜行") },
    { _T("ice"), _T("冰原挑战") }
};

static const ResolutionInfo RESOLUTIONS[] = {
    { RESOLUTION_HD, 1280, 720, 640, _T("1280 x 720") },
    { RESOLUTION_HD_PLUS, 1600, 900, 800, _T("1600 x 900") },
    { RESOLUTION_FHD, 1920, 1080, 960, _T("1920 x 1080") },
    { RESOLUTION_QHD, 2560, 1440, 1280, _T("2560 x 1440") }
};

static const COLORREF FALLBACK_COLORS[TEX_COUNT] = {
    RGB(61, 114, 74),
    RGB(84, 84, 86),
    RGB(91, 68, 52),
    RGB(222, 71, 63),
    RGB(246, 184, 70),
    RGB(64, 183, 216),
    RGB(151, 116, 215),
    RGB(39, 41, 48),
    RGB(86, 206, 144),
    RGB(206, 154, 78),
    RGB(86, 206, 144),
    RGB(36, 38, 44),
    RGB(121, 120, 220),
    RGB(56, 210, 92),
    RGB(46, 154, 79),
    RGB(238, 86, 86),
    RGB(176, 50, 65)
};

static const TCHAR *TEXTURE_NAMES[TEX_COUNT] = {
    _T("ground.bmp"),
    _T("wall.bmp"),
    _T("obstacle.bmp"),
    _T("food.bmp"),
    _T("food_bonus.bmp"),
    _T("food_speed.bmp"),
    _T("food_slow.bmp"),
    _T("trap.bmp"),
    _T("shield.bmp"),
    _T("bow.bmp"),
    _T("battle_shield.bmp"),
    _T("spike.bmp"),
    _T("clock.bmp"),
    _T("player_head.bmp"),
    _T("player_body.bmp"),
    _T("ai_head.bmp"),
    _T("ai_body.bmp")
};

static int gWindowWidth = 1440;
static int gWindowHeight = 1000;

static int minInt(int a, int b)
{
    return a < b ? a : b;
}

static int maxInt(int a, int b)
{
    return a > b ? a : b;
}

static int resolutionCount(void)
{
    return (int)(sizeof(RESOLUTIONS) / sizeof(RESOLUTIONS[0]));
}

static const ResolutionInfo *resolutionInfo(DisplayResolution resolution)
{
    int i;

    for (i = 0; i < resolutionCount(); i++) {
        if (RESOLUTIONS[i].id == resolution) {
            return &RESOLUTIONS[i];
        }
    }

    return &RESOLUTIONS[1];
}

static void getWorkAreaSize(int *width, int *height)
{
    RECT workArea;

    if (SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0)) {
        *width = workArea.right - workArea.left;
        *height = workArea.bottom - workArea.top;
        return;
    }

    *width = GetSystemMetrics(SM_CXSCREEN);
    *height = GetSystemMetrics(SM_CYSCREEN);
}

static void getWindowedClientLimit(int *width, int *height)
{
    int workWidth;
    int workHeight;
    int frameWidth;
    int frameHeight;

    getWorkAreaSize(&workWidth, &workHeight);
    frameWidth = GetSystemMetrics(SM_CXSIZEFRAME) * 2
        + GetSystemMetrics(SM_CXPADDEDBORDER) * 2;
    frameHeight = GetSystemMetrics(SM_CYSIZEFRAME) * 2
        + GetSystemMetrics(SM_CXPADDEDBORDER) * 2
        + GetSystemMetrics(SM_CYCAPTION);

    *width = maxInt(640, workWidth - frameWidth - 24);
    *height = maxInt(360, workHeight - frameHeight - 24);
}

static void fitTo16By9(int requestedWidth, int requestedHeight,
    int limitWidth, int limitHeight, int *width, int *height)
{
    int fittedWidth = requestedWidth;
    int fittedHeight = requestedHeight;

    if (fittedWidth > limitWidth) {
        fittedWidth = limitWidth;
        fittedHeight = fittedWidth * 9 / 16;
    }
    if (fittedHeight > limitHeight) {
        fittedHeight = limitHeight;
        fittedWidth = fittedHeight * 16 / 9;
    }

    fittedWidth = minInt(fittedWidth, limitWidth);
    fittedHeight = minInt(fittedHeight, limitHeight);
    if (fittedWidth < 640 || fittedHeight < 360) {
        fittedWidth = minInt(limitWidth, 640);
        fittedHeight = minInt(limitHeight, 360);
    }

    *width = fittedWidth;
    *height = fittedHeight;
}

static void resolveDisplaySize(const ResolutionInfo *info, bool fullscreen,
    int *width, int *height)
{
    if (fullscreen) {
        *width = GetSystemMetrics(SM_CXSCREEN);
        *height = GetSystemMetrics(SM_CYSCREEN);
        return;
    }

    {
        int limitWidth;
        int limitHeight;

        getWindowedClientLimit(&limitWidth, &limitHeight);
        fitTo16By9(info->windowWidth, info->windowHeight,
            limitWidth, limitHeight, width, height);
    }
}

static int boardPixelSizeForWindow(int windowWidth, int windowHeight)
{
    int maxBoardWidth = windowWidth - BOARD_LEFT - SIDE_PANEL_WIDTH - 72;
    int maxBoardHeight = windowHeight - BOARD_TOP - 24;
    int boardSize = minInt(maxBoardWidth, maxBoardHeight);

    if (boardSize < 320) {
        boardSize = minInt(maxInt(240, windowWidth - BOARD_LEFT * 2),
            maxInt(240, windowHeight - BOARD_TOP - 24));
    }

    return boardSize;
}

static void applyNativeWindowMode(bool fullscreen, int clientWidth, int clientHeight)
{
    HWND hwnd = GetHWnd();
    LONG_PTR style;
    LONG_PTR exStyle;

    if (hwnd == NULL) {
        return;
    }

    if (fullscreen) {
        style = WS_POPUP | WS_VISIBLE;
        exStyle = WS_EX_APPWINDOW;
        SetWindowLongPtr(hwnd, GWL_STYLE, style);
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
        SetWindowPos(hwnd, HWND_TOP, 0, 0, clientWidth, clientHeight,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    } else {
        RECT workArea;
        RECT windowRect = { 0, 0, clientWidth, clientHeight };
        int x = CW_USEDEFAULT;
        int y = CW_USEDEFAULT;
        int windowWidth;
        int windowHeight;

        style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        exStyle = WS_EX_APPWINDOW;
        AdjustWindowRectEx(&windowRect, (DWORD)style, FALSE, (DWORD)exStyle);
        windowWidth = windowRect.right - windowRect.left;
        windowHeight = windowRect.bottom - windowRect.top;
        if (SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0)) {
            x = workArea.left + maxInt(0,
                (workArea.right - workArea.left - windowWidth) / 2);
            y = workArea.top + maxInt(0,
                (workArea.bottom - workArea.top - windowHeight) / 2);
        }

        SetWindowLongPtr(hwnd, GWL_STYLE, style);
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
        SetWindowPos(hwnd, HWND_NOTOPMOST, x, y, windowWidth, windowHeight,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }
}

static int cellSizeForMap(const RenderContext *render, int mapSize)
{
    int cellSize;
    int minimumCellSize = 1;

    mapSize = Game_validMapSize(mapSize);
    cellSize = render->boardPixelSize / mapSize;
    if (mapSize >= 50) {
        minimumCellSize = 18;
    }
    if (cellSize < minimumCellSize) {
        cellSize = minimumCellSize;
    }
    return cellSize < 1 ? 1 : cellSize;
}

static int visibleCellsForMap(const RenderContext *render, int mapSize, int cellSize)
{
    int visibleCells;

    mapSize = Game_validMapSize(mapSize);
    visibleCells = render->boardPixelSize / cellSize;
    if (visibleCells > mapSize) {
        visibleCells = mapSize;
    }
    if (visibleCells < 1) {
        visibleCells = 1;
    }

    return visibleCells;
}

static int boardSizeForMap(const RenderContext *render, int mapSize)
{
    int cellSize = cellSizeForMap(render, mapSize);

    return visibleCellsForMap(render, mapSize, cellSize) * cellSize;
}

static int panelLeft(const RenderContext *render, int mapSize)
{
    return BOARD_LEFT + boardSizeForMap(render, mapSize) + 24;
}

static int clampViewportStart(int focus, int visibleCells, int mapSize)
{
    int start = focus - visibleCells / 2;

    if (start < 0) {
        start = 0;
    }
    if (start + visibleCells > mapSize) {
        start = mapSize - visibleCells;
    }
    if (start < 0) {
        start = 0;
    }

    return start;
}

static bool isInView(Pos pos, int startRow, int startCol, int visibleCells)
{
    return pos.row >= startRow
        && pos.row < startRow + visibleCells
        && pos.col >= startCol
        && pos.col < startCol + visibleCells;
}

static void setFont(int size)
{
    settextstyle(size, 0, _T("Microsoft YaHei"));
    setbkmode(TRANSPARENT);
}

static void drawTextAt(int x, int y, const TCHAR *text, int size, COLORREF color)
{
    setFont(size);
    settextcolor(color);
    outtextxy(x, y, text);
}

static void drawCenteredText(int left, int top, int right, int bottom,
    const TCHAR *text, int size, COLORREF color)
{
    int width;
    int height;

    setFont(size);
    settextcolor(color);
    width = textwidth(text);
    height = textheight(text);
    outtextxy(left + (right - left - width) / 2,
        top + (bottom - top - height) / 2, text);
}

static void drawMenuButton(int index, int selected, const TCHAR *text)
{
    int width = 390;
    int height = 50;
    int left = (gWindowWidth - width) / 2;
    int top = 190 + index * 58;
    bool isSelected = index == selected;

    setfillcolor(isSelected ? RGB(53, 108, 81) : RGB(30, 42, 52));
    setlinecolor(isSelected ? RGB(247, 202, 99) : RGB(91, 110, 122));
    solidrectangle(left, top, left + width, top + height);
    rectangle(left, top, left + width, top + height);
    drawCenteredText(left, top, left + width, top + height, text, 23, RGB(239, 245, 238));
}

static void drawSmallButton(int index, bool selected, const TCHAR *text, int count)
{
    int width = 260;
    int height = 56;
    int gap = 24;
    int total = count * width + (count - 1) * gap;
    int left = (gWindowWidth - total) / 2 + index * (width + gap);
    int top = 330;

    setfillcolor(selected ? RGB(63, 122, 90) : RGB(33, 45, 55));
    setlinecolor(selected ? RGB(248, 211, 106) : RGB(88, 105, 118));
    solidrectangle(left, top, left + width, top + height);
    rectangle(left, top, left + width, top + height);
    drawCenteredText(left, top, left + width, top + height, text, 24, RGB(240, 246, 241));
}

static void drawSettingsRow(int row, bool selected, const TCHAR *label, const TCHAR *value)
{
    int left = 210;
    int top = 200 + row * 62;
    int width = gWindowWidth - left * 2;
    int height = 48;

    setfillcolor(selected ? RGB(49, 83, 77) : RGB(28, 39, 48));
    setlinecolor(selected ? RGB(248, 211, 106) : RGB(83, 101, 114));
    solidrectangle(left, top, left + width, top + height);
    rectangle(left, top, left + width, top + height);
    drawTextAt(left + 22, top + 12, label, 21, RGB(235, 242, 235));
    drawTextAt(left + width - 260, top + 12, value, 21, RGB(247, 202, 99));
}

static void loadTexture(TextureSlot *slot, const TCHAR *folder, TextureId id, int textureSize)
{
    TCHAR path[MAX_PATH];

    slot->loaded = false;
    slot->fallbackColor = FALLBACK_COLORS[id];
    _stprintf_s(path, MAX_PATH, _T("assets\\%s\\%s"), folder, TEXTURE_NAMES[id]);

    if (_taccess(path, 0) == 0) {
        loadimage(&slot->image, path, textureSize, textureSize, true);
        slot->loaded = true;
    }
}

static void drawTexture(const RenderContext *render, TextureId id, int x, int y, int cellSize)
{
    const TextureSlot *slot = &render->textures[id];

    if (slot->loaded) {
        putimage(x, y, (IMAGE *)&slot->image);
        return;
    }

    setfillcolor(slot->fallbackColor);
    solidrectangle(x, y, x + cellSize, y + cellSize);
}

static void drawBoardBackground(const RenderContext *render, int mapSize)
{
    int boardSize = boardSizeForMap(render, mapSize);

    setfillcolor(render->textures[TEX_GROUND].fallbackColor);
    solidrectangle(BOARD_LEFT, BOARD_TOP, BOARD_LEFT + boardSize, BOARD_TOP + boardSize);
}

static void drawCell(const RenderContext *render, int row, int col,
    int startRow, int startCol, int cellSize, CellType cell)
{
    int x = BOARD_LEFT + (col - startCol) * cellSize;
    int y = BOARD_TOP + (row - startRow) * cellSize;

    switch (cell) {
    case CELL_WALL:
        drawTexture(render, TEX_WALL, x, y, cellSize);
        break;
    case CELL_OBSTACLE:
        drawTexture(render, TEX_OBSTACLE, x, y, cellSize);
        break;
    case CELL_FOOD:
        drawTexture(render, TEX_FOOD, x, y, cellSize);
        break;
    case CELL_FOOD_BONUS:
        drawTexture(render, TEX_FOOD_BONUS, x, y, cellSize);
        break;
    case CELL_FOOD_SPEED:
        drawTexture(render, TEX_FOOD_SPEED, x, y, cellSize);
        break;
    case CELL_FOOD_SLOW:
        drawTexture(render, TEX_FOOD_SLOW, x, y, cellSize);
        break;
    case CELL_TRAP:
        drawTexture(render, TEX_TRAP, x, y, cellSize);
        break;
    case CELL_SHIELD:
        drawTexture(render, TEX_SHIELD, x, y, cellSize);
        break;
    case CELL_BATTLE_BOW:
        drawTexture(render, TEX_BOW, x, y, cellSize);
        break;
    case CELL_BATTLE_SHIELD:
        drawTexture(render, TEX_BATTLE_SHIELD, x, y, cellSize);
        break;
    case CELL_BATTLE_SPIKE:
        drawTexture(render, TEX_SPIKE, x, y, cellSize);
        break;
    case CELL_BATTLE_CLOCK:
        drawTexture(render, TEX_CLOCK, x, y, cellSize);
        break;
    default:
        break;
    }
}

static void drawArrow(const ArrowProjectile *arrow, int startRow, int startCol,
    int visibleCells, int cellSize)
{
    int x;
    int y;
    int cx;
    int cy;
    int tipX;
    int tipY;
    int tailX;
    int tailY;
    int wing = cellSize / 3;

    if (!arrow->active || !isInView(arrow->pos, startRow, startCol, visibleCells)) {
        return;
    }

    x = BOARD_LEFT + (arrow->pos.col - startCol) * cellSize;
    y = BOARD_TOP + (arrow->pos.row - startRow) * cellSize;
    cx = x + cellSize / 2;
    cy = y + cellSize / 2;
    tipX = cx;
    tipY = cy;
    tailX = cx;
    tailY = cy;
    if (wing < 2) {
        wing = 2;
    }

    if (arrow->dir == DIR_UP) {
        tipY = y + 1;
        tailY = y + cellSize - 2;
    } else if (arrow->dir == DIR_DOWN) {
        tipY = y + cellSize - 2;
        tailY = y + 1;
    } else if (arrow->dir == DIR_LEFT) {
        tipX = x + 1;
        tailX = x + cellSize - 2;
    } else if (arrow->dir == DIR_RIGHT) {
        tipX = x + cellSize - 2;
        tailX = x + 1;
    }

    setlinecolor(RGB(247, 226, 128));
    setfillcolor(RGB(247, 226, 128));
    line(tailX, tailY, tipX, tipY);
    if (arrow->dir == DIR_UP || arrow->dir == DIR_DOWN) {
        solidcircle(tipX, tipY, wing / 2);
        line(tipX, tipY, cx - wing, cy);
        line(tipX, tipY, cx + wing, cy);
    } else {
        solidcircle(tipX, tipY, wing / 2);
        line(tipX, tipY, cx, cy - wing);
        line(tipX, tipY, cx, cy + wing);
    }
}

static void drawSnake(const RenderContext *render, const Snake *snake,
    int startRow, int startCol, int visibleCells, int cellSize, bool player)
{
    int i;

    if (!snake->alive || snake->length <= 0) {
        return;
    }

    for (i = snake->length - 1; i >= 0; i--) {
        int x;
        int y;
        TextureId id = player
            ? (i == 0 ? TEX_PLAYER_HEAD : TEX_PLAYER_BODY)
            : (i == 0 ? TEX_AI_HEAD : TEX_AI_BODY);

        if (!isInView(snake->body[i], startRow, startCol, visibleCells)) {
            continue;
        }
        x = BOARD_LEFT + (snake->body[i].col - startCol) * cellSize;
        y = BOARD_TOP + (snake->body[i].row - startRow) * cellSize;
        drawTexture(render, id, x, y, cellSize);
        if (i == 0 && (snake->shieldMs > 0 || snake->shieldCharges > 0) && cellSize >= 10) {
            setlinecolor(RGB(122, 232, 169));
            rectangle(x + 2, y + 2, x + cellSize - 3, y + cellSize - 3);
        }
    }
}

static void drawBoardGrid(const RenderContext *render, int visibleCells, int cellSize)
{
    int row;
    int col;
    int boardSize = visibleCells * cellSize;

    if (cellSize < 12) {
        return;
    }

    setlinecolor(RGB(34, 51, 48));
    for (row = 0; row <= visibleCells; row++) {
        int y = BOARD_TOP + row * cellSize;
        line(BOARD_LEFT, y, BOARD_LEFT + boardSize, y);
    }
    for (col = 0; col <= visibleCells; col++) {
        int x = BOARD_LEFT + col * cellSize;
        line(x, BOARD_TOP, x, BOARD_TOP + boardSize);
    }
}

static const TCHAR *modeText(GameMode mode)
{
    switch (mode) {
    case MODE_SINGLE:
        return _T("单人模式");
    case MODE_AI_BATTLE:
        return _T("AI 对战");
    case MODE_TIME_CHALLENGE:
        return _T("限时挑战");
    case MODE_LOCAL_MULTIPLAYER:
        return _T("本地多人");
    default:
        return _T("未知模式");
    }
}

static const TCHAR *variantText(MapVariant variant)
{
    return variant == VARIANT_DIVERSE ? _T("多样模式") : _T("常规模式");
}

static const TCHAR *difficultyText(AiDifficulty difficulty)
{
    switch (difficulty) {
    case AI_EASY:
        return _T("低难度");
    case AI_MEDIUM:
        return _T("中难度");
    case AI_HARD:
        return _T("高难度");
    default:
        return _T("中难度");
    }
}

bool Render_init(RenderContext *render, int skinId)
{
    const ResolutionInfo *info;
    int windowWidth;
    int windowHeight;

    render->resolution = RESOLUTION_HD_PLUS;
    render->fullscreen = false;
    info = resolutionInfo(render->resolution);
    resolveDisplaySize(info, render->fullscreen, &windowWidth, &windowHeight);
    render->windowWidth = windowWidth;
    render->windowHeight = windowHeight;
    render->boardPixelSize = boardPixelSizeForWindow(windowWidth, windowHeight);
    render->skinId = 0;
    render->textureCellSize = TEXTURE_SIZE;
    render->skinName[0] = _T('\0');
    gWindowWidth = render->windowWidth;
    gWindowHeight = render->windowHeight;
    initgraph(render->windowWidth, render->windowHeight);
    applyNativeWindowMode(render->fullscreen, render->windowWidth, render->windowHeight);
    setbkcolor(RGB(13, 19, 24));
    BeginBatchDraw();
    return Render_loadSkin(render, skinId);
}

void Render_shutdown(void)
{
    EndBatchDraw();
    closegraph();
}

bool Render_applyResolution(RenderContext *render, DisplayResolution resolution)
{
    return Render_applyDisplayMode(render, resolution, render->fullscreen);
}

bool Render_applyDisplayMode(RenderContext *render, DisplayResolution resolution, bool fullscreen)
{
    const ResolutionInfo *info = resolutionInfo(resolution);
    int windowWidth;
    int windowHeight;
    int boardPixelSize;

    resolveDisplaySize(info, fullscreen, &windowWidth, &windowHeight);
    boardPixelSize = boardPixelSizeForWindow(windowWidth, windowHeight);

    if (render->resolution == info->id
        && render->fullscreen == fullscreen
        && render->windowWidth == windowWidth
        && render->windowHeight == windowHeight
        && render->boardPixelSize == boardPixelSize) {
        return true;
    }

    EndBatchDraw();
    closegraph();
    render->resolution = info->id;
    render->fullscreen = fullscreen;
    render->windowWidth = windowWidth;
    render->windowHeight = windowHeight;
    render->boardPixelSize = boardPixelSize;
    render->textureCellSize = 0;
    gWindowWidth = render->windowWidth;
    gWindowHeight = render->windowHeight;
    initgraph(render->windowWidth, render->windowHeight);
    applyNativeWindowMode(render->fullscreen, render->windowWidth, render->windowHeight);
    setbkcolor(RGB(13, 19, 24));
    BeginBatchDraw();
    return Render_loadSkin(render, render->skinId);
}

bool Render_loadSkin(RenderContext *render, int skinId)
{
    int i;
    int textureSize;
    const TCHAR *folder;

    if (skinId < 0 || skinId >= Render_skinCount()) {
        skinId = 0;
    }
    if (render->textureCellSize <= 0) {
        render->textureCellSize = TEXTURE_SIZE;
    }

    textureSize = render->textureCellSize;
    render->skinId = skinId;
    _tcscpy_s(render->skinName, 64, SKINS[skinId].name);
    folder = SKINS[skinId].folder;
    for (i = 0; i < TEX_COUNT; i++) {
        loadTexture(&render->textures[i], folder, (TextureId)i, textureSize);
    }

    return true;
}

int Render_skinCount(void)
{
    return (int)(sizeof(SKINS) / sizeof(SKINS[0]));
}

const TCHAR *Render_skinDisplayName(int skinId)
{
    if (skinId < 0 || skinId >= Render_skinCount()) {
        skinId = 0;
    }

    return SKINS[skinId].name;
}

const TCHAR *Render_resolutionDisplayName(DisplayResolution resolution)
{
    return resolutionInfo(resolution)->name;
}

static void ensureGameTextureSize(RenderContext *render, int cellSize)
{
    if (render->textureCellSize == cellSize) {
        return;
    }

    render->textureCellSize = cellSize;
    Render_loadSkin(render, render->skinId);
}

void Render_drawWelcome(int selected)
{
    static const TCHAR *OPTIONS[] = {
        _T("单人模式"),
        _T("AI 对战模式"),
        _T("限时挑战模式"),
        _T("本地多人模式"),
        _T("更换时装"),
        _T("设置"),
        _T("退出游戏")
    };
    int i;

    cleardevice();
    setfillcolor(RGB(13, 19, 24));
    solidrectangle(0, 0, gWindowWidth, gWindowHeight);
    drawCenteredText(0, 72, gWindowWidth, 132, _T("图形化贪吃蛇"), 44, RGB(241, 246, 239));
    drawCenteredText(0, 137, gWindowWidth, 175, _T("BUPT EasyX Edition"), 20, RGB(132, 185, 158));
    for (i = 0; i < 7; i++) {
        drawMenuButton(i, selected, OPTIONS[i]);
    }
    FlushBatchDraw();
}

void Render_drawVariantMenu(MapVariant selected)
{
    cleardevice();
    setfillcolor(RGB(13, 19, 24));
    solidrectangle(0, 0, gWindowWidth, gWindowHeight);
    drawCenteredText(0, 145, gWindowWidth, 200, _T("选择玩法规则"), 38, RGB(242, 247, 241));
    drawSmallButton(0, selected == VARIANT_CLASSIC, _T("常规模式"), 2);
    drawSmallButton(1, selected == VARIANT_DIVERSE, _T("多样模式"), 2);
    FlushBatchDraw();
}

void Render_drawDifficultyMenu(AiDifficulty selected)
{
    cleardevice();
    setfillcolor(RGB(13, 19, 24));
    solidrectangle(0, 0, gWindowWidth, gWindowHeight);
    drawCenteredText(0, 145, gWindowWidth, 200, _T("选择 AI 难度"), 38, RGB(242, 247, 241));
    drawSmallButton(0, selected == AI_EASY, _T("低"), 3);
    drawSmallButton(1, selected == AI_MEDIUM, _T("中"), 3);
    drawSmallButton(2, selected == AI_HARD, _T("高"), 3);
    FlushBatchDraw();
}

void Render_drawSkinMenu(int selectedSkin)
{
    int i;

    cleardevice();
    setfillcolor(RGB(13, 19, 24));
    solidrectangle(0, 0, gWindowWidth, gWindowHeight);
    drawCenteredText(0, 110, gWindowWidth, 165, _T("更换时装"), 40, RGB(242, 247, 241));
    for (i = 0; i < Render_skinCount(); i++) {
        drawMenuButton(i, selectedSkin, Render_skinDisplayName(i));
    }
    FlushBatchDraw();
}

void Render_drawSettings(const GameConfig *config, int selectedRow)
{
    TCHAR value[64];

    cleardevice();
    setfillcolor(RGB(13, 19, 24));
    solidrectangle(0, 0, gWindowWidth, gWindowHeight);
    drawCenteredText(0, 74, gWindowWidth, 128, _T("设置"), 40, RGB(242, 247, 241));
    drawCenteredText(0, 136, gWindowWidth, 168,
        _T("W/S 选择，A/D 修改，Enter 返回"), 18, RGB(144, 190, 163));

    _stprintf_s(value, 64, _T("%s"), config->enableStepGrowth ? _T("开启") : _T("关闭"));
    drawSettingsRow(0, selectedRow == 0, _T("N 步自动增长"), value);
    _stprintf_s(value, 64, _T("%d x %d"), config->mapSize, config->mapSize);
    drawSettingsRow(1, selectedRow == 1, _T("地图尺寸"), value);
    _stprintf_s(value, 64, _T("%s"), Render_resolutionDisplayName(config->resolution));
    drawSettingsRow(2, selectedRow == 2, _T("窗口分辨率"), value);
    _stprintf_s(value, 64, _T("%s"), config->fullscreen ? _T("开启") : _T("关闭"));
    drawSettingsRow(3, selectedRow == 3, _T("全屏显示"), value);
    _stprintf_s(value, 64, _T("%s"), config->musicEnabled ? _T("开启") : _T("关闭"));
    drawSettingsRow(4, selectedRow == 4, _T("背景音乐"), value);
    _stprintf_s(value, 64, _T("%s"), config->soundEnabled ? _T("开启") : _T("关闭"));
    drawSettingsRow(5, selectedRow == 5, _T("游戏音效"), value);

    FlushBatchDraw();
}

void Render_drawGame(RenderContext *render, const GameState *state,
    bool paused, bool waitingForStart)
{
    int mapSize = Game_validMapSize(state->config.mapSize);
    int cellSize = cellSizeForMap(render, mapSize);
    int visibleCells = visibleCellsForMap(render, mapSize, cellSize);
    int boardSize = boardSizeForMap(render, mapSize);
    Pos focus = state->player.length > 0 ? state->player.body[0] : state->ai.body[0];
    int startRow = clampViewportStart(focus.row, visibleCells, mapSize);
    int startCol = clampViewportStart(focus.col, visibleCells, mapSize);
    int row;
    int col;
    int x = panelLeft(render, mapSize);
    TCHAR buffer[128];

    ensureGameTextureSize(render, cellSize);
    cleardevice();
    setfillcolor(RGB(13, 19, 24));
    solidrectangle(0, 0, render->windowWidth, render->windowHeight);

    drawBoardBackground(render, mapSize);

    /* 轰炸区地面染色（在道具/蛇之前，不遮挡） */
    if (state->event.activeEvent == EVENT_BOMBARDMENT) {
        int z;
        COLORREF zoneColor;
        if (state->event.bombActive) {
            zoneColor = RGB(180, 30, 20);
        } else if (state->event.bombWarning) {
            zoneColor = RGB(230, 110, 40);
        } else {
            zoneColor = RGB(200, 55, 40);
        }
        for (z = 0; z < state->event.zoneCount; z++) {
            int rStart = state->event.zones[z].rowStart;
            int rEnd = state->event.zones[z].rowEnd;
            int cStart = state->event.zones[z].colStart;
            int cEnd = state->event.zones[z].colEnd;
            int vrStart = (rStart > startRow) ? rStart : startRow;
            int vrEnd = (rEnd < startRow + visibleCells - 1) ? rEnd : startRow + visibleCells - 1;
            int vcStart = (cStart > startCol) ? cStart : startCol;
            int vcEnd = (cEnd < startCol + visibleCells - 1) ? cEnd : startCol + visibleCells - 1;
            int br, bc;

            for (br = vrStart; br <= vrEnd; br++) {
                for (bc = vcStart; bc <= vcEnd; bc++) {
                    int bx = BOARD_LEFT + (bc - startCol) * cellSize;
                    int by = BOARD_TOP + (br - startRow) * cellSize;
                    setfillcolor(zoneColor);
                    setlinecolor(zoneColor);
                    solidrectangle(bx + 1, by + 1, bx + cellSize - 1, by + cellSize - 1);
                }
            }
        }
    }

    for (row = startRow; row < startRow + visibleCells; row++) {
        for (col = startCol; col < startCol + visibleCells; col++) {
            if (state->cells[row][col] != CELL_EMPTY) {
                drawCell(render, row, col, startRow, startCol, cellSize, state->cells[row][col]);
            }
        }
    }
    drawSnake(render, &state->player, startRow, startCol, visibleCells, cellSize, true);
    drawSnake(render, &state->ai, startRow, startCol, visibleCells, cellSize, false);
    for (row = 0; row < MAX_ACTIVE_ARROWS; row++) {
        drawArrow(&state->arrows[row], startRow, startCol, visibleCells, cellSize);
    }

    /* 箭雨边界闪烁 */
    if (state->event.activeEvent == EVENT_ARROW_STORM && state->event.borderFlashing) {
        int k;
        for (k = 0; k < state->event.borderSourceCount; k++) {
            Pos p = state->event.borderSources[k].pos;
            if (isInView(p, startRow, startCol, visibleCells)) {
                int bx = BOARD_LEFT + (p.col - startCol) * cellSize;
                int by = BOARD_TOP + (p.row - startRow) * cellSize;
                int tcx = bx + cellSize / 2;
                int tcy = by + cellSize / 2;

                setfillcolor(RGB(255, 210, 50));
                setlinecolor(RGB(255, 160, 20));
                solidrectangle(bx + 2, by + 2, bx + cellSize - 2, by + cellSize - 2);

                switch (state->event.borderSources[k].dir) {
                case DIR_UP:    tcy = by + 4; break;
                case DIR_DOWN:  tcy = by + cellSize - 4; break;
                case DIR_LEFT:  tcx = bx + 4; break;
                case DIR_RIGHT: tcx = bx + cellSize - 4; break;
                default: break;
                }
                setfillcolor(RGB(255, 80, 40));
                solidcircle(tcx, tcy, cellSize / 5);
            }
        }
    }

    drawBoardGrid(render, visibleCells, cellSize);

    setfillcolor(RGB(24, 34, 42));
    solidrectangle(x, BOARD_TOP, render->windowWidth - 24, BOARD_TOP + boardSize);
    drawTextAt(x + 20, BOARD_TOP + 24, modeText(state->config.mode), 26, RGB(241, 246, 239));
    drawTextAt(x + 20, BOARD_TOP + 70, variantText(state->config.variant), 20, RGB(144, 190, 163));
    _stprintf_s(buffer, 128, _T("地图：%d x %d"), mapSize, mapSize);
    drawTextAt(x + 20, BOARD_TOP + 102, buffer, 18, RGB(144, 190, 163));

    if (state->event.activeEvent != EVENT_NONE) {
        const TCHAR *eventName = (state->event.activeEvent == EVENT_BOMBARDMENT)
            ? _T("事件：地图轰炸") : _T("事件：刀光箭影");
        int remainSec = (state->event.eventTimerMs + 999) / 1000;
        if (remainSec < 0) remainSec = 0;
        _stprintf_s(buffer, 128, _T("%s (%d s)"), eventName, remainSec);
        drawTextAt(x + 20, BOARD_TOP + 126, buffer, 16, RGB(255, 140, 60));

        if (state->event.bombWarning) {
            drawCenteredText(x, BOARD_TOP + 160,
                render->windowWidth - 24, BOARD_TOP + 200,
                _T("!!! 即将轰炸 !!!"), 26, RGB(255, 30, 20));
        }
    }

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

        drawTextAt(x + 20, BOARD_TOP + boardSize - 64, render->skinName, 20, RGB(160, 197, 181));
    } else {
        _stprintf_s(buffer, 128, _T("速度档：%+d"), state->speedLevel);
        drawTextAt(x + 20, BOARD_TOP + 130, buffer, 18, RGB(247, 202, 99));
        drawTextAt(x + 20, BOARD_TOP + 168, _T("玩家得分"), 20, RGB(178, 194, 194));
        _stprintf_s(buffer, 128, _T("%d"), state->player.score);
        drawTextAt(x + 20, BOARD_TOP + 196, buffer, 32, RGB(247, 202, 99));
        _stprintf_s(buffer, 128, _T("长度：%d"), state->player.length);
        drawTextAt(x + 20, BOARD_TOP + 248, buffer, 20, RGB(225, 235, 224));
        _stprintf_s(buffer, 128, _T("弓箭：%d  护盾：%d"),
            state->player.bowArrows, state->player.shieldCharges);
        drawTextAt(x + 20, BOARD_TOP + 278, buffer, 18, RGB(204, 214, 222));

        if (state->config.mode == MODE_AI_BATTLE) {
            _stprintf_s(buffer, 128, _T("AI 分数：%d"), state->ai.score);
            drawTextAt(x + 20, BOARD_TOP + 330, buffer, 20, RGB(242, 122, 119));
            _stprintf_s(buffer, 128, _T("AI 弓箭：%d  护盾：%d"),
                state->ai.bowArrows, state->ai.shieldCharges);
            drawTextAt(x + 20, BOARD_TOP + 360, buffer, 18, RGB(204, 214, 222));
            drawTextAt(x + 20, BOARD_TOP + 392, difficultyText(state->config.aiDifficulty), 20, RGB(204, 214, 222));
        }

        if (state->config.mode == MODE_TIME_CHALLENGE) {
            _stprintf_s(buffer, 128, _T("剩余时间：%d s"), state->remainingSeconds);
            drawTextAt(x + 20, BOARD_TOP + 330, buffer, 22, RGB(103, 196, 224));
        }

        drawTextAt(x + 20, BOARD_TOP + boardSize - 64, render->skinName, 20, RGB(160, 197, 181));
    }

    if (state->config.mode == MODE_LOCAL_MULTIPLAYER) {
        drawTextAt(x + 20, BOARD_TOP + boardSize - 36,
            _T("P1: E射箭  P2: /射箭"), 16, RGB(160, 197, 181));
    } else {
        drawTextAt(x + 20, BOARD_TOP + boardSize - 36,
            _T("E 射箭  1 加速  2 减速"), 17, RGB(160, 197, 181));
    }

    if (waitingForStart || paused) {
        int left = BOARD_LEFT + boardSize / 2 - 190;
        int top = BOARD_TOP + boardSize / 2 - 52;
        int right = left + 380;
        int bottom = top + 104;
        const TCHAR *text;
        if (waitingForStart) {
            text = (state->config.mode == MODE_LOCAL_MULTIPLAYER)
                ? _T("P1: W/A/S/D  P2: 方向键  开始")
                : _T("按 W/A/S/D 开始");
        } else {
            text = _T("暂停");
        }

        setfillcolor(RGB(16, 22, 27));
        solidrectangle(left, top, right, bottom);
        setlinecolor(RGB(247, 202, 99));
        rectangle(left, top, right, bottom);
        drawCenteredText(left, top, right, bottom, text, 32, RGB(247, 202, 99));
    }

    FlushBatchDraw();
}

void Render_drawGameOver(const GameState *state, int selectedAction)
{
    TCHAR score[128];
    const TCHAR *title;

    cleardevice();
    setfillcolor(RGB(13, 19, 24));
    solidrectangle(0, 0, gWindowWidth, gWindowHeight);
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

    drawCenteredText(0, 130, gWindowWidth, 190, title, 44, RGB(241, 246, 239));
    _stprintf_s(score, 128, _T("玩家一：%d"), state->player.score);
    drawCenteredText(0, 215, gWindowWidth, 260, score, 28, RGB(247, 202, 99));
    if (state->config.mode == MODE_AI_BATTLE || state->config.mode == MODE_LOCAL_MULTIPLAYER) {
        const TCHAR *p2Label = (state->config.mode == MODE_LOCAL_MULTIPLAYER)
            ? _T("玩家二") : _T("AI");
        _stprintf_s(score, 128, _T("%s：%d"), p2Label, state->ai.score);
        drawCenteredText(0, 260, gWindowWidth, 305, score, 24,
            state->config.mode == MODE_LOCAL_MULTIPLAYER ? RGB(86, 206, 144) : RGB(242, 122, 119));
    }
    drawSmallButton(0, selectedAction == 0, _T("重新开始"), 2);
    drawSmallButton(1, selectedAction == 1, _T("返回菜单"), 2);
    FlushBatchDraw();
}
