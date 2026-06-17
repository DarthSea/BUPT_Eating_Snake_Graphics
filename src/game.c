#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include "ai.h"
#include "game.h"

typedef struct StepPlan {
    Pos next;
    Direction dir;
    CellType item;
    bool dead;
    bool stays;
    bool grow;
    bool itemGrow;
    bool intervalGrow;
    bool shieldUsed;
} StepPlan;

static int gPerfAccumMs = 0;
static int gPerfFrameCount = 0;
static bool gUseParticles = true;

/* 判断两坐标是否相同 */
static bool posEquals(Pos a, Pos b)
{
    return a.row == b.row && a.col == b.col;
}

/* 检查地形是否阻挡箭矢飞行 */
static bool terrainBlocksArrow(CellType cell)
{
    return cell == CELL_WALL || cell == CELL_OBSTACLE;
}

/* LCG 伪随机数生成器：每次调用推进一个状态 */
static unsigned int nextRandom(GameState *state)
{
    state->randomSeed = state->randomSeed * 1103515245u + 12345u;
    return state->randomSeed;
}

/* 生成 [0, limit) 范围内的伪随机整数 */
static int randomRange(GameState *state, int limit)
{
    if (limit <= 0) {
        return 0;
    }

    /* 取高位：LCG 低位周期性差，右移 16 位后取模分布更均匀 */
    return (int)((nextRandom(state) >> 16) % (unsigned int)limit);
}

/* 获取标准化的地图尺寸 */
static int mapSizeOf(const GameState *state)
{
    return Game_validMapSize(state->config.mapSize);
}

/* 计算当前地图下蛇的最大长度 = 地图面积 */
static int maxSnakeLengthOf(const GameState *state)
{
    int mapSize = mapSizeOf(state);

    return mapSize * mapSize;
}

/* 按地图面积比例缩放数量，保证至少不低于 baseCount */
static int scaledCountForMap(const GameState *state, int baseCount)
{
    int mapSize = mapSizeOf(state);
    int scaled = baseCount * mapSize * mapSize / (DEFAULT_MAP_SIZE * DEFAULT_MAP_SIZE);

    return scaled < baseCount ? baseCount : scaled;
}

/* 大地图如果只放 1 个道具会过空，所以按 20/50/100 三档提高道具数量。 */
static int diverseItemTarget(const GameState *state, int baseCount)
{
    int mapSize = mapSizeOf(state);

    if (mapSize >= 100) {
        return baseCount * 4;
    }
    if (mapSize >= 50) {
        return baseCount * 2;
    }

    return baseCount;
}

/* 计算普通食物应在地图上的目标数量 */
static int normalFoodTarget(const GameState *state)
{
    int mapSize = mapSizeOf(state);

    if (mapSize >= 100) {
        return 8;
    }
    if (mapSize >= 50) {
        return 4;
    }

    return 1;
}

/* 计算战斗道具在地图上的目标数量（按地图比例系数缩放） */
static int battleItemTarget(const GameState *state, int baseCount)
{
    int mapSize = mapSizeOf(state);

    if (mapSize >= 100) {
        return baseCount * 5;
    }
    if (mapSize >= 50) {
        return baseCount * 3;
    }

    return baseCount;
}

/* 设置游戏状态文本（用于 HUD 和日志） */
static void setStatus(GameState *state, const char *text)
{
    snprintf(state->statusText, sizeof(state->statusText), "%s", text);
}

/* 初始化地图：边界设为墙，内部清空 */
static void clearCells(GameState *state)
{
    int mapSize = mapSizeOf(state);
    int row;
    int col;

    for (row = 0; row < mapSize; row++) {
        for (col = 0; col < mapSize; col++) {
            if (row == 0 || row == mapSize - 1 || col == 0 || col == mapSize - 1) {
                state->cells[row][col] = CELL_WALL;
            } else {
                state->cells[row][col] = CELL_EMPTY;
            }
        }
    }
}

/* 初始化蛇：长度为 3，头部在 head，身体向 dir 反方向延伸 */
static void initSnake(Snake *snake, Pos head, Direction dir)
{
    int i;

    memset(snake, 0, sizeof(*snake));
    snake->length = 3;
    snake->dir = dir;
    snake->nextDir = dir;
    snake->alive = true;

    snake->body[0] = head;
    for (i = 1; i < snake->length; i++) {
        Pos back = head;

        if (dir == DIR_UP) {
            back.row += i;
        } else if (dir == DIR_DOWN) {
            back.row -= i;
        } else if (dir == DIR_LEFT) {
            back.col += i;
        } else if (dir == DIR_RIGHT) {
            back.col -= i;
        }

        snake->body[i] = back;
    }
}

/* 检查某位置是否可以生成道具：在界内、为空地、无蛇占据 */
static bool isSpawnFree(const GameState *state, Pos pos)
{
    if (!Game_isInside(state, pos)) {
        return false;
    }
    if (state->cells[pos.row][pos.col] != CELL_EMPTY) {
        return false;
    }
    if (Game_cellHasSnake(state, pos, -1, false)) {
        return false;
    }

    return true;
}

/* 在地图上找一个空闲位置放置指定道具（两阶段搜索） */
static bool spawnSpecificItem(GameState *state, CellType item)
{
    int mapSize = mapSizeOf(state);
    int tries;
    int maxTries = mapSize * mapSize * 4;

    /* 第一阶段：随机尝试 */
    for (tries = 0; tries < maxTries; tries++) {
        Pos pos;

        pos.row = 1 + randomRange(state, mapSize - 2);
        pos.col = 1 + randomRange(state, mapSize - 2);
        if (isSpawnFree(state, pos)) {
            state->cells[pos.row][pos.col] = item;
            return true;
        }
    }

    /* 第二阶段：全图扫描，但起点随机化避免道具扎堆在左上角 */
    {
        int startRow = 1 + randomRange(state, mapSize - 2);
        int startCol = 1 + randomRange(state, mapSize - 2);
        int r, c;
        int row, col;

        for (r = 0; r < mapSize - 2; r++) {
            row = 1 + (startRow - 1 + r) % (mapSize - 2);
            for (c = 0; c < mapSize - 2; c++) {
                col = 1 + (startCol - 1 + c) % (mapSize - 2);
                Pos pos;
                pos.row = row;
                pos.col = col;
                if (isSpawnFree(state, pos)) {
                    state->cells[row][col] = item;
                    return true;
                }
            }
        }
    }

    return false;
}

/* 统计地图上指定类型道具的数量 */
static int countCellType(const GameState *state, CellType item)
{
    int mapSize = mapSizeOf(state);
    int row;
    int col;
    int count = 0;

    for (row = 0; row < mapSize; row++) {
        for (col = 0; col < mapSize; col++) {
            if (state->cells[row][col] == item) {
                count++;
            }
        }
    }

    return count;
}

