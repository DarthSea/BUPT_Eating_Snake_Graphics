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
    COLOR_BOARD,
    COLOR_WALL_COLOR,
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
static int gMenuBgTimer = 0;

/* 菜单装饰小蛇 */
#define MENU_SNAKE_COUNT 2
#define MENU_SNAKE_MAXLEN 6
typedef struct {
    Pos body[MENU_SNAKE_MAXLEN];
    int length;
    Direction dir;
    int moveTimer;
} MenuBgSnake;
static MenuBgSnake gMenuSnakes[MENU_SNAKE_COUNT];
static Pos gMenuFoods[3];
static bool gMenuSnakesInit = false;

static void initMenuBgSnakes(void)
{
    int i, j;
    for (i = 0; i < MENU_SNAKE_COUNT; i++) {
        MenuBgSnake *s = &gMenuSnakes[i];
        s->length = 4 + i;
        s->dir = (Direction)(i % 4);
        s->moveTimer = 0;
        s->body[0].row = 30 + i * 60;
        s->body[0].col = 40 + i * 80;
        for (j = 1; j < s->length; j++) {
            s->body[j] = s->body[j-1];
            if (s->dir == DIR_RIGHT) s->body[j].col--;
            else if (s->dir == DIR_UP) s->body[j].row++;
            else if (s->dir == DIR_DOWN) s->body[j].row--;
            else s->body[j].col++;
        }
    }
    gMenuFoods[0].row = 15; gMenuFoods[0].col = 25;
    gMenuFoods[1].row = 55; gMenuFoods[1].col = 70;
    gMenuFoods[2].row = 80; gMenuFoods[2].col = 45;
    gMenuSnakesInit = true;
}

static void updateMenuBgSnakes(int deltaMs)
{
    int i, j;
    if (!gMenuSnakesInit) initMenuBgSnakes();
    gMenuBgTimer += deltaMs;
    if (gMenuBgTimer < 300) return;
    gMenuBgTimer = 0;

    for (i = 0; i < MENU_SNAKE_COUNT; i++) {
        MenuBgSnake *s = &gMenuSnakes[i];
        Pos head = s->body[0];
        Pos next = Common_nextPos(head, s->dir);

        /* 碰边界转向 */
        if (next.row < 2 || next.row > 98 || next.col < 2 || next.col > 98) {
            s->dir = (Direction)((s->dir + 1 + rand() % 3) % 4);
            next = Common_nextPos(head, s->dir);
        }
        /* 碰食物 */
        for (j = 0; j < 3; j++) {
            if (next.row == gMenuFoods[j].row && next.col == gMenuFoods[j].col) {
                if (s->length < MENU_SNAKE_MAXLEN) s->length++;
                gMenuFoods[j].row = 3 + rand() % 94;
                gMenuFoods[j].col = 3 + rand() % 94;
            }
        }
        /* 移动 */
        for (j = s->length - 1; j > 0; j--) s->body[j] = s->body[j-1];
        s->body[0] = next;
    }
}

static void drawMenuBgSnakes(void)
{
    int i, j;
    for (i = 0; i < MENU_SNAKE_COUNT; i++) {
        MenuBgSnake *s = &gMenuSnakes[i];
        COLORREF color = (i == 0) ? RGB(64, 112, 160) : RGB(80, 96, 160);
        for (j = 0; j < s->length; j++) {
            int px = s->body[j].col * gWindowWidth / 100;
            int py = s->body[j].row * gWindowHeight / 100;
            int r = (int)(3.5f - j * 0.5f);
            if (r < 1) r = 1;
            setfillcolor(color);
            solidcircle(px, py, r);
        }
    }
    /* 食物 */
    for (i = 0; i < 3; i++) {
        int fx = gMenuFoods[i].col * gWindowWidth / 100;
        int fy = gMenuFoods[i].row * gWindowHeight / 100;
        setfillcolor(RGB(255, 112, 64));
        solidcircle(fx, fy, 2);
    }
}

/* 菜单通用背景：渐变 + 小蛇 + 粒子 */
static void drawMenuBackground(void)
{
    int y, step = 4;
    for (y = 0; y < gWindowHeight; y += step) {
        int r = 22 + 8 * y / gWindowHeight;
        int g = 32 + 14 * y / gWindowHeight;
        int b = 48 + 24 * y / gWindowHeight;
        setfillcolor(RGB(r, g, b));
        solidrectangle(0, y, gWindowWidth, y + step);
    }
    drawMenuBgSnakes();
}

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

int Render_cellSizeForMap(const RenderContext *render, int mapSize)
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
    int cellSize = Render_cellSizeForMap(render, mapSize);

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
    LOGFONT lf = { 0 };
    lf.lfHeight = size;
    lf.lfQuality = CLEARTYPE_QUALITY;
    _tcscpy_s(lf.lfFaceName, 32, _T("Microsoft YaHei"));
    settextstyle(&lf);
    settextcolor(color);
    setbkmode(TRANSPARENT);
    outtextxy(x, y, text);
}

static void drawCenteredText(int left, int top, int right, int bottom,
    const TCHAR *text, int size, COLORREF color)
{
    int width;
    int height;
    LOGFONT lf = { 0 };
    lf.lfHeight = size;
    lf.lfQuality = CLEARTYPE_QUALITY;
    _tcscpy_s(lf.lfFaceName, 32, _T("Microsoft YaHei"));
    settextstyle(&lf);
    settextcolor(color);
    setbkmode(TRANSPARENT);
    width = textwidth(text);
    height = textheight(text);
    outtextxy(left + (right - left - width) / 2,
        top + (bottom - top - height) / 2, text);
}

/* 带阴影的卡片：先画偏移阴影，再画卡片本体 */
static void drawCardWithShadow(int left, int top, int right, int bottom, int radius,
    COLORREF fill)
{
    /* 阴影：偏移 3px */
    setfillcolor(COLOR_SHADOW);
    solidrectangle(left + 3, top + 3, right + 3, bottom + 3);
    /* 卡片本体 */
    setfillcolor(fill);
    solidrectangle(left + radius, top, right - radius, bottom);
    solidrectangle(left, top + radius, right, bottom - radius);
    solidcircle(left + radius, top + radius, radius);
    solidcircle(right - radius, top + radius, radius);
    solidcircle(left + radius, bottom - radius, radius);
    solidcircle(right - radius, bottom - radius, radius);
}

/* 模拟圆角矩形：中心矩形 + 四角圆形，扁平无边线 */
static void drawRoundedRect(int left, int top, int right, int bottom, int radius,
    COLORREF fill, COLORREF border)
{
    (void)border; /* 扁平风格不用边线 */
    setfillcolor(fill);
    /* 主体 */
    solidrectangle(left + radius, top, right - radius, bottom);
    solidrectangle(left, top + radius, right, bottom - radius);
    /* 四角 */
    solidcircle(left + radius, top + radius, radius);
    solidcircle(right - radius, top + radius, radius);
    solidcircle(left + radius, bottom - radius, radius);
    solidcircle(right - radius, bottom - radius, radius);
}

