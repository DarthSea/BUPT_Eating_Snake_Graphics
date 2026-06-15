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
#define SIDE_PANEL_WIDTH 280

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
#define COLOR_PANEL      RGB(38, 45, 58)
#define COLOR_GRID       RGB(50, 58, 72)

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

typedef struct Pos {
    int row;
    int col;
} Pos;

typedef struct Snake {
    Pos body[MAX_SNAKE_LEN];
    int length;
    Direction dir;
    Direction nextDir;
    int score;
    int stepCount;
    bool alive;

    /* Timed effects used by normal modes and battle clock effects. */
    int shieldMs;
    int speedMs;
    int slowMs;
    int dirChangeCooldownMs;
    int slowStepCounter;

    /* AI battle exclusive resources. */
    int bowArrows;
    int shieldCharges;
} Snake;

typedef struct ArrowProjectile {
    bool active;
    Pos pos;
    Direction dir;
    int ownerIndex;
    int moveTimerMs;
} ArrowProjectile;

typedef enum RandomEventType {
    EVENT_NONE = 0,
    EVENT_BOMBARDMENT,
    EVENT_ARROW_STORM
} RandomEventType;

typedef struct BombZone {
    int rowStart, rowEnd;
    int colStart, colEnd;
} BombZone;

typedef struct BorderSource {
    Pos pos;
    Direction dir;
} BorderSource;

typedef struct RandomEventState {
    RandomEventType activeEvent;
    int eventTimerMs;
    int phaseTimerMs;
    int sinceLastEventMs;
    int bombPhase;
    bool bombActive;
    int bombFlashMs;
    bool bombWarning;
    int bombWarningMs;
    BombZone zones[3];
    int zoneCount;
    BorderSource borderSources[MAX_BORDER_SOURCES];
    int borderSourceCount;
    bool borderFlashing;
    int borderFlashMs;
} RandomEventState;

#define MAX_PARTICLES 200

typedef struct Particle {
    bool active;
    float x, y;
    float vx, vy;
    int lifeMs;
    COLORREF color;
    int radius;
} Particle;

typedef struct GameConfig {
    GameMode mode;
    MapVariant variant;
    AiDifficulty aiDifficulty;
    DisplayResolution resolution;
    bool fullscreen;
    int skinId;

    /* Runtime map size. Valid values are 20, 50, and 100. */
    int mapSize;
    int moveIntervalMs;
    int growthInterval;
    int timeLimitSeconds;

    /* If false, snakes grow only after eating food. */
    bool enableStepGrowth;
    bool musicEnabled;
    bool soundEnabled;
} GameConfig;

typedef struct GameState {
    /* cells stores terrain, food, and items. Snake bodies live in Snake.body. */
    CellType cells[MAX_MAP_SIZE][MAX_MAP_SIZE];
    Snake player;
    Snake ai;
    ArrowProjectile arrows[MAX_ACTIVE_ARROWS];
    GameConfig config;
    GameResult result;

    /* -2..2. Larger means faster movement and higher food reward. */
    int speedLevel;
    int moveTimerMs;
    int elapsedMs;
    int remainingSeconds;
    unsigned int randomSeed;

    /* Rules register sound events; UI consumes them and plays audio. */
    SoundEvent soundEvents[MAX_SOUND_EVENTS];
    int soundEventCount;
    char statusText[128];
    RandomEventState event;
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