/* 补充普通模式道具：食物、增益道具、陷阱，低于目标则生成 */
static void maintainNormalItems(GameState *state)
{
    int foodTarget = normalFoodTarget(state);
    int utilityTarget = diverseItemTarget(state, 1);
    int trapTarget = diverseItemTarget(state, 4);

    while (countCellType(state, CELL_FOOD) < foodTarget) {
        if (!spawnSpecificItem(state, CELL_FOOD)) {
            break;
        }
    }

    if (state->config.variant != VARIANT_DIVERSE) {
        return;
    }

    while (countCellType(state, CELL_FOOD_BONUS) < utilityTarget) {
        if (!spawnSpecificItem(state, CELL_FOOD_BONUS)) {
            break;
        }
    }
    while (countCellType(state, CELL_FOOD_SPEED) < utilityTarget) {
        if (!spawnSpecificItem(state, CELL_FOOD_SPEED)) {
            break;
        }
    }
    while (countCellType(state, CELL_FOOD_SLOW) < utilityTarget) {
        if (!spawnSpecificItem(state, CELL_FOOD_SLOW)) {
            break;
        }
    }
    while (countCellType(state, CELL_SHIELD) < utilityTarget) {
        if (!spawnSpecificItem(state, CELL_SHIELD)) {
            break;
        }
    }
    while (countCellType(state, CELL_TRAP) < trapTarget) {
        if (!spawnSpecificItem(state, CELL_TRAP)) {
            break;
        }
    }
}

/* 补充战斗模式道具：食物、弓、盾、尖刺、时钟，低于目标则生成 */
static void maintainBattleItems(GameState *state)
{
    int foodTarget = normalFoodTarget(state);
    int bowTarget = battleItemTarget(state, 1);
    int shieldTarget = battleItemTarget(state, 1);
    int spikeTarget = battleItemTarget(state, 4);
    int clockTarget = battleItemTarget(state, 1);

    while (countCellType(state, CELL_FOOD) < foodTarget) {
        if (!spawnSpecificItem(state, CELL_FOOD)) {
            break;
        }
    }
    while (countCellType(state, CELL_BATTLE_BOW) < bowTarget) {
        if (!spawnSpecificItem(state, CELL_BATTLE_BOW)) {
            break;
        }
    }
    while (countCellType(state, CELL_BATTLE_SHIELD) < shieldTarget) {
        if (!spawnSpecificItem(state, CELL_BATTLE_SHIELD)) {
            break;
        }
    }
    while (countCellType(state, CELL_BATTLE_SPIKE) < spikeTarget) {
        if (!spawnSpecificItem(state, CELL_BATTLE_SPIKE)) {
            break;
        }
    }
    while (countCellType(state, CELL_BATTLE_CLOCK) < clockTarget) {
        if (!spawnSpecificItem(state, CELL_BATTLE_CLOCK)) {
            break;
        }
    }
}

/* 道具维护调度：根据模式选择普通或战斗道具补充 */
static void maintainItems(GameState *state)
{
    if (state->config.mode == MODE_AI_BATTLE || state->config.mode == MODE_LOCAL_MULTIPLAYER) {
        maintainBattleItems(state);
    } else {
        maintainNormalItems(state);
    }
}

/* 统计 pos 周围 3x3 区域内障碍物数量 */
static int countObstaclesNear(const GameState *state, Pos center)
{
    int count = 0;
    int dr, dc;
    int mapSize = Game_validMapSize(state->config.mapSize);

    for (dr = -1; dr <= 1; dr++) {
        for (dc = -1; dc <= 1; dc++) {
            Pos p;
            p.row = center.row + dr;
            p.col = center.col + dc;
            if (Game_isInsideMap(p, mapSize)
                && state->cells[p.row][p.col] == CELL_OBSTACLE) {
                count++;
            }
        }
    }
    return count;
}

/* 检查在 pos 放障碍物后，所有蛇出生点 3x3 区域障碍物是否仍 ≤2 */
static bool obstacleNearSpawnOk(const GameState *state, Pos pos)
{
    int mapSize = Game_validMapSize(state->config.mapSize);
    Pos spawns[2];
    int spawnCount = 0;
    int i;

    /* P1 出生点 */
    if (state->config.mode == MODE_AI_BATTLE
        || state->config.mode == MODE_LOCAL_MULTIPLAYER) {
        spawns[0].row = mapSize / 2;
        spawns[0].col = mapSize / 4;
        spawns[1].row = mapSize / 2;
        spawns[1].col = mapSize - mapSize / 4 - 1;
        spawnCount = 2;
    } else {
        spawns[0].row = mapSize / 2;
        spawns[0].col = mapSize / 2;
        spawnCount = 1;
    }

    for (i = 0; i < spawnCount; i++) {
        int cur = countObstaclesNear(state, spawns[i]);
        /* 如果 pos 正好在出生点的 3x3 范围内，则 count 会 +1 */
        {
            int dr = pos.row - spawns[i].row;
            int dc = pos.col - spawns[i].col;
            if (dr >= -1 && dr <= 1 && dc >= -1 && dc <= 1) {
                cur++;
            }
        }
        if (cur > 2) {
            return false;
        }
    }
    return true;
}

/* 在地图上放置障碍物：随机尝试 + 兜底扫描，避开出生点周围 */
static void placeObstacles(GameState *state)
{
    int mapSize = mapSizeOf(state);
    int placed = 0;
    int tries = 0;
    int maxRandomTries = mapSize * mapSize * 4;
    int row;
    int col;
    int baseTarget = state->config.variant == VARIANT_DIVERSE
        ? DIVERSE_OBSTACLE_COUNT
        : CLASSIC_OBSTACLE_COUNT;
    int target = scaledCountForMap(state, baseTarget);

    while (placed < target && tries < maxRandomTries) {
        Pos pos;

        tries++;
        pos.row = 1 + randomRange(state, mapSize - 2);
        pos.col = 1 + randomRange(state, mapSize - 2);
        if (isSpawnFree(state, pos) && obstacleNearSpawnOk(state, pos)) {
            state->cells[pos.row][pos.col] = CELL_OBSTACLE;
            placed++;
        }
    }

    /* 兜底扫描：起点随机化，避免障碍物全挤在第二行 */
    {
        int startRow = 1 + randomRange(state, mapSize - 2);
        int startCol = 1 + randomRange(state, mapSize - 2);
        int r, c;

        for (r = 0; r < mapSize - 2 && placed < target; r++) {
            row = 1 + (startRow - 1 + r) % (mapSize - 2);
            for (c = 0; c < mapSize - 2 && placed < target; c++) {
                col = 1 + (startCol - 1 + c) % (mapSize - 2);
                Pos pos;

                pos.row = row;
                pos.col = col;
                if (isSpawnFree(state, pos) && obstacleNearSpawnOk(state, pos)) {
                    state->cells[row][col] = CELL_OBSTACLE;
                    placed++;
                }
            }
        }
    }
}

/* 统计前 upTo 个轰炸区的总格子数 */
static int totalPlacedSoFar(const GameState *state, int upTo)
{
    int i, total = 0;
    for (i = 0; i < upTo; i++) {
        int rows = state->event.zones[i].rowEnd - state->event.zones[i].rowStart + 1;
        int cols = state->event.zones[i].colEnd - state->event.zones[i].colStart + 1;
        if (rows > 0 && cols > 0) total += rows * cols;
    }
    return total;
}

/* 计算实际移动间隔：考虑速度档、加速/减速道具的综合影响 */
static int effectiveMoveInterval(const GameState *state)
{
    int interval = state->config.moveIntervalMs;
    int speedLevel = state->speedLevel;

    if (state->config.mode != MODE_AI_BATTLE && state->config.mode != MODE_LOCAL_MULTIPLAYER) {
        if (state->player.speedMs > 0) {
            interval = interval * 70 / 100;
        }
        if (state->player.slowMs > 0) {
            interval = interval * 130 / 100;
        }
    }

    if (state->config.mode != MODE_LOCAL_MULTIPLAYER) {
        if (speedLevel > 0) {
            interval = interval * (100 - speedLevel * 18) / 100;
        } else if (speedLevel < 0) {
            interval = interval * (100 + (-speedLevel) * 22) / 100;
        }
    }

    if (interval < 55) {
        interval = 55;
    }

    return interval;
}