/* 文字发光效果 — 四角偏移暗色光晕 */
static void drawTextGlow(int x, int y, const TCHAR *text, int size,
    COLORREF color, COLORREF glowColor)
{
    int r;
    LOGFONT lf = { 0 };
    lf.lfHeight = size;
    lf.lfQuality = CLEARTYPE_QUALITY;
    _tcscpy_s(lf.lfFaceName, 32, _T("Microsoft YaHei"));
    settextstyle(&lf);
    setbkmode(TRANSPARENT);

    /* 多层发光：从大到小递减偏移，产生光晕效果 */
    int offsets[][2] = {{-3,0},{3,0},{0,-3},{0,3},{-2,-2},{2,-2},{-2,2},{2,2},
        {-1,-1},{1,-1},{-1,1},{1,1}};
    for (r = 0; r < 12; r++) {
        settextcolor(glowColor);
        outtextxy(x + offsets[r][0], y + offsets[r][1], text);
    }

    settextcolor(color);
    outtextxy(x, y, text);
}

/* 文字描边效果 — 8方向偏移描边 */
static void drawTextOutline(int x, int y, const TCHAR *text, int size,
    COLORREF color, COLORREF outlineColor)
{
    LOGFONT lf = { 0 };
    lf.lfHeight = size;
    lf.lfQuality = CLEARTYPE_QUALITY;
    _tcscpy_s(lf.lfFaceName, 32, _T("Microsoft YaHei"));
    settextstyle(&lf);
    setbkmode(TRANSPARENT);

    settextcolor(outlineColor);
    {
        int d;
        int dirs[8][2] = {{-1,-1},{0,-1},{1,-1},{-1,0},{1,0},{-1,1},{0,1},{1,1}};
        for (d = 0; d < 8; d++) {
            outtextxy(x + dirs[d][0], y + dirs[d][1], text);
        }
    }

    settextcolor(color);
    outtextxy(x, y, text);
}

/* 居中文字 + 发光 */
static void drawCenteredTextGlow(int left, int top, int right, int bottom,
    const TCHAR *text, int size, COLORREF color, COLORREF glowColor)
{
    int w, h, x, y, r;
    LOGFONT lf = { 0 };
    lf.lfHeight = size;
    lf.lfQuality = CLEARTYPE_QUALITY;
    _tcscpy_s(lf.lfFaceName, 32, _T("Microsoft YaHei"));
    settextstyle(&lf);
    setbkmode(TRANSPARENT);
    w = textwidth(text);
    h = textheight(text);
    x = left + (right - left - w) / 2;
    y = top + (bottom - top - h) / 2;

    /* 多层光晕 */
    int offsets[][2] = {{-3,0},{3,0},{0,-3},{0,3},{-2,-2},{2,-2},{-2,2},{2,2},
        {-1,-1},{1,-1},{-1,1},{1,1}};
    for (r = 0; r < 12; r++) {
        settextcolor(glowColor);
        outtextxy(x + offsets[r][0], y + offsets[r][1], text);
    }

    settextcolor(color);
    outtextxy(x, y, text);
}

/* 道具标签 — 圆角小方块 + 文字 */
typedef enum { ICON_LENGTH = 0, ICON_ARROW, ICON_SHIELD } TagIcon;

/* 精细像素图标 — 模拟 emoji 风格 */
static void drawTagIcon(int cx, int cy, int sz, TagIcon icon, COLORREF color)
{
    int midY = cy + sz/2;
    setfillcolor(color);
    setlinecolor(color);
    if (icon == ICON_LENGTH) {
        /* 🍖 蛇身：四节身体 + 眼睛头 */
        int segR = sz / 5;
        solidcircle(cx + sz - segR,    midY, segR);      /* 头 */
        solidcircle(cx + sz - segR*3,  midY, segR - 1);  /* 段1 */
        solidcircle(cx + segR*3,       midY, segR - 1);  /* 段2 */
        solidcircle(cx + segR,         midY, segR - 1);  /* 段3 */
        /* 眼睛 */
        setfillcolor(RGB(255,255,255));
        solidcircle(cx + sz - segR + 1, midY - 1, segR/3);
        setfillcolor(RGB(20,20,20));
        solidcircle(cx + sz - segR + 2, midY - 1, segR/5);
    } else if (icon == ICON_ARROW) {
        /* 🏹 弓+箭 */
        int bowX = cx + sz/4;
        /* 弓弧（圆形边框模拟） */
        setlinecolor(color);
        arc(bowX - sz/3, cy, bowX + sz/3, cy + sz, 0.0f, 3.14f);
        arc(bowX - sz/3, cy, bowX + sz/3, cy + sz, 3.14f, 6.28f);
        /* 弦 */
        line(bowX, cy + 2, bowX, cy + sz - 2);
        /* 箭杆 */
        line(bowX, midY, cx + sz, midY);
        /* 箭头 */
        line(cx + sz - 4, midY - 3, cx + sz, midY);
        line(cx + sz - 4, midY + 3, cx + sz, midY);
        /* 尾羽 */
        line(cx + sz/2, midY - 2, cx + sz/2 - 3, midY - 4);
        line(cx + sz/2, midY + 2, cx + sz/2 - 3, midY + 4);
    } else {
        /* 🛡 盾牌 + 十字 */
        int shW = sz;
        int shH = sz;
        /* 盾体 */
        {
            POINT shield[5] = {
                {cx, cy + shH/6},
                {cx + shW/2, cy},
                {cx + shW, cy + shH/6},
                {cx + shW, cy + shH/2},
                {cx + shW/2, cy + shH}
            };
            setfillcolor(color);
            solidpolygon(shield, 5);
        }
        /* 十字内部 */
        setfillcolor(COLOR_BG);
        solidrectangle(cx + shW*2/5, cy + shH/4, cx + shW*3/5, cy + shH*3/5);
        solidrectangle(cx + shW/3, cy + shH*2/5, cx + shW*2/3, cy + shH*3/5);
    }
}

