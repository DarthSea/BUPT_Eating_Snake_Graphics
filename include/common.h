#ifndef BUPT_SNAKE_COMMON_H
#define BUPT_SNAKE_COMMON_H

#include <windows.h>
#include <stdbool.h>

#define DEFAULT_MAP_SIZE 20
#define MAX_MAP_SIZE 100
#define MAX_SNAKE_LEN (MAX_MAP_SIZE * MAX_MAP_SIZE)

#define TEXTURE_SIZE 32
#define BOARD_PIXEL_SIZE 700
#define BOARD_LEFT 24
#define BOARD_TOP 56
#define SIDE_PANEL_WIDTH 210

/* UI 配色系统 — 清爽现代暗色 */
#define COLOR_BG         RGB(26, 31, 43)
#define COLOR_CARD       RGB(38, 45, 58)
#define COLOR_CARD_HOVER RGB(48, 56, 70)
#define COLOR_BOARD      RGB(42, 48, 64)
#define COLOR_WALL_COLOR RGB(58, 64, 80)
#define COLOR_ACCENT     RGB(88, 166, 255)
#define COLOR_POSITIVE   RGB(124, 255, 107)
#define COLOR_DANGER     RGB(255, 64, 64)
#define COLOR_WARNING    RGB(255, 200, 55)
#define COLOR_TEXT       RGB(232, 236, 240)
#define COLOR_TEXT_DIM   RGB(106, 116, 132)
#define COLOR_SCORE      RGB(255, 202, 99)
#define COLOR_BORDER     RGB(64, 76, 90)
#define COLOR_PANEL      RGB(22, 27, 38)
#define COLOR_SHADOW     RGB(12, 15, 22)
#define COLOR_GRID       RGB(50, 58, 72)

/* 菜单专用配色 — 更亮的深蓝科技风 */
#define COLOR_MENU_BG1   RGB(22, 32, 48)
#define COLOR_MENU_BG2   RGB(30, 46, 72)
#define COLOR_MENU_CARD   RGB(42, 52, 70)
#define COLOR_MENU_GRAD   RGB(30, 58, 95)
#define COLOR_MENU_EXIT   RGB(55, 22, 22)
#define COLOR_MENU_NUM    RGB(58, 68, 86)

#define WINDOW_WIDTH (BOARD_LEFT * 2 + BOARD_PIXEL_SIZE + SIDE_PANEL_WIDTH)
#define WINDOW_HEIGHT (BOARD_TOP + BOARD_PIXEL_SIZE + 24)

#define PLAYER_INDEX 0
#define AI_INDEX 1
#define P2_INDEX 1   /* 本地多人模式玩家二复用 ai 蛇槽位，与 AI_INDEX 指向同一蛇 */

#define DEFAULT_GROWTH_INTERVAL 32
#define CLASSIC_OBSTACLE_COUNT 10
#define DIVERSE_OBSTACLE_COUNT 16
#define TIME_LIMIT_SECONDS 90
#define MAX_SOUND_EVENTS 8
#define MAX_ACTIVE_ARROWS 64  /* 覆盖 100x100 地图箭雨全部 50 个边界源 */
#define ARROW_INTERVAL_MS 35
#define MAX_BORDER_SOURCES (MAX_MAP_SIZE * 2)

#define SPEED_LEVEL_MIN (-2)
#define SPEED_LEVEL_MAX 2

typedef enum Direction {
    DIR_UP = 0,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT,
    DIR_NONE
} Direction;

typedef enum GameMode {
    MODE_SINGLE = 0,
    MODE_AI_BATTLE,
    MODE_TIME_CHALLENGE,
    MODE_LOCAL_MULTIPLAYER
} GameMode;

typedef enum MapVariant {
    VARIANT_CLASSIC = 0,
    VARIANT_DIVERSE
} MapVariant;

typedef enum AiDifficulty {
    AI_EASY = 0,
    AI_MEDIUM,
    AI_HARD
} AiDifficulty;

typedef enum ControlMethod {
    CONTROL_KEYBOARD_WASD = 0,
    CONTROL_KEYBOARD_ARROWS,
    CONTROL_MOUSE,
    CONTROL_GAMEPAD_1,
    CONTROL_GAMEPAD_2
} ControlMethod;

typedef enum DisplayResolution {
    RESOLUTION_HD = 0,
    RESOLUTION_HD_PLUS,
    RESOLUTION_FHD,
    RESOLUTION_QHD
} DisplayResolution;

typedef enum CellType {
    CELL_EMPTY = 0,
    CELL_WALL,
    CELL_OBSTACLE,
    CELL_FOOD,
    CELL_FOOD_BONUS,
    CELL_FOOD_SPEED,
    CELL_FOOD_SLOW,
    CELL_TRAP,
    CELL_SHIELD,
    CELL_BATTLE_BOW,
    CELL_BATTLE_SHIELD,
    CELL_BATTLE_SPIKE,
    CELL_BATTLE_CLOCK
} CellType;

typedef enum GameResult {
    RESULT_RUNNING = 0,
    RESULT_PLAYER_WIN,
    RESULT_AI_WIN,
    RESULT_DRAW,
    RESULT_TIME_UP,
    RESULT_PLAYER_DEAD,
    RESULT_P1_WIN,
    RESULT_P2_WIN
} GameResult;

typedef enum SoundEvent {
    SOUND_NONE = 0,
    SOUND_START,
    SOUND_EAT_NORMAL,
    SOUND_EAT_BONUS,
    SOUND_EAT_SPEED,
    SOUND_EAT_SLOW,
    SOUND_SHIELD,
    SOUND_TRAP,
    SOUND_COLLISION,
    SOUND_BOW,
    SOUND_ARROW_HIT
} SoundEvent;