/* 减少蛇身上所有限时效果的剩余毫秒 */
static void reduceEffectTimers(Snake *snake, int deltaMs)
{
    if (snake->shieldMs > 0) {
        snake->shieldMs -= deltaMs;
        if (snake->shieldMs < 0) {
            snake->shieldMs = 0;
        }
    }
    if (snake->speedMs > 0) {
        snake->speedMs -= deltaMs;
        if (snake->speedMs < 0) {
            snake->speedMs = 0;
        }
    }
    if (snake->slowMs > 0) {
        snake->slowMs -= deltaMs;
        if (snake->slowMs < 0) {
            snake->slowMs = 0;
            snake->slowStepCounter = 0;
        }
    }
    if (snake->dirChangeCooldownMs > 0) {
        snake->dirChangeCooldownMs -= deltaMs;
        if (snake->dirChangeCooldownMs < 0) {
            snake->dirChangeCooldownMs = 0;
        }
    }
}

/* 设置蛇的方向：阻止反向并处理减速冷却限制 */
static void setSnakeDirectionWithSlow(Snake *snake, Direction dir)
{
    if (dir == DIR_NONE) {
        return;
    }
    if (Common_isOpposite(dir, snake->dir)) {
        return;
    }
    if (snake->slowMs > 0 && dir != snake->nextDir && snake->dirChangeCooldownMs > 0) {
        return;
    }

    snake->nextDir = dir;
    if (snake->slowMs > 0 && dir != snake->dir) {
        snake->dirChangeCooldownMs = 350;
    }
}

/* 减速时每两帧走一步：交替返回 true/false */
static bool snakeMovesThisBattleStep(Snake *snake)
{
    if (snake->slowMs <= 0) {
        return true;
    }

    snake->slowStepCounter = 1 - snake->slowStepCounter;
    return snake->slowStepCounter == 0;
}

/* 构建蛇的单步移动计划：计算下一步位置、碰撞判定、是否增长 */
static StepPlan buildStepPlan(const GameState *state, const Snake *snake)
{
    StepPlan plan;
    bool itemGrow;
    bool intervalGrow;

    memset(&plan, 0, sizeof(plan));
    plan.dir = snake->nextDir;
    if (Common_isOpposite(plan.dir, snake->dir)) {
        plan.dir = snake->dir;
    }
    plan.next = Common_nextPos(snake->body[0], plan.dir);

    if (!Game_isInside(state, plan.next)) {
        plan.dead = true;
        return plan;
    }

    plan.item = state->cells[plan.next.row][plan.next.col];
    if (plan.item == CELL_WALL || plan.item == CELL_OBSTACLE) {
        plan.dead = true;
        return plan;
    }
    if (plan.item == CELL_TRAP) {
        if (snake->shieldMs > 0) {
            plan.shieldUsed = true;
        } else {
            plan.dead = true;
            return plan;
        }
    }
    if (plan.item == CELL_BATTLE_SPIKE) {
        if (snake->shieldCharges > 0) {
            plan.shieldUsed = true;
        } else {
            plan.dead = true;
            return plan;
        }
    }

    /* 轰炸事件：在爆破区中且炸弹活跃时受伤/死亡 */
    if (state->event.activeEvent == EVENT_BOMBARDMENT
        && state->event.bombActive
        && Game_isInBombZone(state, plan.next)) {
        if (snake->shieldCharges > 0 || snake->shieldMs > 0) {
            plan.shieldUsed = true;
        } else {
            plan.dead = true;
            return plan;
        }
    }

    itemGrow = Game_itemGrows(plan.item);
    intervalGrow = state->config.enableStepGrowth
        && state->config.growthInterval > 0
        && snake->stepCount + 1 >= state->config.growthInterval;

    plan.itemGrow = itemGrow;
    plan.intervalGrow = intervalGrow;
    plan.grow = itemGrow || intervalGrow;

    return plan;
}

/* 构建蛇的停顿计划：减速时本帧不行走，头部原地不动 */
static StepPlan buildStayPlan(const Snake *snake)
{
    StepPlan plan;

    memset(&plan, 0, sizeof(plan));
    plan.next = snake->body[0];
    plan.dir = snake->dir;
    plan.item = CELL_EMPTY;
    plan.stays = true;

    return plan;
}

/* 检测某蛇身（考虑尾部离开）是否占据指定格 */
static bool snakeBlocksBattleCell(const Snake *snake, Pos pos, bool tailWillLeave)
{
    int i;
    int limit;

    if (!snake->alive || snake->length <= 0) {
        return false;
    }

    limit = snake->length;
    if (tailWillLeave && limit > 0) {
        limit--;
    }

    for (i = 0; i < limit; i++) {
        if (posEquals(snake->body[i], pos)) {
            return true;
        }
    }

    return false;
}

/* 单人模式碰撞检测：蛇头碰到自己身体即死亡 */
static void addSingleSnakeCollisions(const GameState *state, StepPlan *plan)
{
    if (plan->dead) {
        return;
    }

    if (Game_snakeContains(&state->player, plan->next, !plan->grow)) {
        plan->dead = true;
    }
}

/* 对战模式碰撞检测：双方蛇头对撞、碰到自身或对方身体 */
static void addBattleCollisions(const GameState *state, StepPlan *playerPlan, StepPlan *aiPlan)
{
    bool playerTailLeaves = !playerPlan->stays && !playerPlan->grow && !playerPlan->dead;
    bool aiTailLeaves = !aiPlan->stays && !aiPlan->grow && !aiPlan->dead;

    if (!playerPlan->stays && !aiPlan->stays && !playerPlan->dead && !aiPlan->dead) {
        if (posEquals(playerPlan->next, aiPlan->next)) {
            playerPlan->dead = true;
            aiPlan->dead = true;
            return;
        }
        if (posEquals(playerPlan->next, state->ai.body[0])
            && posEquals(aiPlan->next, state->player.body[0])) {
            playerPlan->dead = true;
            aiPlan->dead = true;
            return;
        }
    }

    if (!playerPlan->stays && !playerPlan->dead) {
        if (Game_snakeContains(&state->player, playerPlan->next, !playerPlan->grow)
            || snakeBlocksBattleCell(&state->ai, playerPlan->next, aiTailLeaves)) {
            playerPlan->dead = true;
        }
    }

    if (!aiPlan->stays && !aiPlan->dead) {
        if (Game_snakeContains(&state->ai, aiPlan->next, !aiPlan->grow)
            || snakeBlocksBattleCell(&state->player, aiPlan->next, playerTailLeaves)) {
            aiPlan->dead = true;
        }
    }
}