static void drawTag(int x, int y, const TCHAR *label, int value, COLORREF color, float txtScale)
{
    TCHAR buf[32];
    int tagW = (int)(64 * txtScale);
    int tagH = (int)(24 * txtScale);
    int r = 4;
    int iconSz = (int)(12 * txtScale);

    setfillcolor(COLOR_BG);
    solidrectangle(x + r, y, x + tagW - r, y + tagH);
    solidrectangle(x, y + r, x + tagW, y + tagH - r);
    solidcircle(x + r, y + r, r);
    solidcircle(x + tagW - r, y + r, r);
    solidcircle(x + r, y + tagH - r, r);
    solidcircle(x + tagW - r, y + tagH - r, r);

    /* 边框 */
    setlinecolor(COLOR_BORDER);
    line(x + r, y, x + tagW - r, y);
    line(x + r, y + tagH, x + tagW - r, y + tagH);
    line(x, y + r, x, y + tagH - r);
    line(x + tagW, y + r, x + tagW, y + tagH - r);

    /* 图标 */
    {
        TagIcon icon;
        if (_tcscmp(label, _T("长度")) == 0) icon = ICON_LENGTH;
        else if (_tcscmp(label, _T("弓箭")) == 0) icon = ICON_ARROW;
        else icon = ICON_SHIELD;
        drawTagIcon(x + (int)(6 * txtScale), y + (tagH - iconSz)/2, iconSz, icon, color);
    }
    _stprintf_s(buf, 32, _T("%s %d"), label, value);
    drawTextAt(x + (int)(22 * txtScale), y + (int)(3 * txtScale), buf, (int)(12 * txtScale), color);
}

/* 菜单缩放因子 */
static float menuScale(void)
{
    float s = gWindowWidth / 1280.0f;
    if (s < 0.85f) s = 0.85f;
    if (s > 1.5f) s = 1.5f;
    return s;
}

static void drawMenuButton(int index, int selected, const TCHAR *text)
{
    float ms = menuScale();
    int width = (int)(gWindowWidth * 0.48f);
    if (width > (int)(560 * ms)) width = (int)(560 * ms);
    if (width < (int)(300 * ms)) width = (int)(300 * ms);
    int height = (int)(58 * ms);
    int gap = (int)(10 * ms);
    int left = (gWindowWidth - width) / 2;
    int top = (int)(170 * ms) + index * (height + gap);
    bool isSelected = index == selected;
    int r = (int)(8 * ms);
    int numR = (int)(14 * ms);

    /* 卡片背景 — 圆角纯色，选中加单层描边 */
    setfillcolor(isSelected ? COLOR_MENU_GRAD : COLOR_MENU_CARD);
    solidrectangle(left + r, top, left + width - r, top + height);
    solidrectangle(left, top + r, left + width, top + height - r);
    solidcircle(left + r, top + r, r);
    solidcircle(left + width - r, top + r, r);
    solidcircle(left + r, top + height - r, r);
    solidcircle(left + width - r, top + height - r, r);

    if (isSelected) {
        /* 单层描边 — 用 2px 粗线 */
        setlinecolor(COLOR_ACCENT);
        rectangle(left + 2, top + 2, left + width - 2, top + height - 2);
    }

    /* 圆形编号 */
    {
        int numX = left + (int)(22 * ms);
        int numY = top + height / 2;
        TCHAR num[4];
        _stprintf_s(num, 4, _T("%d"), index + 1);
        if (isSelected) {
            setfillcolor(COLOR_ACCENT);
            solidcircle(numX, numY, numR);
            drawCenteredText(numX - numR, numY - numR, numX + numR, numY + numR,
                num, (int)(16 * ms), RGB(13, 21, 32));
        } else {
            setfillcolor(COLOR_MENU_NUM);
            solidcircle(numX, numY, numR);
            drawCenteredText(numX - numR, numY - numR, numX + numR, numY + numR,
                num, (int)(16 * ms), COLOR_TEXT_DIM);
        }
    }

    /* 文字 */
    drawTextAt(left + (int)(50 * ms), top + (height - (int)(22 * ms))/2,
        text, (int)(18 * ms),
        isSelected ? COLOR_ACCENT : RGB(192, 200, 212));
}

static void drawSmallButton(int index, bool selected, const TCHAR *text, int count)
{
    float ms = menuScale();
    int width = (int)(gWindowWidth * 0.20f);
    if (width > (int)(200 * ms)) width = (int)(200 * ms);
    if (width < (int)(120 * ms)) width = (int)(120 * ms);
    int height = (int)(42 * ms);
    int gap = (int)(20 * ms);
    int total = count * width + (count - 1) * gap;
    int left = (gWindowWidth - total) / 2 + index * (width + gap);
    /* 放在主按钮下方 */
    int top = (int)(170 * ms) + 4 * ((int)(58 * ms) + (int)(10 * ms)) + (int)(20 * ms);
    int r = (int)(6 * ms);

    if (selected) {
        setfillcolor(index == 2 ? COLOR_MENU_EXIT : COLOR_ACCENT);
    } else {
        setfillcolor(index == 2 ? COLOR_MENU_EXIT : COLOR_MENU_CARD);
    }
    solidrectangle(left + r, top, left + width - r, top + height);
    solidrectangle(left, top + r, left + width, top + height - r);
    solidcircle(left + r, top + r, r);
    solidcircle(left + width - r, top + r, r);
    solidcircle(left + r, top + height - r, r);
    solidcircle(left + width - r, top + height - r, r);

    COLORREF txtColor;
    if (selected) {
        txtColor = (index == 2) ? COLOR_DANGER : RGB(13, 21, 32);
    } else {
        txtColor = (index == 2) ? COLOR_DANGER : RGB(136, 148, 164);
    }
    drawCenteredText(left, top, left + width, top + height, text,
        (int)(15 * ms), txtColor);
}

static void drawSettingsRow(int row, bool selected, const TCHAR *label, const TCHAR *value)
{
    int left = gWindowWidth * 12 / 100;
    if (left < 120) left = 120;
    if (left > 260) left = 260;
    int top = 200 + row * 62;
    int width = gWindowWidth - left * 2;
    int height = 48;

    setfillcolor(selected ? COLOR_ACCENT : COLOR_MENU_CARD);
    solidrectangle(left + 5, top, left + width - 5, top + height);
    solidrectangle(left, top + 5, left + width, top + height - 5);
    solidcircle(left + 5, top + 5, 5);
    solidcircle(left + width - 5, top + 5, 5);
    solidcircle(left + 5, top + height - 5, 5);
    solidcircle(left + width - 5, top + height - 5, 5);
    if (selected) {
        setlinecolor(COLOR_ACCENT);
        rectangle(left + 3, top + 3, left + width - 3, top + height - 3);
    }
    drawTextAt(left + 22, top + (height - 22)/2, label, 20, selected ? COLOR_ACCENT : COLOR_TEXT);
    drawTextAt(left + width - (int)(gWindowWidth * 0.18f), top + (height - 22)/2, value, 20, COLOR_SCORE);
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

    /* 箭身：亮橙红 + 白色边框，与道具颜色形成反差 */
    setlinecolor(RGB(255, 255, 255));
    setfillcolor(RGB(255, 80, 40));
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
    /* 箭头白色描边 */
    setlinecolor(RGB(255, 255, 255));
    circle(tipX, tipY, wing / 2);
    /* 尾部拖尾 */
    setfillcolor(RGB(255, 180, 80));
    solidcircle(tailX, tailY, cellSize / 8);
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

    setlinecolor(COLOR_GRID);
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
    setbkcolor(COLOR_BG);
    Render_particlesInit();
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
    setbkcolor(COLOR_BG);
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
    static const TCHAR *MAIN_OPTIONS[] = {
        _T("单人模式"),
        _T("AI 对战模式"),
        _T("限时挑战模式"),
        _T("本地多人模式")
    };
    static const TCHAR *SUB_OPTIONS[] = {
        _T("更换时装"), _T("设置"), _T("退出游戏")
    };
    int i;

    updateMenuBgSnakes(16);
    cleardevice();
    drawMenuBackground();

    /* 标题 */
    drawCenteredText(0, 70, gWindowWidth, 140, _T("贪吃蛇"), 48, COLOR_TEXT);
    drawCenteredText(0, 120, gWindowWidth, 158,
        _T("BUPT  EASYX  EDITION"), 13, COLOR_ACCENT);

    /* 装饰线 */
    {
        int lineW = 50;
        int lineX = (gWindowWidth - lineW) / 2;
        setlinecolor(COLOR_ACCENT);
        line(lineX, 168, lineX + lineW, 168);
    }

    /* 主按钮 */
    for (i = 0; i < 4; i++) {
        drawMenuButton(i, selected, MAIN_OPTIONS[i]);
    }

    /* 底部次要按钮 */
    for (i = 0; i < 3; i++) {
        bool sel = (i + 4 == selected);
        drawSmallButton(i, sel, SUB_OPTIONS[i], 3);
    }

    /* 飘浮粒子 */
    if (Game_isUsingParticles()) Render_particlesDraw();

    FlushBatchDraw();
}