/* 2D 整数坐标，用于格子定位和搜索 */
typedef struct Pos {
    int row;
    int col;
} Pos;

/* 蛇的数据：身体数组、生命、得分、道具/战斗资源 */
typedef struct Snake {
    Pos body[MAX_SNAKE_LEN];
    int length;
    Direction dir;         /* 当前移动方向 */
    Direction nextDir;     /* 下一帧输入方向，用于缓冲掉头检测 */
    int score;
    int stepCount;         /* 自上次增长后的步数，用于 interval 增长 */
    bool alive;

    /* 限时效果计时器，普通模式和战斗 clock 共用 */
    int shieldMs;
    int speedMs;
    int slowMs;
    int dirChangeCooldownMs; /* 减速时防止快速来回转向 */
    int slowStepCounter;     /* 减速时每两帧才走一步 */

    /* AI 对战专属资源 */
    int bowArrows;
    int shieldCharges;
} Snake;

/* 飞行中的箭矢：战斗模式下蛇射出或箭雨事件生成 */
typedef struct ArrowProjectile {
    bool active;
    Pos pos;
    Direction dir;
    int ownerIndex;    /* PLAYER_INDEX/AI_INDEX，-1 表示箭雨无主箭 */
    int moveTimerMs;   /* 累积毫秒，达 ARROW_INTERVAL_MS 时前进一格 */
} ArrowProjectile;

typedef enum RandomEventType {
    EVENT_NONE = 0,
    EVENT_BOMBARDMENT,
    EVENT_ARROW_STORM
} RandomEventType;

/* 轰炸区矩形范围，用于炸弹事件的地面标记 */
typedef struct BombZone {
    int rowStart, rowEnd;
    int colStart, colEnd;
} BombZone;

/* 箭雨事件的边界发射源：位置和射入方向 */
typedef struct BorderSource {
    Pos pos;
    Direction dir;
} BorderSource;

/* 随机事件（轰炸/箭雨）的全状态管理 */
typedef struct RandomEventState {
    RandomEventType activeEvent;
    int eventTimerMs;       /* 事件总剩余时长 */
    int phaseTimerMs;       /* 阶段计时器 */
    int sinceLastEventMs;   /* 距上次事件的间隔，用于触发新事件 */
    int bombPhase;          /* 轰炸轮次 */
    bool bombActive;        /* 炸弹当前是否处于引爆状态 */
    int bombFlashMs;        /* 爆炸闪烁剩余时间 */
    bool bombWarning;       /* 是否为轰炸预警阶段（红色闪烁提示） */
    int bombWarningMs;      /* 预警闪烁计时 */
    BombZone zones[3];      /* 最多 3 个轰炸区 */
    int zoneCount;
    BorderSource borderSources[MAX_BORDER_SOURCES];
    int borderSourceCount;
    bool borderFlashing;    /* 边界闪烁指示 */
    int borderFlashMs;      /* 边界闪烁剩余毫秒 */
} RandomEventState;

#define MAX_PARTICLES 200

/* 粒子特效：用于得分弹出、碰撞火花等视觉效果 */
typedef struct Particle {
    bool active;
    float x, y;       /* 屏幕坐标 */
    float vx, vy;     /* 速度（像素/秒） */
    int lifeMs;       /* 剩余生命，到期自动灭活 */
    COLORREF color;
    int radius;
} Particle;

/* 游戏全局配置：模式、难度、分辨率、音频/控制等所有设置 */
typedef struct GameConfig {
    GameMode mode;
    MapVariant variant;
    AiDifficulty aiDifficulty;
    DisplayResolution resolution;
    bool fullscreen;
    int skinId;

    /* 运行时地图尺寸，有效值 20/50/100 */
    int mapSize;
    int moveIntervalMs;       /* 基础移动间隔毫秒 */
    int growthInterval;       /* N 步自动增长间隔 */
    int timeLimitSeconds;     /* 限时模式剩余秒数 */

    /* false 时蛇只靠吃食物增长，不自动增长 */
    bool enableStepGrowth;
    bool musicEnabled;
    bool soundEnabled;
    ControlMethod p1ControlMethod;
    ControlMethod p2ControlMethod;
} GameConfig;

/* 游戏运行时状态：包含地图、蛇、道具、事件等所有动态数据 */
typedef struct GameState {
    /* cells 存储地形/食物/道具，蛇身坐标在 Snake.body 中独立维护 */
    CellType cells[MAX_MAP_SIZE][MAX_MAP_SIZE];
    Snake player;
    Snake ai;           /* AI 或本地 P2 共用此槽位 */
    ArrowProjectile arrows[MAX_ACTIVE_ARROWS];
    GameConfig config;
    GameResult result;

    /* 速度档 -2..2，越大移动越快、食物奖励越高 */
    int speedLevel;
    int moveTimerMs;       /* 移动间隔累积计时器 */
    int elapsedMs;         /* 限时模式已用时间 */
    int remainingSeconds;  /* 限时模式剩余秒数 */
    unsigned int randomSeed;

    /* 逻辑层注册音效事件，UI 层消费并播放 */
    SoundEvent soundEvents[MAX_SOUND_EVENTS];
    int soundEventCount;
    char statusText[128];      /* 状态栏文字 */
    RandomEventState event;    /* 随机事件状态 */
    Particle particles[MAX_PARTICLES];
} GameState;

const char *Common_modeName(GameMode mode);
const char *Common_variantName(MapVariant variant);
const char *Common_aiDifficultyName(AiDifficulty difficulty);
const char *Common_resultName(GameResult result);
bool Common_isOpposite(Direction a, Direction b);
Pos Common_nextPos(Pos pos, Direction dir);
int Common_manhattan(Pos a, Pos b);

#endif