/* 将蛇的移动应用到地图：头部插入新位置，若不增长则丢弃尾部 */
static void applySnakeMove(GameState *state, Snake *snake, const StepPlan *plan)
{
    int i;
    int oldLength;
    int newLength;
    int maxLength = maxSnakeLengthOf(state);

    if (plan->dead || !snake->alive) {
        snake->alive = false;
        return;
    }
    if (plan->stays) {
        return;
    }

    oldLength = snake->length;
    newLength = oldLength;
    if (plan->grow && newLength < maxLength) {
        newLength++;
    }

    for (i = newLength - 1; i > 0; i--) {
        if (i - 1 < oldLength) {
            snake->body[i] = snake->body[i - 1];
        }
    }
    snake->body[0] = plan->next;
    snake->length = newLength;
    snake->dir = plan->dir;
    snake->nextDir = plan->dir;

    if (plan->intervalGrow) {
        snake->stepCount = 0;
    } else {
        snake->stepCount++;
    }
}

/* 计算道具的得分值：基础分乘以速度和模式修正系数 */
static int scoreForItem(const GameState *state, CellType item)
{
    int baseScore = Game_itemScore(item);
    int multiplier;
    int score;

    if (!Game_isFood(item)) {
        return baseScore;
    }
    if (state->config.mode == MODE_LOCAL_MULTIPLAYER) {
        multiplier = 100;
    } else {
        multiplier = 100 + state->speedLevel * 25;
        if (multiplier < 40) {
            multiplier = 40;
        }
    }

    score = baseScore * multiplier / 100;
    return score < 1 ? 1 : score;
}

/* 处理蛇吃到道具后的效果：加分、获得状态、触发音效 */
static void applyItemEffect(GameState *state, Snake *snake, Snake *opponent, const StepPlan *plan)
{
    if (plan->dead) {
        return;
    }

    if (plan->shieldUsed) {
        if ((plan->item == CELL_BATTLE_SPIKE
             || state->event.activeEvent == EVENT_BOMBARDMENT)
            && snake->shieldCharges > 0) {
            snake->shieldCharges--;
        } else {
            snake->shieldMs = 0;
        }
        if (plan->item == CELL_BATTLE_SPIKE || plan->item == CELL_TRAP) {
            state->cells[plan->next.row][plan->next.col] = CELL_EMPTY;
        }
        snake->score += 5;
        Game_pushSoundEvent(state, SOUND_TRAP);
        return;
    }

    if (plan->item == CELL_EMPTY || plan->item == CELL_WALL || plan->item == CELL_OBSTACLE) {
        return;
    }

    snake->score += scoreForItem(state, plan->item);

    if (plan->item == CELL_FOOD_SPEED) {
        snake->speedMs = 5000;
        Game_pushSoundEvent(state, SOUND_EAT_SPEED);
    } else if (plan->item == CELL_FOOD_SLOW) {
        if (opponent != NULL && opponent->alive) {
            opponent->slowMs = 4000;
        } else {
            snake->slowMs = 3500;
        }
        Game_pushSoundEvent(state, SOUND_EAT_SLOW);
    } else if (plan->item == CELL_SHIELD) {
        snake->shieldMs = 7000;
        Game_pushSoundEvent(state, SOUND_SHIELD);
    } else if (plan->item == CELL_FOOD_BONUS) {
        Game_pushSoundEvent(state, SOUND_EAT_BONUS);
    } else if (plan->item == CELL_FOOD) {
        Game_pushSoundEvent(state, SOUND_EAT_NORMAL);
    } else if (plan->item == CELL_BATTLE_BOW) {
        snake->bowArrows++;
        Game_pushSoundEvent(state, SOUND_BOW);
    } else if (plan->item == CELL_BATTLE_SHIELD) {
        snake->shieldCharges++;
        Game_pushSoundEvent(state, SOUND_SHIELD);
    } else if (plan->item == CELL_BATTLE_CLOCK) {
        if (opponent != NULL && opponent->alive) {
            opponent->slowMs = 1000;
            opponent->dirChangeCooldownMs = 350;
        }
        Game_pushSoundEvent(state, SOUND_EAT_SLOW);
    }

    state->cells[plan->next.row][plan->next.col] = CELL_EMPTY;
}

/* 检查游戏是否应结束：蛇死亡、时间到等情况 */
static void finishIfNeeded(GameState *state)
{
    if (state->config.mode == MODE_AI_BATTLE || state->config.mode == MODE_LOCAL_MULTIPLAYER) {
        if (!state->player.alive && !state->ai.alive) {
            state->result = RESULT_DRAW;
            setStatus(state, "Draw");
            Game_pushSoundEvent(state, SOUND_COLLISION);
        } else if (!state->player.alive) {
            state->result = (state->config.mode == MODE_LOCAL_MULTIPLAYER) ? RESULT_P2_WIN : RESULT_AI_WIN;
            setStatus(state, state->config.mode == MODE_LOCAL_MULTIPLAYER ? "P2 wins" : "AI wins");
            Game_pushSoundEvent(state, SOUND_COLLISION);
        } else if (!state->ai.alive) {
            state->result = (state->config.mode == MODE_LOCAL_MULTIPLAYER) ? RESULT_P1_WIN : RESULT_PLAYER_WIN;
            setStatus(state, state->config.mode == MODE_LOCAL_MULTIPLAYER ? "P1 wins" : "Player wins");
            Game_pushSoundEvent(state, SOUND_COLLISION);
        }
    } else {
        if (!state->player.alive) {
            state->result = RESULT_PLAYER_DEAD;
            setStatus(state, "Game over");
            Game_pushSoundEvent(state, SOUND_COLLISION);
        }
    }
}

/* 在蛇身上查找指定位置，返回段索引，未找到返回 -1 */
static int findSnakeSegmentAt(const Snake *snake, Pos pos)
{
    int i;

    if (!snake->alive || snake->length <= 0) {
        return -1;
    }

    for (i = 0; i < snake->length; i++) {
        if (posEquals(snake->body[i], pos)) {
            return i;
        }
    }

    return -1;
}

static void applyArrowHit(GameState *state, ArrowProjectile *arrow)
{
    Snake *shooter;
    Snake *target;
    int hitIndex;

    if (!arrow->active) {
        return;
    }
    if (!Game_isInside(state, arrow->pos)
        || terrainBlocksArrow(state->cells[arrow->pos.row][arrow->pos.col])) {
        arrow->active = false;
        return;
    }

    /* 箭雨无主箭矢：碰到任意蛇即造成伤害 */
    if (arrow->ownerIndex < 0) {
        hitIndex = findSnakeSegmentAt(&state->player, arrow->pos);
        if (hitIndex >= 0) {
            arrow->active = false;
            Game_pushSoundEvent(state, SOUND_ARROW_HIT);
            if (hitIndex == 0) {
                state->player.alive = false;
            } else {
                state->player.length = hitIndex;
                if (state->player.length < 1) state->player.alive = false;
            }
            finishIfNeeded(state);
            return;
        }
        hitIndex = findSnakeSegmentAt(&state->ai, arrow->pos);
        if (hitIndex >= 0) {
            arrow->active = false;
            Game_pushSoundEvent(state, SOUND_ARROW_HIT);
            if (hitIndex == 0) {
                state->ai.alive = false;
            } else {
                state->ai.length = hitIndex;
                if (state->ai.length < 1) state->ai.alive = false;
            }
            finishIfNeeded(state);
            return;
        }
        return;
    }

    shooter = Game_getMutableSnake(state, arrow->ownerIndex);
    target = Game_getMutableSnake(state,
        arrow->ownerIndex == PLAYER_INDEX ? AI_INDEX : PLAYER_INDEX);
    hitIndex = findSnakeSegmentAt(target, arrow->pos);
    if (hitIndex < 0) {
        return;
    }

    arrow->active = false;
    if (target->shieldCharges > 0) {
        target->shieldCharges--;
        Game_pushSoundEvent(state, SOUND_SHIELD);
        return;
    }

    Game_pushSoundEvent(state, SOUND_ARROW_HIT);
    if (hitIndex == 0) {
        target->alive = false;
        finishIfNeeded(state);
        return;
    }

    target->length = hitIndex;
    if (target->length < 1) {
        target->alive = false;
        finishIfNeeded(state);
    } else if (shooter->alive) {
        shooter->score += 20;
    }
}