void Render_drawVariantMenu(MapVariant selected)
{
    updateMenuBgSnakes(16);
    cleardevice();
    drawMenuBackground();
    drawCenteredText(0, 145, gWindowWidth, 200, _T("选择玩法规则"), 38, COLOR_TEXT);
    drawSmallButton(0, selected == VARIANT_CLASSIC, _T("常规模式"), 2);
    drawSmallButton(1, selected == VARIANT_DIVERSE, _T("多样模式"), 2);
    FlushBatchDraw();
}

void Render_drawDifficultyMenu(AiDifficulty selected)
{
    updateMenuBgSnakes(16);
    cleardevice();
    drawMenuBackground();
    drawCenteredText(0, 145, gWindowWidth, 200, _T("选择 AI 难度"), 38, COLOR_TEXT);
    drawSmallButton(0, selected == AI_EASY, _T("低"), 3);
    drawSmallButton(1, selected == AI_MEDIUM, _T("中"), 3);
    drawSmallButton(2, selected == AI_HARD, _T("高"), 3);
    FlushBatchDraw();
}

void Render_drawSkinMenu(int selectedSkin)
{
    int i;

    updateMenuBgSnakes(16);
    cleardevice();
    drawMenuBackground();
    drawCenteredText(0, 110, gWindowWidth, 165, _T("更换时装"), 40, COLOR_TEXT);
    for (i = 0; i < Render_skinCount(); i++) {
        drawMenuButton(i, selectedSkin, Render_skinDisplayName(i));
    }
    FlushBatchDraw();
}

