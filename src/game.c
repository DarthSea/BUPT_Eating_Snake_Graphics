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

static bool posEquals(Pos a, Pos b)
{
    return a.row == b.row && a.col == b.col;
}

static bool terrainBlocksArrow(CellType cell)
{
    return cell == CELL_WALL || cell == CELL_OBSTACLE;
}

static unsigned int nextRandom(GameState *state)
{
    state->randomSeed = state->randomSeed * 1103515245u + 12345u;
    return state->randomSeed;
}

static int randomRange(GameState *state, int limit)
{
    if (limit <= 0) {
        return 0;
    }

    return (int)(nextRandom(state) % (unsigned int)limit);
}

static int mapSizeOf(const GameState *state)
{
    return Game_validMapSize(state->config.mapSize);
}

static int maxSnakeLengthOf(const GameState *state)
{
    int mapSize = mapSizeOf(state);

    return mapSize * mapSize;
}

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

static void setStatus(GameState *state, const char *text)
{
    snprintf(state->statusText, sizeof(state->statusText), "%s", text);
}

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

static bool spawnSpecificItem(GameState *state, CellType item)
{
    int mapSize = mapSizeOf(state);
    int tries;
    int row;
    int col;

    for (tries = 0; tries < mapSize * mapSize * 2; tries++) {
        Pos pos;

        pos.row = 1 + randomRange(state, mapSize - 2);
        pos.col = 1 + randomRange(state, mapSize - 2);
        if (isSpawnFree(state, pos)) {
            state->cells[pos.row][pos.col] = item;
            return true;
        }
    }

    for (row = 1; row < mapSize - 1; row++) {
        for (col = 1; col < mapSize - 1; col++) {
            Pos pos;

            pos.row = row;
            pos.col = col;
            if (isSpawnFree(state, pos)) {
                state->cells[row][col] = item;
                return true;
            }
        }
    }

    return false;
}

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

static void maintainItems(GameState *state)
{
    if (state->config.mode == MODE_AI_BATTLE || state->config.mode == MODE_LOCAL_MULTIPLAYER) {
        maintainBattleItems(state);
    } else {
        maintainNormalItems(state);
    }
}

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
        if (isSpawnFree(state, pos)) {
            state->cells[pos.row][pos.col] = CELL_OBSTACLE;
            placed++;
        }
    }

    for (row = 1; row < mapSize - 1 && placed < target; row++) {
        for (col = 1; col < mapSize - 1 && placed < target; col++) {
            Pos pos;

            pos.row = row;
            pos.col = col;
            if (isSpawnFree(state, pos)) {
                state->cells[row][col] = CELL_OBSTACLE;
                placed++;
            }
        }
    }
}

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

static bool snakeMovesThisBattleStep(Snake *snake)
{
    if (snake->slowMs <= 0) {
        return true;
    }

    snake->slowStepCounter = 1 - snake->slowStepCounter;
    return snake->slowStepCounter == 0;
}

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

static void addSingleSnakeCollisions(const GameState *state, StepPlan *plan)
{
    if (plan->dead) {
        return;
    }

    if (Game_snakeContains(&state->player, plan->next, !plan->grow)) {
        plan->dead = true;
    }
}

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

void Game_applyModeDefaults(GameConfig *config, GameMode mode)
{
    config->mode = mode;
    config->mapSize = Game_validMapSize(config->mapSize);
    config->growthInterval = config->enableStepGrowth ? DEFAULT_GROWTH_INTERVAL : 0;

    if (mode == MODE_SINGLE) {
        config->moveIntervalMs = 140;
        config->timeLimitSeconds = 0;
    } else if (mode == MODE_AI_BATTLE) {
        config->moveIntervalMs = 130;
        config->timeLimitSeconds = 0;
    } else if (mode == MODE_TIME_CHALLENGE) {
        config->moveIntervalMs = 115;
        config->timeLimitSeconds = TIME_LIMIT_SECONDS;
    } else if (mode == MODE_LOCAL_MULTIPLAYER) {
        config->moveIntervalMs = 130;
        config->timeLimitSeconds = TIME_LIMIT_SECONDS;
        config->variant = VARIANT_DIVERSE;
    }
}

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