/* 更新所有活跃箭矢：按计时器推进位置并检测碰撞 */
static void updateArrows(GameState *state, int deltaMs)
{
    int i;

    for (i = 0; i < MAX_ACTIVE_ARROWS; i++) {
        ArrowProjectile *arrow = &state->arrows[i];

        if (!arrow->active) {
            continue;
        }

        arrow->moveTimerMs += deltaMs;
        while (arrow->active
            && arrow->moveTimerMs >= ARROW_INTERVAL_MS
            && state->result == RESULT_RUNNING) {
            arrow->moveTimerMs -= ARROW_INTERVAL_MS;
            arrow->pos = Common_nextPos(arrow->pos, arrow->dir);
            applyArrowHit(state, arrow);
        }
    }
}

/* 单人模式单步处理：玩家移动、碰撞、道具效果、维护 */
static void stepSingle(GameState *state)
{
    StepPlan playerPlan = buildStepPlan(state, &state->player);

    addSingleSnakeCollisions(state, &playerPlan);
    applySnakeMove(state, &state->player, &playerPlan);
    applyItemEffect(state, &state->player, NULL, &playerPlan);
    finishIfNeeded(state);
    if (state->result == RESULT_RUNNING) {
        maintainItems(state);
    }
}

/* 战斗模式单步处理：AI决策、双方移动、碰撞检测、物品处理 */
static void stepBattle(GameState *state)
{
    StepPlan playerPlan;
    StepPlan aiPlan;
    Direction aiDir;
    bool playerMoves;
    bool aiMoves;

    if (state->ai.bowArrows > 0) {
        if ((state->config.aiDifficulty == AI_EASY && Game_hasClearShot(state, AI_INDEX, true))
            || (state->config.aiDifficulty == AI_MEDIUM && Game_hasClearShot(state, AI_INDEX, false))
            || (state->config.aiDifficulty == AI_HARD && Game_hasClearShot(state, AI_INDEX, false))) {
            Game_aiFireArrow(state);
        }
    }

    aiDir = Ai_decideDirection(state, state->config.aiDifficulty);
    setSnakeDirectionWithSlow(&state->ai, aiDir);

    playerMoves = snakeMovesThisBattleStep(&state->player);
    aiMoves = snakeMovesThisBattleStep(&state->ai);
    playerPlan = playerMoves ? buildStepPlan(state, &state->player) : buildStayPlan(&state->player);
    aiPlan = aiMoves ? buildStepPlan(state, &state->ai) : buildStayPlan(&state->ai);

    addBattleCollisions(state, &playerPlan, &aiPlan);

    applySnakeMove(state, &state->player, &playerPlan);
    applySnakeMove(state, &state->ai, &aiPlan);
    applyItemEffect(state, &state->player, &state->ai, &playerPlan);
    applyItemEffect(state, &state->ai, &state->player, &aiPlan);

    finishIfNeeded(state);
    if (state->result == RESULT_RUNNING) {
        maintainItems(state);
    }
}

/* 本地多人模式单步处理：双方玩家移动、碰撞检测、物品处理 */
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

/* 创建默认游戏配置：单人经典模式、中等难度、HD+ 分辨率 */
void Game_makeDefaultConfig(GameConfig *config)
{
    memset(config, 0, sizeof(*config));
    config->mode = MODE_SINGLE;
    config->variant = VARIANT_CLASSIC;
    config->aiDifficulty = AI_MEDIUM;
    config->skinId = 0;
    config->resolution = RESOLUTION_HD_PLUS;
    config->fullscreen = false;
    config->mapSize = DEFAULT_MAP_SIZE;
    config->moveIntervalMs = 140;
    config->growthInterval = DEFAULT_GROWTH_INTERVAL;
    config->timeLimitSeconds = TIME_LIMIT_SECONDS;
    config->enableStepGrowth = true;
    config->musicEnabled = true;
    config->soundEnabled = true;
}

/* 根据游戏模式设置默认参数：速度、时间限制等 */
void Game_applyModeDefaults(GameConfig *config, GameMode mode)
{
    config->mode = mode;
    config->mapSize = Game_validMapSize(config->mapSize);
    config->growthInterval = config->enableStepGrowth ? DEFAULT_GROWTH_INTERVAL : 0;

    if (mode == MODE_SINGLE) {
        config->moveIntervalMs = 115;
        config->timeLimitSeconds = 0;
    } else if (mode == MODE_AI_BATTLE) {
        config->moveIntervalMs = 110;
        config->timeLimitSeconds = 0;
    } else if (mode == MODE_TIME_CHALLENGE) {
        config->moveIntervalMs = 115;
        config->timeLimitSeconds = TIME_LIMIT_SECONDS;
    } else if (mode == MODE_LOCAL_MULTIPLAYER) {
        config->moveIntervalMs = 130;
        config->timeLimitSeconds = TIME_LIMIT_SECONDS;
        config->variant = VARIANT_DIVERSE;
        /* 多人模式限制最大 50x50，100x100 屏幕显示不全 */
        if (config->mapSize > 50) config->mapSize = 50;
    }
}

/* 初始化游戏状态：清地图、生成蛇、摆障碍、补道具 */
void Game_init(GameState *state, const GameConfig *config)
{
    unsigned int seed;
    int mapSize;

    memset(state, 0, sizeof(*state));
    state->config = *config;
    state->config.mapSize = Game_validMapSize(state->config.mapSize);
    if (!state->config.enableStepGrowth) {
        state->config.growthInterval = 0;
    }
    mapSize = mapSizeOf(state);
    state->result = RESULT_RUNNING;
    state->speedLevel = 0;
    state->remainingSeconds = config->timeLimitSeconds;
    seed = (unsigned int)time(NULL);
    seed ^= (unsigned int)(uintptr_t)state;
    state->randomSeed = seed == 0u ? 1u : seed;

    clearCells(state);

    if (config->mode == MODE_AI_BATTLE || config->mode == MODE_LOCAL_MULTIPLAYER) {
        Pos playerHead;
        Pos aiHead;

        playerHead.row = mapSize / 2;
        playerHead.col = mapSize / 4;
        aiHead.row = mapSize / 2;
        aiHead.col = mapSize - mapSize / 4 - 1;
        initSnake(&state->player, playerHead, DIR_RIGHT);
        initSnake(&state->ai, aiHead, DIR_LEFT);
    } else {
        Pos playerHead;

        playerHead.row = mapSize / 2;
        playerHead.col = mapSize / 2;
        initSnake(&state->player, playerHead, DIR_RIGHT);
        state->ai.alive = false;
    }

    placeObstacles(state);
    maintainItems(state);
    setStatus(state, "Running");
    memset(&state->event, 0, sizeof(state->event));
}

