#include "ai.h"
#include "game.h"

#define AI_BAD_SCORE (-1000000000)

typedef struct QueueNode {
    Pos pos;
} QueueNode;

static const Direction allDirs[4] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };

static bool posEquals(Pos a, Pos b)
{
    return a.row == b.row && a.col == b.col;
}

static bool isBombDanger(const GameState *state, Pos pos)
{
    return state->event.activeEvent == EVENT_BOMBARDMENT
        && state->event.bombActive
        && Game_isInBombZone(state, pos);
}

static bool isBombZoneActive(const GameState *state, Pos pos)
{
    return state->event.activeEvent == EVENT_BOMBARDMENT
        && Game_isInBombZone(state, pos);
}

static bool terrainBlocks(const GameState *state, const Snake *snake, Pos pos)
{
    CellType cell;

    if (!Game_isInside(state, pos)) {
        return true;
    }

    cell = state->cells[pos.row][pos.col];
    if (cell == CELL_WALL || cell == CELL_OBSTACLE) {
        return true;
    }
    if (cell == CELL_TRAP && snake->shieldMs <= 0) {
        return true;
    }
    if (cell == CELL_BATTLE_SPIKE && snake->shieldCharges <= 0) {
        return true;
    }
    /* 炸弹活跃时，轰炸区不可通行 */
    if (isBombDanger(state, pos)
        && snake->shieldCharges <= 0 && snake->shieldMs <= 0) {
        return true;
    }

    return false;
}

static bool candidateWillGrow(const GameState *state, const Snake *snake, Pos next)
{
    CellType cell = state->cells[next.row][next.col];
    bool intervalGrow = state->config.enableStepGrowth
        && state->config.growthInterval > 0
        && snake->stepCount + 1 >= state->config.growthInterval;

    return Game_itemGrows(cell) || intervalGrow;
}

static bool isSafeCandidate(const GameState *state, Direction dir)
{
    const Snake *ai = &state->ai;
    Pos next;
    bool grow;

    if (Common_isOpposite(dir, ai->dir)) {
        return false;
    }

    next = Common_nextPos(ai->body[0], dir);
    if (terrainBlocks(state, ai, next)) {
        return false;
    }
    /* 轰炸区在炸弹活跃时致命 */
    if (isBombDanger(state, next)
        && ai->shieldCharges <= 0 && ai->shieldMs <= 0) {
        return false;
    }

    grow = candidateWillGrow(state, ai, next);
    if (Game_snakeContains(ai, next, !grow)) {
        return false;
    }
    if (Game_snakeContains(&state->player, next, false)) {
        return false;
    }

    return true;
}

static int itemValueForAi(CellType cell)
{
    if (cell == CELL_BATTLE_BOW) {
        return 35;
    }
    if (cell == CELL_BATTLE_SHIELD) {
        return 28;
    }
    if (cell == CELL_BATTLE_CLOCK) {
        return 24;
    }
    if (cell == CELL_SHIELD) {
        return 16;
    }

    return Game_itemScore(cell);
}

static bool findBestItemByDistance(const GameState *state, Pos from, Pos *target)
{
    int mapSize = Game_validMapSize(state->config.mapSize);
    int row;
    int col;
    int bestScore = AI_BAD_SCORE;
    bool found = false;

    for (row = 1; row < mapSize - 1; row++) {
        for (col = 1; col < mapSize - 1; col++) {
            CellType cell = state->cells[row][col];
            Pos pos; pos.row = row; pos.col = col;
            if (Game_isFood(cell)
                || cell == CELL_SHIELD
                || cell == CELL_BATTLE_BOW
                || cell == CELL_BATTLE_SHIELD
                || cell == CELL_BATTLE_CLOCK) {
                int dist;
                int score;

                /* 跳过活跃炸弹区内的物品 */
                if (isBombDanger(state, pos)) {
                    continue;
                }

                dist = Common_manhattan(from, pos);
                score = itemValueForAi(cell) * 10 - dist * 4;
                if (!found || score > bestScore) {
                    bestScore = score;
                    *target = pos;
                    found = true;
                }
            }
        }
    }

    return found;
}