static bool shouldTriggerEvent(const GameState *state)
{
    if (state->config.variant != VARIANT_DIVERSE) {
        return false;
    }
    if (state->config.mode != MODE_SINGLE
        && state->config.mode != MODE_AI_BATTLE
        && state->config.mode != MODE_LOCAL_MULTIPLAYER) {
        return false;
    }
    return true;
}

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

void Game_setPlayerDirection(GameState *state, Direction dir)
{
    if (dir == DIR_NONE || state->result != RESULT_RUNNING) {
        return;
    }

    setSnakeDirectionWithSlow(&state->player, dir);
}

void Game_setP2Direction(GameState *state, Direction dir)
{
    if (dir == DIR_NONE || state->result != RESULT_RUNNING) {
        return;
    }
    setSnakeDirectionWithSlow(&state->ai, dir);
}

void Game_update(GameState *state, int deltaMs)
{
    int interval;
    int steps = 0;

    if (state->result != RESULT_RUNNING) {
        return;
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

bool Game_isFinished(const GameState *state)
{
    return state->result != RESULT_RUNNING;
}

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

bool Game_playerFireArrow(GameState *state)
{
    return fireArrow(state, PLAYER_INDEX);
}

bool Game_aiFireArrow(GameState *state)
{
    return fireArrow(state, AI_INDEX);
}

bool Game_player2FireArrow(GameState *state)
{
    return fireArrow(state, P2_INDEX);
}

bool Game_hasClearShot(const GameState *state, int shooterIndex, bool requireHead)
{
    return findArrowHitIndex(state, shooterIndex, requireHead) >= 0;
}

bool Game_isInsideMap(Pos pos, int mapSize)
{
    mapSize = Game_validMapSize(mapSize);

    return pos.row >= 0 && pos.row < mapSize && pos.col >= 0 && pos.col < mapSize;
}

bool Game_isInside(const GameState *state, Pos pos)
{
    return Game_isInsideMap(pos, state->config.mapSize);
}

bool Game_isFood(CellType cell)
{
    return cell == CELL_FOOD
        || cell == CELL_FOOD_BONUS
        || cell == CELL_FOOD_SPEED
        || cell == CELL_FOOD_SLOW;
}

bool Game_isBattleItem(CellType cell)
{
    return cell == CELL_BATTLE_BOW
        || cell == CELL_BATTLE_SHIELD
        || cell == CELL_BATTLE_SPIKE
        || cell == CELL_BATTLE_CLOCK;
}

bool Game_isDangerCell(CellType cell)
{
    return cell == CELL_WALL
        || cell == CELL_OBSTACLE
        || cell == CELL_TRAP
        || cell == CELL_BATTLE_SPIKE;
}

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

bool Game_itemGrows(CellType cell)
{
    return cell == CELL_FOOD
        || cell == CELL_FOOD_BONUS
        || cell == CELL_FOOD_SPEED
        || cell == CELL_FOOD_SLOW;
}

int Game_validMapSize(int mapSize)
{
    if (mapSize == 50 || mapSize == 100) {
        return mapSize;
    }

    return DEFAULT_MAP_SIZE;
}

const Snake *Game_getSnake(const GameState *state, int snakeIndex)
{
    return snakeIndex == AI_INDEX ? &state->ai : &state->player;
}

Snake *Game_getMutableSnake(GameState *state, int snakeIndex)
{
    return snakeIndex == AI_INDEX ? &state->ai : &state->player;
}

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

void Game_pushSoundEvent(GameState *state, SoundEvent event)
{
    if (event == SOUND_NONE || state->soundEventCount >= MAX_SOUND_EVENTS) {
        return;
    }

    state->soundEvents[state->soundEventCount++] = event;
}

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

bool Game_isInBombZone(const GameState *state, Pos pos)
{
    return isInBombZone(&state->event, pos);
}

bool Game_isBombActive(const GameState *state)
{
    return state->event.bombActive;
}

bool Game_isBorderFlashing(const GameState *state)
{
    return state->event.borderFlashing;
}

int Game_getBorderSourceCount(const GameState *state)
{
    return state->event.borderSourceCount;
}

const BorderSource *Game_getBorderSources(const GameState *state)
{
    return state->event.borderSources;
}

RandomEventType Game_getActiveEvent(const GameState *state)
{
    return state->event.activeEvent;
}