void Render_drawSettings(const GameConfig *config, int selectedRow)
{
    TCHAR value[64];

    updateMenuBgSnakes(16);
    cleardevice();
    drawMenuBackground();
    drawCenteredText(0, 74, gWindowWidth, 128, _T("设置"), 40, COLOR_TEXT);
    drawCenteredText(0, 136, gWindowWidth, 168,
        _T("W/S 选择，A/D 修改，Enter 返回"), 18, COLOR_TEXT_DIM);

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
    int cellSize = Render_cellSizeForMap(render, mapSize);
    int visibleCells = visibleCellsForMap(render, mapSize, cellSize);
    int boardSize = boardSizeForMap(render, mapSize);
    Pos focus = state->player.length > 0 ? state->player.body[0] : state->ai.body[0];
    int startRow = clampViewportStart(focus.row, visibleCells, mapSize);
    int startCol = clampViewportStart(focus.col, visibleCells, mapSize);
    int row;
    int col;
    int x = panelLeft(render, mapSize);
    TCHAR buffer[128];
    static int gLastScore = -1;
    static int gScoreBounceMs = 0;
    static int gWarnPulseTimer = 0;
    static int gDeathFlashMs = 0;

    /* Score bounce + particle spawn on score change */
    if (state->player.score != gLastScore && gLastScore >= 0) {
        gScoreBounceMs = 300;
        if (state->player.length > 0) {
            Pos head = state->player.body[0];
            int sx = BOARD_LEFT + (head.col - startCol) * cellSize + cellSize / 2;
            int sy = BOARD_TOP + (head.row - startRow) * cellSize + cellSize / 2;
            Render_spawnParticles(sx, sy, 6, COLOR_SCORE, 600);
        }
    }
    gLastScore = state->player.score;

    ensureGameTextureSize(render, cellSize);
    cleardevice();
    setfillcolor(COLOR_BG);
    solidrectangle(0, 0, render->windowWidth, render->windowHeight);

    drawBoardBackground(render, mapSize);

    /* 轰炸区地面染色：每个区画一个整矩形，O(1) 而非逐格绘制 */
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
        setfillcolor(zoneColor);
        setlinecolor(zoneColor);
        for (z = 0; z < state->event.zoneCount; z++) {
            int vrStart = state->event.zones[z].rowStart;
            int vrEnd = state->event.zones[z].rowEnd;
            int vcStart = state->event.zones[z].colStart;
            int vcEnd = state->event.zones[z].colEnd;

            /* 裁剪到视口 */
            if (vrStart < startRow) vrStart = startRow;
            if (vrEnd >= startRow + visibleCells) vrEnd = startRow + visibleCells - 1;
            if (vcStart < startCol) vcStart = startCol;
            if (vcEnd >= startCol + visibleCells) vcEnd = startCol + visibleCells - 1;
            if (vrStart > vrEnd || vcStart > vcEnd) continue;

            {
                int bx = BOARD_LEFT + (vcStart - startCol) * cellSize;
                int by = BOARD_TOP + (vrStart - startRow) * cellSize;
                int bw = (vcEnd - vcStart + 1) * cellSize;
                int bh = (vrEnd - vrStart + 1) * cellSize;
                solidrectangle(bx + 1, by + 1, bx + bw - 1, by + bh - 1);
                setlinecolor(state->event.bombActive ? COLOR_DANGER : COLOR_WARNING);
                rectangle(bx + 1, by + 1, bx + bw - 1, by + bh - 1);
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

    /* ═══════════════════════════════════════════════════════════════
     * Board HUD overlay
     * ═══════════════════════════════════════════════════════════════ */

    /* ── HUD 缩放（基于棋盘大小） ── */
    float hudScale = boardSize / 640.0f;
    if (hudScale < 0.8f) hudScale = 0.8f;
    if (hudScale > 1.6f) hudScale = 1.6f;

    /* ── HUD: 左上角 模式+速度档 ── */
    {
        int hudX = BOARD_LEFT + (int)(6 * hudScale);
        int hudY = BOARD_TOP + (int)(6 * hudScale);
        TCHAR hudBuf[64];

        if (state->config.mode == MODE_LOCAL_MULTIPLAYER) {
            _stprintf_s(hudBuf, 64, _T("%s"), modeText(state->config.mode));
        } else {
            _stprintf_s(hudBuf, 64, _T("%s  %+d"),
                modeText(state->config.mode), state->speedLevel);
        }

        int hudFont = (int)(18 * hudScale);
        setFont(hudFont);
        {
            int txtW = textwidth(hudBuf) + (int)(24 * hudScale);
            int txtH = (int)(30 * hudScale);
            setfillcolor(RGB(10, 15, 22));
            solidrectangle(hudX, hudY, hudX + txtW, hudY + txtH);
            setfillcolor(COLOR_ACCENT);
            solidrectangle(hudX, hudY + txtH - (int)(3 * hudScale), hudX + txtW, hudY + txtH);

            drawTextAt(hudX + (int)(10 * hudScale), hudY + (int)(4 * hudScale),
                hudBuf, hudFont, COLOR_ACCENT);
        }
    }

    /* ── HUD: 右上角 剩余时间 ── */
    if (state->config.mode == MODE_TIME_CHALLENGE
        || state->config.mode == MODE_LOCAL_MULTIPLAYER) {
        TCHAR timeBuf[32];
        COLORREF timeColor = (state->remainingSeconds <= 10 && state->remainingSeconds > 0)
            ? COLOR_DANGER : COLOR_TEXT;

        _stprintf_s(timeBuf, 32, _T("%ds"), state->remainingSeconds);
        int timeFont = (int)(20 * hudScale);
        setFont(timeFont);
        {
            int tW = textwidth(timeBuf) + (int)(28 * hudScale);
            int timeX = BOARD_LEFT + boardSize - tW - (int)(6 * hudScale);
            int timeY = BOARD_TOP + (int)(6 * hudScale);
            int tH = (int)(30 * hudScale);

            setfillcolor(RGB(10, 15, 22));
            solidrectangle(timeX, timeY, timeX + tW, timeY + tH);
            setfillcolor(timeColor);
            solidrectangle(timeX, timeY + tH - (int)(3 * hudScale), timeX + tW, timeY + tH);

            drawTextAt(timeX + (int)(12 * hudScale), timeY + (int)(4 * hudScale),
                timeBuf, timeFont, timeColor);
        }
    }

    /* ── HUD: 底部道具状态栏 ── */
    {
        int barH = (int)(28 * hudScale);
        int barW = (int)(220 * hudScale);
        int barX = BOARD_LEFT + (boardSize - barW) / 2;
        int barY = BOARD_TOP + boardSize - barH - (int)(4 * hudScale);
        int segW = barW / 3;
        int barFont = (int)(14 * hudScale);

        setfillcolor(RGB(10, 15, 22));
        solidrectangle(barX, barY, barX + barW, barY + barH);

        TCHAR tag[16];
        int pad = (int)(10 * hudScale);
        _stprintf_s(tag, 16, _T("蛇 %d"), state->player.length);
        drawTextAt(barX + pad, barY + (barH - barFont)/2, tag, barFont, COLOR_TEXT);
        _stprintf_s(tag, 16, _T("弓 %d"), state->player.bowArrows);
        drawTextAt(barX + segW + pad, barY + (barH - barFont)/2, tag, barFont, COLOR_SCORE);
        _stprintf_s(tag, 16, _T("盾 %d"), state->player.shieldCharges);
        drawTextAt(barX + segW*2 + pad, barY + (barH - barFont)/2, tag, barFont, COLOR_POSITIVE);
    }

    /* Death flash overlay */
    if (state->result == RESULT_RUNNING) {
        gDeathFlashMs = 0;  /* 新游戏开始时重置 */
    } else if (gDeathFlashMs == 0) {
        gDeathFlashMs = 900;
    }
    if (gDeathFlashMs > 0) {
        gDeathFlashMs -= 16;
        if (gDeathFlashMs < 0) gDeathFlashMs = 0;
        if ((gDeathFlashMs / 300) % 2 == 0) {
            setfillcolor(COLOR_DANGER);
            solidrectangle(BOARD_LEFT, BOARD_TOP, BOARD_LEFT + boardSize, BOARD_TOP + boardSize);
        }
    }

    setfillcolor(COLOR_PANEL);
    solidrectangle(x, BOARD_TOP, render->windowWidth - 24, BOARD_TOP + boardSize);

    /* 文字缩放因子：基于面板实际宽度 */
    float txtScale = (float)(render->windowWidth - 24 - x) / 240.0f;
    if (txtScale < 0.75f) txtScale = 0.75f;
    if (txtScale > 1.5f) txtScale = 1.5f;

    /* ──────────── 卡片化侧栏 ──────────── */
    {
        int cardX = x + 6;
        int cardW = render->windowWidth - 24 - cardX - 6;
        int cardY = BOARD_TOP + 12;
        enum { CG = 8 }; /* card gap */

        /* === 模式卡片 === */
        {
            const int ch = (int)(56 * txtScale);
            /* 卡片边框 — 蓝色描边 */
            setlinecolor(COLOR_ACCENT);
            setfillcolor(COLOR_CARD);
            solidrectangle(cardX + 4, cardY, cardX + cardW - 4, cardY + ch);
            solidrectangle(cardX, cardY + 4, cardX + cardW, cardY + ch - 4);
            solidcircle(cardX + 4, cardY + 4, 4);
            solidcircle(cardX + cardW - 4, cardY + 4, 4);
            solidcircle(cardX + 4, cardY + ch - 4, 4);
            solidcircle(cardX + cardW - 4, cardY + ch - 4, 4);
            /* 蓝色左边条 */
            setfillcolor(COLOR_ACCENT);
            solidrectangle(cardX, cardY, cardX + 3, cardY + ch);
            drawTextAt(cardX + 14, cardY + 8, modeText(state->config.mode),
                (int)(18 * txtScale), COLOR_ACCENT);
            _stprintf_s(buffer, 128, _T("%s  %dx%d"),
                variantText(state->config.variant), mapSize, mapSize);
            drawTextAt(cardX + 14, cardY + (int)(32 * txtScale), buffer,
                (int)(13 * txtScale), COLOR_TEXT_DIM);
            cardY += ch + CG;
        }

        /* === 事件卡片（有事件时） === */
        if (state->event.activeEvent != EVENT_NONE) {
            const int ch = (int)(54 * txtScale);
            const TCHAR *eventName = (state->event.activeEvent == EVENT_BOMBARDMENT)
                ? _T("地图轰炸") : _T("刀光箭影");
            int remainSec = (state->event.eventTimerMs + 999) / 1000;
            if (remainSec < 0) remainSec = 0;

            /* 红色圆角边框 */
            setlinecolor(COLOR_DANGER);
            setfillcolor(COLOR_CARD);
            solidrectangle(cardX + 4, cardY, cardX + cardW - 4, cardY + ch);
            solidrectangle(cardX, cardY + 4, cardX + cardW, cardY + ch - 4);
            solidcircle(cardX + 4, cardY + 4, 4);
            solidcircle(cardX + cardW - 4, cardY + 4, 4);
            solidcircle(cardX + 4, cardY + ch - 4, 4);
            solidcircle(cardX + cardW - 4, cardY + ch - 4, 4);

            _stprintf_s(buffer, 128, _T("%s    %ds"), eventName, remainSec);
            drawTextAt(cardX + (int)(16 * txtScale), cardY + (int)(6 * txtScale),
                buffer, (int)(14 * txtScale), COLOR_WARNING);

            /* 进度条 */
            {
                int barX = cardX + (int)(16 * txtScale);
                int barY = cardY + (int)(26 * txtScale);
                int barW = cardW - (int)(32 * txtScale);
                int barH = 8;
                int elapsed = 12000 - state->event.eventTimerMs;
                if (elapsed < 0) elapsed = 0;
                if (elapsed > 12000) elapsed = 12000;
                int filled = barW * elapsed / 12000;
                COLORREF barColor = (state->event.eventTimerMs < 3000)
                    ? COLOR_DANGER : COLOR_ACCENT;

                setfillcolor(COLOR_BG);
                solidrectangle(barX, barY, barX + barW, barY + barH);
                setfillcolor(barColor);
                solidrectangle(barX, barY, barX + filled, barY + barH);
            }

            /* 轰炸预警 */
            if (state->event.bombWarning) {
                gWarnPulseTimer += 16;
                int warnSize = ((gWarnPulseTimer / 500) % 2 == 0)
                    ? (int)(14 * txtScale) : (int)(16 * txtScale);
                drawTextAt(cardX + (int)(16 * txtScale), cardY + (int)(38 * txtScale),
                    _T("!!!  即将轰炸  !!!"), warnSize, COLOR_DANGER);
            }

            cardY += ch + CG;
        }

        if (state->config.mode == MODE_LOCAL_MULTIPLAYER) {
            /* === P1 卡片 === */
            {
                const int ch = (int)(76 * txtScale);
                drawCardWithShadow(cardX, cardY, cardX + cardW, cardY + ch, 6, COLOR_CARD);
                /* 蓝色上边框 */
                setfillcolor(COLOR_ACCENT);
                solidrectangle(cardX, cardY, cardX + cardW, cardY + 3);

                drawTextAt(cardX + (int)(16 * txtScale), cardY + (int)(8 * txtScale),
                    _T("玩家一  (WASD)"), (int)(15 * txtScale), COLOR_ACCENT);
                _stprintf_s(buffer, 128, _T("得分: %d"), state->player.score);
                drawTextAt(cardX + (int)(16 * txtScale), cardY + (int)(30 * txtScale),
                    buffer, (int)(17 * txtScale), COLOR_SCORE);
                {
                    int tY = cardY + (int)(52 * txtScale);
                    drawTag(cardX + (int)(16 * txtScale), tY,
                        _T("长度"), state->player.length, COLOR_TEXT, txtScale);
                    drawTag(cardX + (int)(78 * txtScale), tY,
                        _T("弓箭"), state->player.bowArrows, COLOR_SCORE, txtScale);
                    drawTag(cardX + (int)(140 * txtScale), tY,
                        _T("护盾"), state->player.shieldCharges, COLOR_POSITIVE, txtScale);
                }
                cardY += ch + CG;
            }

            /* === P2 卡片 === */
            {
                const int ch = (int)(76 * txtScale);
                drawCardWithShadow(cardX, cardY, cardX + cardW, cardY + ch, 6, COLOR_CARD);
                /* 绿色上边框 */
                setfillcolor(COLOR_POSITIVE);
                solidrectangle(cardX, cardY, cardX + cardW, cardY + 3);

                drawTextAt(cardX + (int)(16 * txtScale), cardY + (int)(8 * txtScale),
                    _T("玩家二  (方向键)"), (int)(15 * txtScale), COLOR_POSITIVE);
                _stprintf_s(buffer, 128, _T("得分: %d"), state->ai.score);
                drawTextAt(cardX + (int)(16 * txtScale), cardY + (int)(30 * txtScale),
                    buffer, (int)(17 * txtScale), COLOR_POSITIVE);
                {
                    int tY = cardY + (int)(52 * txtScale);
                    drawTag(cardX + (int)(16 * txtScale), tY,
                        _T("长度"), state->ai.length, COLOR_TEXT, txtScale);
                    drawTag(cardX + (int)(78 * txtScale), tY,
                        _T("弓箭"), state->ai.bowArrows, COLOR_SCORE, txtScale);
                    drawTag(cardX + (int)(140 * txtScale), tY,
                        _T("护盾"), state->ai.shieldCharges, COLOR_POSITIVE, txtScale);
                }
                cardY += ch + CG;
            }

            /* === 时间卡片 === */
            {
                const int ch = (int)(36 * txtScale);
                drawCardWithShadow(cardX, cardY, cardX + cardW, cardY + ch, 6, COLOR_CARD);
                _stprintf_s(buffer, 128, _T("剩余时间: %d s"), state->remainingSeconds);
                drawTextAt(cardX + (int)(16 * txtScale), cardY + (int)(8 * txtScale),
                    buffer, (int)(17 * txtScale), COLOR_ACCENT);
                cardY += ch + CG;
            }

            /* === 底部卡片（锚定底边） === */
            {
                const int ch = (int)(52 * txtScale);
                int bY = BOARD_TOP + boardSize - ch - 12;
                drawCardWithShadow(cardX, bY, cardX + cardW, bY + ch, 6, COLOR_CARD);
                drawTextAt(cardX + (int)(16 * txtScale), bY + (int)(6 * txtScale),
                    render->skinName, (int)(13 * txtScale), COLOR_TEXT_DIM);
                drawTextAt(cardX + (int)(16 * txtScale), bY + (int)(26 * txtScale),
                    _T("P1: E 射箭    P2: / 射箭"), (int)(13 * txtScale), COLOR_TEXT_DIM);
            }
        } else {
            /* === 分数卡片 === */
            {
                const int ch = (int)(80 * txtScale);
                drawCardWithShadow(cardX, cardY, cardX + cardW, cardY + ch, 6, COLOR_CARD);

                drawCenteredText(cardX, cardY + (int)(4 * txtScale),
                    cardX + cardW, cardY + (int)(20 * txtScale),
                    _T("玩家得分"), (int)(11 * txtScale), COLOR_TEXT_DIM);

                _stprintf_s(buffer, 128, _T("%d"), state->player.score);
                drawCenteredTextGlow(cardX, cardY + (int)(22 * txtScale),
                    cardX + cardW, cardY + (int)(54 * txtScale),
                    buffer, (gScoreBounceMs > 0) ? (int)(30 * txtScale) : (int)(28 * txtScale),
                    COLOR_SCORE, RGB(60, 50, 0));

                /* 分隔线 */
                {
                    int lineY = cardY + (int)(46 * txtScale);
                    int lineW = (int)(cardW * 0.6f);
                    int lineX = cardX + (cardW - lineW) / 2;
                    setlinecolor(COLOR_BORDER);
                    line(lineX, lineY, lineX + lineW, lineY);
                }

                {
                    int tY = cardY + (int)(56 * txtScale);
                    int tagW = (int)(54 * txtScale);
                    int tagArea = cardW - (int)(32 * txtScale);
                    int gap = (tagArea - tagW * 3) / 2;
                    if (gap < 4) gap = 4;
                    drawTag(cardX + (int)(16 * txtScale), tY,
                        _T("长度"), state->player.length, COLOR_TEXT, txtScale);
                    drawTag(cardX + (int)(16 * txtScale) + tagW + gap, tY,
                        _T("弓箭"), state->player.bowArrows, COLOR_SCORE, txtScale);
                    drawTag(cardX + (int)(16 * txtScale) + (tagW + gap) * 2, tY,
                        _T("护盾"), state->player.shieldCharges, COLOR_POSITIVE, txtScale);
                }
                cardY += ch + CG;
            }

            /* === 速度档卡片 === */
            {
                const int ch = (int)(36 * txtScale);
                COLORREF spdColor = (state->speedLevel > 0) ? COLOR_POSITIVE
                    : (state->speedLevel < 0) ? COLOR_DANGER : COLOR_SCORE;

                drawCardWithShadow(cardX, cardY, cardX + cardW, cardY + ch, 6, COLOR_CARD);
                drawTextAt(cardX + (int)(14 * txtScale), cardY + (int)(8 * txtScale),
                    _T("速度档"), (int)(15 * txtScale), COLOR_TEXT);

                _stprintf_s(buffer, 128, _T("%+d"), state->speedLevel);
                setFont((int)(15 * txtScale));
                {
                    int valW = textwidth(buffer);
                    drawTextAt(cardX + cardW - valW - (int)(30 * txtScale),
                        cardY + (int)(8 * txtScale), buffer, (int)(15 * txtScale), spdColor);
                }

                /* 速度指示方块 */
                setfillcolor(spdColor);
                solidrectangle(cardX + cardW - (int)(24 * txtScale), cardY + (int)(10 * txtScale),
                    cardX + cardW - (int)(16 * txtScale), cardY + (int)(18 * txtScale));
                cardY += ch + CG;
            }

            /* === AI 卡片（AI对战模式） === */
            if (state->config.mode == MODE_AI_BATTLE) {
                const int ch = (int)(50 * txtScale);
                drawCardWithShadow(cardX, cardY, cardX + cardW, cardY + ch, 6, COLOR_CARD);
                setfillcolor(COLOR_DANGER);
                solidrectangle(cardX, cardY, cardX + 3, cardY + ch);

                _stprintf_s(buffer, 128, _T("AI 得分: %d"), state->ai.score);
                drawTextAt(cardX + (int)(16 * txtScale), cardY + (int)(8 * txtScale),
                    buffer, (int)(15 * txtScale), COLOR_DANGER);
                _stprintf_s(buffer, 128, _T("弓箭 %d    护盾 %d    %s"),
                    state->ai.bowArrows, state->ai.shieldCharges,
                    difficultyText(state->config.aiDifficulty));
                drawTextAt(cardX + (int)(16 * txtScale), cardY + (int)(28 * txtScale),
                    buffer, (int)(11 * txtScale), COLOR_TEXT_DIM);
                cardY += ch + CG;
            }

            /* === 时间卡片（限时挑战模式） === */
            if (state->config.mode == MODE_TIME_CHALLENGE) {
                const int ch = (int)(36 * txtScale);
                drawCardWithShadow(cardX, cardY, cardX + cardW, cardY + ch, 6, COLOR_CARD);
                _stprintf_s(buffer, 128, _T("剩余时间: %d s"), state->remainingSeconds);
                drawTextAt(cardX + (int)(16 * txtScale), cardY + (int)(8 * txtScale),
                    buffer, (int)(17 * txtScale), COLOR_ACCENT);
                cardY += ch + CG;
            }

            /* === 底部卡片（锚定底边） === */
            {
                const int ch = (int)(52 * txtScale);
                int bY = BOARD_TOP + boardSize - ch - 12;
                drawCardWithShadow(cardX, bY, cardX + cardW, bY + ch, 6, COLOR_CARD);
                drawTextAt(cardX + (int)(16 * txtScale), bY + (int)(6 * txtScale),
                    render->skinName, (int)(13 * txtScale), COLOR_TEXT_DIM);

                drawTextAt(cardX + (int)(16 * txtScale), bY + (int)(26 * txtScale),
                    _T("E 射箭    1 加速    2 减速"), (int)(13 * txtScale), COLOR_TEXT_DIM);
            }
        }
    }

    if (waitingForStart) {
        const TCHAR *text;
        if (state->config.mode == MODE_LOCAL_MULTIPLAYER) {
            text = _T("双方按各自操控方式开始");
        } else if (state->config.p1ControlMethod == CONTROL_MOUSE) {
            text = _T("点击鼠标左键开始");
        } else if (state->config.p1ControlMethod >= CONTROL_GAMEPAD_1) {
            text = _T("按手柄 RB 或移动摇杆开始");
        } else {
            text = _T("按方向键开始");
        }
        drawTextAt(BOARD_LEFT + 8, BOARD_TOP + 8, text, 18, COLOR_SCORE);
    }

    if (paused) {
        int left = BOARD_LEFT + boardSize / 2 - 190;
        int top = BOARD_TOP + boardSize / 2 - 52;
        int right = left + 380;
        int bottom = top + 104;

        setfillcolor(COLOR_BG);
        solidrectangle(left, top, right, bottom);
        setlinecolor(COLOR_SCORE);
        rectangle(left, top, right, bottom);
        drawCenteredText(left, top, right, bottom, _T("暂停"), 32, COLOR_SCORE);
    }

    /* Score bounce timer decrement */
    if (gScoreBounceMs > 0) {
        gScoreBounceMs -= 16;
        if (gScoreBounceMs < 0) gScoreBounceMs = 0;
    }

    /* Particle overlay */
    if (Game_isUsingParticles()) {
        Render_particlesDraw();
    }

    FlushBatchDraw();
}

void Render_drawGameOver(const GameState *state, int selectedAction)
{
    TCHAR score[128];
    const TCHAR *title;

    cleardevice();
    setfillcolor(COLOR_BG);
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

    drawCenteredText(0, 130, gWindowWidth, 190, title, 44, COLOR_TEXT);
    _stprintf_s(score, 128, _T("玩家一：%d"), state->player.score);
    drawCenteredText(0, 215, gWindowWidth, 260, score, 28, COLOR_SCORE);
    if (state->config.mode == MODE_AI_BATTLE || state->config.mode == MODE_LOCAL_MULTIPLAYER) {
        const TCHAR *p2Label = (state->config.mode == MODE_LOCAL_MULTIPLAYER)
            ? _T("玩家二") : _T("AI");
        _stprintf_s(score, 128, _T("%s：%d"), p2Label, state->ai.score);
        drawCenteredText(0, 260, gWindowWidth, 305, score, 24,
            state->config.mode == MODE_LOCAL_MULTIPLAYER ? COLOR_POSITIVE : COLOR_DANGER);
    }
    drawSmallButton(0, selectedAction == 0, _T("重新开始"), 2);
    drawSmallButton(1, selectedAction == 1, _T("返回菜单"), 2);
    FlushBatchDraw();
}

void Render_drawControlSelect(RenderContext *render, int p1Sel, int p2Sel, const GameConfig *config)
{
    static const TCHAR *CONTROL_NAMES[] = {
        _T("键盘 WASD"),
        _T("键盘 方向键"),
        _T("鼠标"),
        _T("手柄 1"),
        _T("手柄 2")
    };
    int i;
    int leftColX = gWindowWidth / 2 - 210 - 16;
    int rightColX = gWindowWidth / 2 + 16;

    (void)render;
    (void)config;

    cleardevice();
    setfillcolor(COLOR_BG);
    solidrectangle(0, 0, gWindowWidth, gWindowHeight);
    drawCenteredText(0, 48, gWindowWidth, 100, _T("选择操控方式"), 38, COLOR_TEXT);
    drawCenteredText(0, 110, gWindowWidth, 140,
        _T("W/S 切换 P1，方向键 切换 P2，Enter 确认"), 18, COLOR_TEXT_DIM);

    /* P1 column */
    drawCenteredText(leftColX, 150, leftColX + 210, 180, _T("玩家一"), 24, COLOR_ACCENT);
    for (i = 0; i < 5; i++) {
        int x = leftColX;
        int y = 195 + i * 52;
        int w = 210;
        int h = 42;
        bool selected = (i == p1Sel);

        drawRoundedRect(x, y, x + w, y + h, 8,
            selected ? COLOR_CARD_HOVER : COLOR_CARD,
            selected ? COLOR_ACCENT : COLOR_BORDER);
        if (selected) {
            setfillcolor(COLOR_ACCENT);
            solidrectangle(x, y, x + 3, y + h);
        }
        drawCenteredText(x, y, x + w, y + h, CONTROL_NAMES[i], 20,
            selected ? COLOR_ACCENT : COLOR_TEXT);
    }

    /* P2 column */
    drawCenteredText(rightColX, 150, rightColX + 210, 180, _T("玩家二"), 24, COLOR_POSITIVE);
    for (i = 0; i < 5; i++) {
        int x = rightColX;
        int y = 195 + i * 52;
        int w = 210;
        int h = 42;
        bool selected = (i == p2Sel);

        drawRoundedRect(x, y, x + w, y + h, 8,
            selected ? COLOR_CARD_HOVER : COLOR_CARD,
            selected ? COLOR_ACCENT : COLOR_BORDER);
        if (selected) {
            setfillcolor(COLOR_POSITIVE);
            solidrectangle(x, y, x + 3, y + h);
        }
        drawCenteredText(x, y, x + w, y + h, CONTROL_NAMES[i], 20,
            selected ? COLOR_POSITIVE : COLOR_TEXT);
    }

    FlushBatchDraw();
}

void Render_drawControlSelectSingle(RenderContext *render, int selected)
{
    static const TCHAR *CONTROL_NAMES[] = {
        _T("键盘 WASD"),
        _T("键盘 方向键"),
        _T("鼠标"),
        _T("手柄 1"),
        _T("手柄 2")
    };
    int i;
    int colX = (gWindowWidth - 210) / 2;

    (void)render;

    cleardevice();
    setfillcolor(COLOR_BG);
    solidrectangle(0, 0, gWindowWidth, gWindowHeight);
    drawCenteredText(0, 48, gWindowWidth, 100, _T("选择操控方式"), 38, COLOR_TEXT);
    drawCenteredText(0, 110, gWindowWidth, 140,
        _T("W/S/方向键 选择，Enter 确认，Esc 返回"), 18, COLOR_TEXT_DIM);

    for (i = 0; i < 5; i++) {
        int x = colX;
        int y = 195 + i * 52;
        int w = 210;
        int h = 42;
        bool sel = (i == selected);

        drawRoundedRect(x, y, x + w, y + h, 8,
            sel ? COLOR_CARD_HOVER : COLOR_CARD,
            sel ? COLOR_ACCENT : COLOR_BORDER);
        if (sel) {
            setfillcolor(COLOR_ACCENT);
            solidrectangle(x, y, x + 3, y + h);
        }
        drawCenteredText(x, y, x + w, y + h, CONTROL_NAMES[i], 20,
            sel ? COLOR_ACCENT : COLOR_TEXT);
    }

    FlushBatchDraw();
}

/* ================================================================
 * Particle system
 * ================================================================ */

static Particle gParticles[MAX_PARTICLES];

void Render_particlesInit(void)
{
    memset(gParticles, 0, sizeof(gParticles));
}

void Render_particlesUpdate(int deltaMs)
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
        if (p->lifeMs < 500 && p->radius > 1) {
            p->radius = (p->radius * p->lifeMs) / 500;
            if (p->radius < 1) p->radius = 1;
        }
    }
}

void Render_particlesDraw(void)
{
    int i;
    for (i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &gParticles[i];
        if (!p->active) continue;
        setfillcolor(p->color);
        solidcircle((int)p->x, (int)p->y, p->radius);
    }
}

void Render_spawnParticles(int screenX, int screenY, int count,
    COLORREF color, int lifeMs)
{
    int i;
    for (i = 0; i < MAX_PARTICLES && count > 0; i++) {
        if (!gParticles[i].active) {
            gParticles[i].active = true;
            gParticles[i].x = (float)screenX;
            gParticles[i].y = (float)screenY;
            gParticles[i].vx = (float)((rand() % 160) - 80);
            gParticles[i].vy = (float)((rand() % 160) - 80);
            gParticles[i].lifeMs = lifeMs + rand() % 300;
            gParticles[i].color = color;
            gParticles[i].radius = 2 + rand() % 4;
            count--;
        }
    }
}