static int floodAreaAfterMove(const GameState *state, Pos start, bool grow)
{
    int mapSize = Game_validMapSize(state->config.mapSize);
    int visited[MAX_MAP_SIZE][MAX_MAP_SIZE] = { 0 };
    QueueNode queue[MAX_SNAKE_LEN];
    int front = 0;
    int rear = 0;
    int count = 0;
    int row;
    int col;
    int i;

    for (row = 0; row < mapSize; row++) {
        for (col = 0; col < mapSize; col++) {
            CellType cell = state->cells[row][col];
            Pos p; p.row = row; p.col = col;
            if (cell == CELL_WALL || cell == CELL_OBSTACLE
                || (cell == CELL_TRAP && state->ai.shieldMs <= 0)
                || (cell == CELL_BATTLE_SPIKE && state->ai.shieldCharges <= 0)
                || (isBombDanger(state, p)
                    && state->ai.shieldCharges <= 0 && state->ai.shieldMs <= 0)) {
                visited[row][col] = 1;
            }
        }
    }

    for (i = 0; i < state->ai.length; i++) {
        if (!grow && i == state->ai.length - 1) {
            continue;
        }
        visited[state->ai.body[i].row][state->ai.body[i].col] = 1;
    }
    for (i = 0; i < state->player.length; i++) {
        visited[state->player.body[i].row][state->player.body[i].col] = 1;
    }

    if (!Game_isInside(state, start)) {
        return 0;
    }
    visited[start.row][start.col] = 1;
    queue[rear++].pos = start;

    while (front < rear) {
        QueueNode node = queue[front++];
        int d;

        count++;
        for (d = 0; d < 4; d++) {
            Pos next = Common_nextPos(node.pos, allDirs[d]);
            if (Game_isInside(state, next) && !visited[next.row][next.col]) {
                visited[next.row][next.col] = 1;
                queue[rear++].pos = next;
            }
        }
    }

    return count;
}

static int bfsDistance(const GameState *state, Pos start, Pos target, bool includePlayer)
{
    int mapSize = Game_validMapSize(state->config.mapSize);
    int visited[MAX_MAP_SIZE][MAX_MAP_SIZE] = { 0 };
    int distance[MAX_MAP_SIZE][MAX_MAP_SIZE] = { 0 };
    QueueNode queue[MAX_SNAKE_LEN];
    int front = 0;
    int rear = 0;
    int row;
    int col;
    int i;

    if (!Game_isInside(state, target)) {
        return 9999;
    }

    for (row = 0; row < mapSize; row++) {
        for (col = 0; col < mapSize; col++) {
            CellType cell = state->cells[row][col];
            Pos p; p.row = row; p.col = col;
            if (cell == CELL_WALL || cell == CELL_OBSTACLE
                || (cell == CELL_TRAP && state->ai.shieldMs <= 0)
                || (cell == CELL_BATTLE_SPIKE && state->ai.shieldCharges <= 0)
                || (isBombDanger(state, p)
                    && state->ai.shieldCharges <= 0 && state->ai.shieldMs <= 0)) {
                visited[row][col] = 1;
            }
        }
    }

    for (i = 0; i < state->ai.length - 1; i++) {
        visited[state->ai.body[i].row][state->ai.body[i].col] = 1;
    }
    if (includePlayer) {
        for (i = 0; i < state->player.length; i++) {
            visited[state->player.body[i].row][state->player.body[i].col] = 1;
        }
    }

    visited[start.row][start.col] = 1;
    queue[rear++].pos = start;

    while (front < rear) {
        QueueNode node = queue[front++];
        int d;

        if (posEquals(node.pos, target)) {
            return distance[node.pos.row][node.pos.col];
        }

        for (d = 0; d < 4; d++) {
            Pos next = Common_nextPos(node.pos, allDirs[d]);
            if (Game_isInside(state, next) && !visited[next.row][next.col]) {
                visited[next.row][next.col] = 1;
                distance[next.row][next.col] = distance[node.pos.row][node.pos.col] + 1;
                queue[rear++].pos = next;
            }
        }
    }

    return 9999;
}