/* 判断当前模式/变体是否应该触发随机事件 */
static bool shouldTriggerEvent(const GameState *state)
{
    if (state->config.variant != VARIANT_DIVERSE) {
        return false;
    }
    if (state->config.mode != MODE_SINGLE
        && state->config.mode != MODE_AI_BATTLE
        && state->config.mode != MODE_LOCAL_MULTIPLAYER
        && state->config.mode != MODE_TIME_CHALLENGE) {
        return false;
    }
    return true;
}

/* 检查给定位置是否在任意一个轰炸区内 */
static bool isInBombZone(const RandomEventState *event, Pos pos)
{
    int i;
    for (i = 0; i < event->zoneCount; i++) {
        if (pos.row >= event->zones[i].rowStart && pos.row <= event->zones[i].rowEnd
            && pos.col >= event->zones[i].colStart && pos.col <= event->zones[i].colEnd) {
            return true;
        }
    }
    return false;
}

/* 初始化轰炸事件：划分 3 个轰炸区，覆盖地图约 1/4 面积 */
static void initBombardment(GameState *state, int mapSize)
{
    int totalCells = mapSize * mapSize;
    int targetTotal = totalCells / 4;
    int i;

    state->event.zoneCount = 3;
    for (i = 0; i < 3; i++) {
        int remaining = (3 - i);
        int currentTarget;
        int area, halfW, halfH;
        int centerRow, centerCol;
        int maxHalf;

        if (remaining <= 0) remaining = 1;
        currentTarget = (targetTotal - totalPlacedSoFar(state, i)) / remaining;
        area = currentTarget;
        if (area < 1) area = 1;

        maxHalf = mapSize / 2 - 2;
        if (maxHalf < 1) maxHalf = 1;
        halfH = 1 + randomRange(state, maxHalf);
        if (halfH < 1) halfH = 1;
        halfW = area / halfH;
        if (halfW < 1) halfW = 1;
        if (halfW > maxHalf) halfW = maxHalf;

        centerRow = 1 + halfH + randomRange(state, mapSize - 2 - halfH * 2 + 1);
        centerCol = 1 + halfW + randomRange(state, mapSize - 2 - halfW * 2 + 1);

        state->event.zones[i].rowStart = centerRow - halfH;
        state->event.zones[i].rowEnd = centerRow + halfH - 1;
        state->event.zones[i].colStart = centerCol - halfW;
        state->event.zones[i].colEnd = centerCol + halfW - 1;

        if (state->event.zones[i].rowStart < 1) state->event.zones[i].rowStart = 1;
        if (state->event.zones[i].rowEnd >= mapSize - 1) state->event.zones[i].rowEnd = mapSize - 2;
        if (state->event.zones[i].colStart < 1) state->event.zones[i].colStart = 1;
        if (state->event.zones[i].colEnd >= mapSize - 1) state->event.zones[i].colEnd = mapSize - 2;
    }
    state->event.bombPhase = 0;
    state->event.bombActive = false;
    state->event.bombFlashMs = 0;
    state->event.phaseTimerMs = 0;
}

/* 初始化箭雨事件：在四周边界随机生成箭矢发射源 */
static void initArrowStorm(GameState *state, int mapSize)
{
    int target = mapSize / 2;
    int tries = 0;
    int maxTries = mapSize * mapSize * 4;

    state->event.borderSourceCount = 0;
    while (state->event.borderSourceCount < target && tries < maxTries) {
        int side = randomRange(state, 4);
        Pos pos;
        Direction dir;
        bool duplicate = false;
        int j;

        tries++;
        switch (side) {
        case 0:
            pos.row = 0; pos.col = 1 + randomRange(state, mapSize - 2);
            dir = DIR_DOWN;
            break;
        case 1:
            pos.row = mapSize - 1; pos.col = 1 + randomRange(state, mapSize - 2);
            dir = DIR_UP;
            break;
        case 2:
            pos.col = 0; pos.row = 1 + randomRange(state, mapSize - 2);
            dir = DIR_RIGHT;
            break;
        default:
            pos.col = mapSize - 1; pos.row = 1 + randomRange(state, mapSize - 2);
            dir = DIR_LEFT;
            break;
        }

        for (j = 0; j < state->event.borderSourceCount; j++) {
            if (posEquals(state->event.borderSources[j].pos, pos)) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            state->event.borderSources[state->event.borderSourceCount].pos = pos;
            state->event.borderSources[state->event.borderSourceCount].dir = dir;
            state->event.borderSourceCount++;
        }
    }
    state->event.borderFlashing = false;
    state->event.borderFlashMs = 0;
    state->event.phaseTimerMs = 0;
}

static bool spawnBorderArrow(GameState *state, Pos from, Direction dir);

/* 随机触发一个事件：轰炸或箭雨，持续 12 秒 */
static void startRandomEvent(GameState *state)
{
    int mapSize = Game_validMapSize(state->config.mapSize);
    int r = randomRange(state, 2);

    memset(&state->event, 0, sizeof(state->event));

    if (r == 0) {
        state->event.activeEvent = EVENT_BOMBARDMENT;
        initBombardment(state, mapSize);
    } else {
        state->event.activeEvent = EVENT_ARROW_STORM;
        initArrowStorm(state, mapSize);
    }
    state->event.eventTimerMs = 12000;
    state->event.bombWarning = false;
    state->event.bombWarningMs = 0;
    state->event.phaseTimerMs = 0;
    setStatus(state, r == 0 ? "Bombardment!" : "Arrow Storm!");
}

/* 更新随机事件逻辑：计时触发新事件、处理轰炸/箭雨阶段 */
static void updateRandomEvents(GameState *state, int deltaMs)
{
    RandomEventState *ev = &state->event;
    int eventIntervalMs;

    if (!shouldTriggerEvent(state)) {
        return;
    }

    if (state->config.mode == MODE_LOCAL_MULTIPLAYER
        && state->remainingSeconds <= 30 && state->remainingSeconds > 0) {
        eventIntervalMs = 5000;
    } else {
        eventIntervalMs = 10000;
    }

    /* 限时挑战模式：间隔缩短 25% */
    if (state->config.mode == MODE_TIME_CHALLENGE) {
        eventIntervalMs = eventIntervalMs * 75 / 100;
    }

    if (ev->activeEvent == EVENT_NONE) {
        ev->sinceLastEventMs += deltaMs;
        if (ev->sinceLastEventMs >= eventIntervalMs) {
            startRandomEvent(state);
        }
        return;
    }

    ev->eventTimerMs -= deltaMs;
    if (ev->eventTimerMs <= 0) {
        ev->activeEvent = EVENT_NONE;
        ev->sinceLastEventMs = 0;
        ev->bombActive = false;
        ev->bombWarning = false;
        ev->borderFlashing = false;
        /* 限时挑战模式：每渡过一次事件 +100 分 */
        if (state->config.mode == MODE_TIME_CHALLENGE) {
            state->player.score += 100;
        }
        return;
    }

    ev->phaseTimerMs += deltaMs;

    if (ev->activeEvent == EVENT_BOMBARDMENT) {
        /* 1s 预警，然后 3s 时爆破 */
        if (ev->phaseTimerMs >= 3000) {
            ev->phaseTimerMs -= 3000;
            ev->bombPhase++;
            ev->bombActive = true;
            ev->bombFlashMs = 500;
            ev->bombWarning = false;
        } else if (ev->phaseTimerMs >= 2000 && !ev->bombActive) {
            ev->bombWarning = true;
        }
        if (ev->bombActive) {
            ev->bombFlashMs -= deltaMs;
            if (ev->bombFlashMs <= 0) {
                ev->bombActive = false;
            }
        }
    } else if (ev->activeEvent == EVENT_ARROW_STORM) {
        if (ev->phaseTimerMs >= 2000) {
            int k;
            ev->phaseTimerMs -= 2000;
            ev->borderFlashing = true;
            ev->borderFlashMs = 400;
            for (k = 0; k < ev->borderSourceCount; k++) {
                spawnBorderArrow(state, ev->borderSources[k].pos, ev->borderSources[k].dir);
            }
            Game_pushSoundEvent(state, SOUND_BOW);
        }
        if (ev->borderFlashing) {
            ev->borderFlashMs -= deltaMs;
            if (ev->borderFlashMs <= 0) {
                ev->borderFlashing = false;
            }
        }
    }
}