static Direction bfsFirstStepTo(const GameState *state, Pos start, Pos target)
{
    int mapSize = Game_validMapSize(state->config.mapSize);
    int visited[MAX_MAP_SIZE][MAX_MAP_SIZE] = { 0 };
    Direction firstStep[MAX_MAP_SIZE][MAX_MAP_SIZE];
    QueueNode queue[MAX_SNAKE_LEN];
    int front = 0;
    int rear = 0;
    int row;
    int col;
    int i;

    for (row = 0; row < mapSize; row++) {
        for (col = 0; col < mapSize; col++) {
            CellType cell = state->cells[row][col];
            Pos p; p.row = row; p.col = col;
            firstStep[row][col] = DIR_NONE;
            if (cell == CELL_WALL || cell == CELL_OBSTACLE
                || (cell == CELL_TRAP && state->ai.shieldMs <= 0)
                || (cell == CELL_BATTLE_SPIKE && state->ai.shieldCharges <= 0)
                || (isBombDanger(state, p)
                    && state->ai.shieldCharges <= 0 && state->ai.shieldMs <= 0)) {
                visited[row][col] = 1;
            }
        }
    }

    for (i = 0; i < state->ai.length - 1; i++) {
        visited[state->ai.body[i].row][state->ai.body[i].col] = 1;
    }
    for (i = 0; i < state->player.length; i++) {
        visited[state->player.body[i].row][state->player.body[i].col] = 1;
    }

    if (!Game_isInside(state, target) || visited[target.row][target.col]) {
        return DIR_NONE;
    }

    visited[start.row][start.col] = 1;
    queue[rear++].pos = start;

    while (front < rear) {
        QueueNode node = queue[front++];
        int d;

        if (posEquals(node.pos, target)) {
            return firstStep[node.pos.row][node.pos.col];
        }

        for (d = 0; d < 4; d++) {
            Pos next = Common_nextPos(node.pos, allDirs[d]);
            if (Game_isInside(state, next) && !visited[next.row][next.col]) {
                visited[next.row][next.col] = 1;
                firstStep[next.row][next.col] = posEquals(node.pos, start)
                    ? allDirs[d]
                    : firstStep[node.pos.row][node.pos.col];
                queue[rear++].pos = next;
            }
        }
    }

    return DIR_NONE;
}

static Direction bestAreaDirection(const GameState *state)
{
    Direction bestDir = DIR_NONE;
    int bestArea = -1;
    int d;

    for (d = 0; d < 4; d++) {
        Direction dir = allDirs[d];
        Pos next;
        bool grow;
        int area;

        if (!isSafeCandidate(state, dir)) {
            continue;
        }

        next = Common_nextPos(state->ai.body[0], dir);
        grow = candidateWillGrow(state, &state->ai, next);
        area = floodAreaAfterMove(state, next, grow);
        if (area > bestArea) {
            bestArea = area;
            bestDir = dir;
        }
    }

    return bestDir;
}

static Direction decideEasy(const GameState *state)
{
    Pos target;
    Direction bestDir = DIR_NONE;
    int bestDist = 9999;
    int d;

    if (!findBestItemByDistance(state, state->ai.body[0], &target)) {
        return bestAreaDirection(state);
    }

    for (d = 0; d < 4; d++) {
        Direction dir = allDirs[d];
        Pos next;
        int dist;

        if (!isSafeCandidate(state, dir)) {
            continue;
        }

        next = Common_nextPos(state->ai.body[0], dir);
        dist = Common_manhattan(next, target);
        if (dist < bestDist) {
            bestDist = dist;
            bestDir = dir;
        }
    }

    return bestDir == DIR_NONE ? bestAreaDirection(state) : bestDir;
}

static Direction decideMedium(const GameState *state)
{
    Pos target;
    Direction pathDir;

    if (findBestItemByDistance(state, state->ai.body[0], &target)) {
        pathDir = bfsFirstStepTo(state, state->ai.body[0], target);
        if (pathDir != DIR_NONE && isSafeCandidate(state, pathDir)) {
            Pos next = Common_nextPos(state->ai.body[0], pathDir);
            bool grow = candidateWillGrow(state, &state->ai, next);
            if (floodAreaAfterMove(state, next, grow) > state->ai.length + 6) {
                return pathDir;
            }
        }
    }

    return bestAreaDirection(state);
}

static int countPlayerLegalMovesAfterAiStep(const GameState *state, Pos aiNext)
{
    const Snake *player = &state->player;
    int legal = 0;
    int d;

    for (d = 0; d < 4; d++) {
        Direction dir = allDirs[d];
        Pos next;
        CellType cell;
        bool grow;

        if (Common_isOpposite(dir, player->dir)) {
            continue;
        }

        next = Common_nextPos(player->body[0], dir);
        if (!Game_isInside(state, next) || posEquals(next, aiNext)) {
            continue;
        }

        cell = state->cells[next.row][next.col];
        if (cell == CELL_WALL || cell == CELL_OBSTACLE || cell == CELL_TRAP
            || (cell == CELL_BATTLE_SPIKE && player->shieldCharges <= 0)) {
            continue;
        }
        /* 活跃炸弹区也阻断玩家 */
        if (isBombDanger(state, next)
            && player->shieldCharges <= 0 && player->shieldMs <= 0) {
            continue;
        }

        grow = Game_itemGrows(cell)
            || (state->config.enableStepGrowth
                && state->config.growthInterval > 0
                && player->stepCount + 1 >= state->config.growthInterval);
        if (Game_snakeContains(player, next, !grow)) {
            continue;
        }
        if (Game_snakeContains(&state->ai, next, false)) {
            continue;
        }

        legal++;
    }

    return legal;
}

static int bestFoodDistanceFrom(const GameState *state, Pos from)
{
    int mapSize = Game_validMapSize(state->config.mapSize);
    int row;
    int col;
    int best = 9999;

    for (row = 1; row < mapSize - 1; row++) {
        for (col = 1; col < mapSize - 1; col++) {
            if (Game_isFood(state->cells[row][col])) {
                Pos food;
                int dist;

                food.row = row;
                food.col = col;
                dist = bfsDistance(state, from, food, true);
                if (dist < best) {
                    best = dist;
                }
            }
        }
    }

    return best;
}

static int hardCandidateScore(const GameState *state, Direction dir)
{
    Pos next;
    CellType cell;
    bool grow;
    int area;
    int foodDist;
    int playerFoodDist;
    int playerMoves;
    int playerDist;
    int score;

    if (!isSafeCandidate(state, dir)) {
        return AI_BAD_SCORE;
    }

    next = Common_nextPos(state->ai.body[0], dir);
    cell = state->cells[next.row][next.col];
    grow = candidateWillGrow(state, &state->ai, next);
    area = floodAreaAfterMove(state, next, grow);
    foodDist = bestFoodDistanceFrom(state, next);
    playerFoodDist = bestFoodDistanceFrom(state, state->player.body[0]);
    playerMoves = countPlayerLegalMovesAfterAiStep(state, next);
    playerDist = Common_manhattan(next, state->player.body[0]);

    score = 0;
    score += area * 3;
    score += itemValueForAi(cell) * 85;
    score -= foodDist * 18;

    if (area < state->ai.length + 5) {
        score -= 520;
    }
    if (foodDist <= playerFoodDist) {
        score += 70;
    }
    if (playerDist <= 4) {
        score += (5 - playerDist) * (state->ai.length >= state->player.length ? 28 : 14);
    }
    score += (4 - playerMoves) * 35;

    if (dir == state->ai.dir) {
        score += 8;
    }
    if (cell == CELL_TRAP && state->ai.shieldMs > 0) {
        score += 35;
    }
    if (cell == CELL_SHIELD && state->ai.shieldMs == 0) {
        score += 120;
    }
    if (cell == CELL_BATTLE_BOW) {
        score += state->ai.bowArrows == 0 ? 220 : 90;
    }
    if (cell == CELL_BATTLE_SHIELD) {
        score += state->ai.shieldCharges == 0 ? 180 : 75;
    }
    if (cell == CELL_BATTLE_CLOCK) {
        score += Common_manhattan(state->ai.body[0], state->player.body[0]) < 10 ? 170 : 80;
    }
    if (cell == CELL_BATTLE_SPIKE) {
        score += state->ai.shieldCharges > 0 ? -80 : -1000;
    }
    /* 轰炸区惩罚 */
    if (isBombZoneActive(state, next)) {
        if (isBombDanger(state, next)) {
            score += state->ai.shieldCharges > 0 || state->ai.shieldMs > 0 ? -300 : AI_BAD_SCORE / 2;
        } else {
            score -= 120;  /* 非活跃期也尽量远离 */
        }
    }
    /* 箭雨期间避免靠近边界 */
    if (state->event.activeEvent == EVENT_ARROW_STORM) {
        int borderDist;
        int mapSize = Game_validMapSize(state->config.mapSize);
        borderDist = next.row;
        if (next.col < borderDist) borderDist = next.col;
        if (mapSize - 1 - next.row < borderDist) borderDist = mapSize - 1 - next.row;
        if (mapSize - 1 - next.col < borderDist) borderDist = mapSize - 1 - next.col;
        if (borderDist <= 2) {
            score -= 60;
        }
    }
    if (state->ai.bowArrows > 0 && Game_hasClearShot(state, AI_INDEX, false)) {
        score += 260;
    }

    return score;
}

static Direction decideHard(const GameState *state)
{
    Direction bestDir = DIR_NONE;
    int bestScore = AI_BAD_SCORE;
    int d;

    for (d = 0; d < 4; d++) {
        Direction dir = allDirs[d];
        int score = hardCandidateScore(state, dir);

        if (score > bestScore) {
            bestScore = score;
            bestDir = dir;
        }
    }

    return bestDir == DIR_NONE ? bestAreaDirection(state) : bestDir;
}

Direction Ai_decideDirection(const GameState *state, AiDifficulty difficulty)
{
    Direction dir;

    if (!state->ai.alive) {
        return DIR_NONE;
    }

    if (difficulty == AI_EASY) {
        dir = decideEasy(state);
    } else if (difficulty == AI_MEDIUM) {
        dir = decideMedium(state);
    } else {
        dir = decideHard(state);
    }

    if (dir == DIR_NONE) {
        dir = state->ai.dir;
    }

    return dir;
}