/* 根据玩家选择的初始方向重新排布蛇身并清除旧格子 */
void Game_preparePlayerStart(GameState *state, Direction dir)
{
    Snake *snake = &state->player;
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

/* 根据 P2 选择的初始方向重新排布其蛇身并清除旧格子 */
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

/* 设置玩家蛇方向：仅在游戏进行中有效 */
void Game_setPlayerDirection(GameState *state, Direction dir)
{
    if (dir == DIR_NONE || state->result != RESULT_RUNNING) {
        return;
    }

    setSnakeDirectionWithSlow(&state->player, dir);
}

/* 设置本地多人模式玩家二蛇的方向 */
void Game_setP2Direction(GameState *state, Direction dir)
{
    if (dir == DIR_NONE || state->result != RESULT_RUNNING) {
        return;
    }
    setSnakeDirectionWithSlow(&state->ai, dir);
}

/* 游戏核心更新：计时器管理、事件处理、根据步长推进游戏逻辑 */
void Game_update(GameState *state, int deltaMs)
{
    int interval;
    int steps = 0;

    if (state->result != RESULT_RUNNING) {
        return;
    }

    gPerfAccumMs += deltaMs;
    gPerfFrameCount++;
    if (gPerfAccumMs >= 10000) {
        int avgMs = gPerfAccumMs / gPerfFrameCount;
        gUseParticles = (avgMs <= 40);
        gPerfAccumMs = 0;
        gPerfFrameCount = 0;
    }

    if (deltaMs > 250) {
        deltaMs = 250;
    }

    reduceEffectTimers(&state->player, deltaMs);
    reduceEffectTimers(&state->ai, deltaMs);
    updateArrows(state, deltaMs);
    updateRandomEvents(state, deltaMs);

    if (state->config.mode == MODE_TIME_CHALLENGE || state->config.mode == MODE_LOCAL_MULTIPLAYER) {
        state->elapsedMs += deltaMs;
        state->remainingSeconds = state->config.timeLimitSeconds - state->elapsedMs / 1000;
        if (state->remainingSeconds <= 0) {
            state->remainingSeconds = 0;
            if (state->config.mode == MODE_LOCAL_MULTIPLAYER) {
                if (state->player.score > state->ai.score) {
                    state->result = RESULT_P1_WIN;
                } else if (state->ai.score > state->player.score) {
                    state->result = RESULT_P2_WIN;
                } else {
                    state->result = RESULT_DRAW;
                }
            } else {
                state->result = RESULT_TIME_UP;
            }
            setStatus(state, "Time up");
            return;
        }
    }

    interval = effectiveMoveInterval(state);
    state->moveTimerMs += deltaMs;
    while (state->moveTimerMs >= interval
        && state->result == RESULT_RUNNING
        && steps < 4) {
        state->moveTimerMs -= interval;
        if (state->config.mode == MODE_AI_BATTLE) {
            stepBattle(state);
        } else if (state->config.mode == MODE_LOCAL_MULTIPLAYER) {
            stepLocalMultiplayer(state);
        } else {
            stepSingle(state);
        }
        steps++;
    }
    if (steps >= 4) {
        state->moveTimerMs = 0;
    }
}

/* 判断游戏是否已结束 */
bool Game_isFinished(const GameState *state)
{
    return state->result != RESULT_RUNNING;
}

/* 沿射手方向扫描目标蛇身，返回第一个命中段的索引（-1 表示未命中） */
static int findArrowHitIndex(const GameState *state, int shooterIndex, bool requireHead)
{
    const Snake *shooter = Game_getSnake(state, shooterIndex);
    const Snake *target = Game_getSnake(state, shooterIndex == PLAYER_INDEX ? AI_INDEX : PLAYER_INDEX);
    Pos pos;
    int i;

    if (!shooter->alive || !target->alive || shooter->length <= 0 || target->length <= 0) {
        return -1;
    }

    pos = Common_nextPos(shooter->body[0], shooter->dir);
    while (Game_isInside(state, pos)) {
        CellType cell = state->cells[pos.row][pos.col];

        if (terrainBlocksArrow(cell)) {
            return -1;
        }

        for (i = 0; i < target->length; i++) {
            if (posEquals(target->body[i], pos)) {
                if (requireHead && i != 0) {
                    return -1;
                }
                return i;
            }
        }

        pos = Common_nextPos(pos, shooter->dir);
    }

    return -1;
}

/* 从指定蛇的头部向前发射一支箭矢（消耗弓数量） */
static bool fireArrow(GameState *state, int shooterIndex)
{
    Snake *shooter = Game_getMutableSnake(state, shooterIndex);
    Pos start;
    int i;

    if ((state->config.mode != MODE_AI_BATTLE
         && state->config.mode != MODE_LOCAL_MULTIPLAYER)
        || shooter->bowArrows <= 0 || !shooter->alive) {
        return false;
    }
    if (shooter->dir == DIR_NONE) {
        return false;
    }

    start = Common_nextPos(shooter->body[0], shooter->dir);

    for (i = 0; i < MAX_ACTIVE_ARROWS; i++) {
        ArrowProjectile *arrow = &state->arrows[i];

        if (!arrow->active) {
            shooter->bowArrows--;
            Game_pushSoundEvent(state, SOUND_BOW);
            if (!Game_isInside(state, start)
                || terrainBlocksArrow(state->cells[start.row][start.col])) {
                return true;
            }

            arrow->active = true;
            arrow->ownerIndex = shooterIndex;
            arrow->dir = shooter->dir;
            arrow->pos = start;
            arrow->moveTimerMs = 0;
            return true;
        }
    }

    return false;
}

/* 在边界指定位置生成一支无主箭矢（箭雨事件使用） */
static bool spawnBorderArrow(GameState *state, Pos from, Direction dir)
{
    int i;
    Pos start;

    start = Common_nextPos(from, dir);
    if (!Game_isInside(state, start)) {
        return false;
    }
    if (terrainBlocksArrow(state->cells[start.row][start.col])) {
        return false;
    }

    for (i = 0; i < MAX_ACTIVE_ARROWS; i++) {
        ArrowProjectile *arrow = &state->arrows[i];
        if (!arrow->active) {
            arrow->active = true;
            arrow->ownerIndex = -1;
            arrow->dir = dir;
            arrow->pos = start;
            arrow->moveTimerMs = 0;
            return true;
        }
    }
    return false;
}

/* 调整速度档 (+1 或 -1)，多人模式下禁用 */
void Game_adjustSpeed(GameState *state, int delta)
{
    if (state->config.mode == MODE_LOCAL_MULTIPLAYER) {
        return;
    }
    state->speedLevel += delta;
    if (state->speedLevel > SPEED_LEVEL_MAX) {
        state->speedLevel = SPEED_LEVEL_MAX;
    }
    if (state->speedLevel < SPEED_LEVEL_MIN) {
        state->speedLevel = SPEED_LEVEL_MIN;
    }
}

/* 玩家（P1）发射箭矢 */
bool Game_playerFireArrow(GameState *state)
{
    return fireArrow(state, PLAYER_INDEX);
}

/* AI 发射箭矢 */
bool Game_aiFireArrow(GameState *state)
{
    return fireArrow(state, AI_INDEX);
}

/* 玩家二（P2）发射箭矢 */
bool Game_player2FireArrow(GameState *state)
{
    return fireArrow(state, P2_INDEX);
}

/* 检测射手是否有无障碍的直线射中路径 */
bool Game_hasClearShot(const GameState *state, int shooterIndex, bool requireHead)
{
    return findArrowHitIndex(state, shooterIndex, requireHead) >= 0;
}

/* 检测坐标是否在标准化地图范围内 */
bool Game_isInsideMap(Pos pos, int mapSize)
{
    mapSize = Game_validMapSize(mapSize);

    return pos.row >= 0 && pos.row < mapSize && pos.col >= 0 && pos.col < mapSize;
}

/* 检测坐标是否在当前游戏地图范围内 */
bool Game_isInside(const GameState *state, Pos pos)
{
    return Game_isInsideMap(pos, state->config.mapSize);
}

/* 判断格子类型是否为可食用道具（普通食物或增益食物） */
bool Game_isFood(CellType cell)
{
    return cell == CELL_FOOD
        || cell == CELL_FOOD_BONUS
        || cell == CELL_FOOD_SPEED
        || cell == CELL_FOOD_SLOW;
}

/* 判断格子类型是否为战斗专属道具 */
bool Game_isBattleItem(CellType cell)
{
    return cell == CELL_BATTLE_BOW
        || cell == CELL_BATTLE_SHIELD
        || cell == CELL_BATTLE_SPIKE
        || cell == CELL_BATTLE_CLOCK;
}

/* 判断格子类型是否为危险格（撞上会死亡） */
bool Game_isDangerCell(CellType cell)
{
    return cell == CELL_WALL
        || cell == CELL_OBSTACLE
        || cell == CELL_TRAP
        || cell == CELL_BATTLE_SPIKE;
}

/* 返回道具的基础分数值 */
int Game_itemScore(CellType cell)
{
    switch (cell) {
    case CELL_FOOD:
        return 10;
    case CELL_FOOD_BONUS:
        return 25;
    case CELL_FOOD_SPEED:
        return 15;
    case CELL_FOOD_SLOW:
        return 15;
    case CELL_SHIELD:
        return 0;
    case CELL_BATTLE_BOW:
    case CELL_BATTLE_SHIELD:
    case CELL_BATTLE_SPIKE:
    case CELL_BATTLE_CLOCK:
        return 0;
    default:
        return 0;
    }
}

/* 判断道具是否会导致蛇长度增长 */
bool Game_itemGrows(CellType cell)
{
    return cell == CELL_FOOD
        || cell == CELL_FOOD_BONUS
        || cell == CELL_FOOD_SPEED
        || cell == CELL_FOOD_SLOW;
}

/* 标准化地图大小：仅接受 20/50/100，其他回落到默认 20 */
int Game_validMapSize(int mapSize)
{
    if (mapSize == 50 || mapSize == 100) {
        return mapSize;
    }

    return DEFAULT_MAP_SIZE;
}

/* 根据索引获取蛇的只读指针（AI_INDEX 返回 ai，其他返回 player） */
const Snake *Game_getSnake(const GameState *state, int snakeIndex)
{
    return snakeIndex == AI_INDEX ? &state->ai : &state->player;
}

/* 根据索引获取蛇的可写指针 */
Snake *Game_getMutableSnake(GameState *state, int snakeIndex)
{
    return snakeIndex == AI_INDEX ? &state->ai : &state->player;
}

/* 检查指定位置是否被某蛇身占据（可忽略尾部） */
bool Game_snakeContains(const Snake *snake, Pos pos, bool ignoreTail)
{
    int i;
    int limit;

    if (!snake->alive || snake->length <= 0) {
        return false;
    }

    limit = snake->length;
    if (ignoreTail && limit > 0) {
        limit--;
    }

    for (i = 0; i < limit; i++) {
        if (posEquals(snake->body[i], pos)) {
            return true;
        }
    }

    return false;
}

/* 检查指定位置是否被任一蛇占据（可忽略指定蛇） */
bool Game_cellHasSnake(const GameState *state, Pos pos, int ignoreSnakeIndex, bool ignoreTail)
{
    if (ignoreSnakeIndex != PLAYER_INDEX
        && Game_snakeContains(&state->player, pos, ignoreTail)) {
        return true;
    }
    if (ignoreSnakeIndex != AI_INDEX
        && Game_snakeContains(&state->ai, pos, ignoreTail)) {
        return true;
    }

    return false;
}

/* 统计地图上所有食物（普通+增益）的总数 */
int Game_countFoodCells(const GameState *state)
{
    int mapSize = mapSizeOf(state);
    int row;
    int col;
    int count = 0;

    for (row = 0; row < mapSize; row++) {
        for (col = 0; col < mapSize; col++) {
            if (Game_isFood(state->cells[row][col])) {
                count++;
            }
        }
    }

    return count;
}

/* 将音效事件推入队列，供 UI 层消费播放 */
void Game_pushSoundEvent(GameState *state, SoundEvent event)
{
    if (event == SOUND_NONE || state->soundEventCount >= MAX_SOUND_EVENTS) {
        return;
    }

    state->soundEvents[state->soundEventCount++] = event;
}

/* 消费所有待播放的音效事件，清空队列并返回事件数 */
int Game_consumeSoundEvents(GameState *state, SoundEvent outEvents[], int maxCount)
{
    int i;
    int count = state->soundEventCount;

    if (count > maxCount) {
        count = maxCount;
    }

    for (i = 0; i < count; i++) {
        outEvents[i] = state->soundEvents[i];
    }
    state->soundEventCount = 0;

    return count;
}

/* 检测位置是否处于当前活跃的轰炸区中 */
bool Game_isInBombZone(const GameState *state, Pos pos)
{
    return isInBombZone(&state->event, pos);
}

/* 返回炸弹当前是否处于引爆状态 */
bool Game_isBombActive(const GameState *state)
{
    return state->event.bombActive;
}

/* 返回箭雨边界是否正在闪烁 */
bool Game_isBorderFlashing(const GameState *state)
{
    return state->event.borderFlashing;
}

/* 获取边界发射源数量（箭雨事件） */
int Game_getBorderSourceCount(const GameState *state)
{
    return state->event.borderSourceCount;
}

/* 获取边界发射源数组只读指针 */
const BorderSource *Game_getBorderSources(const GameState *state)
{
    return state->event.borderSources;
}

/* 获取当前活跃的随机事件类型 */
RandomEventType Game_getActiveEvent(const GameState *state)
{
    return state->event.activeEvent;
}

/* 判断是否应启用粒子特效（根据性能自动调整） */
bool Game_isUsingParticles(void)
{
    return gUseParticles;
}
