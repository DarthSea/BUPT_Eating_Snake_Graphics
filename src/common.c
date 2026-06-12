#include "common.h"

const char *Common_modeName(GameMode mode)
{
    switch (mode) {
    case MODE_SINGLE:
        return "Single";
    case MODE_AI_BATTLE:
        return "AI Battle";
    case MODE_TIME_CHALLENGE:
        return "Time Challenge";
    case MODE_LOCAL_MULTIPLAYER:
        return "Local Multiplayer";
    default:
        return "Unknown";
    }
}

const char *Common_variantName(MapVariant variant)
{
    switch (variant) {
    case VARIANT_CLASSIC:
        return "Classic";
    case VARIANT_DIVERSE:
        return "Diverse";
    default:
        return "Unknown";
    }
}

const char *Common_aiDifficultyName(AiDifficulty difficulty)
{
    switch (difficulty) {
    case AI_EASY:
        return "Easy";
    case AI_MEDIUM:
        return "Medium";
    case AI_HARD:
        return "Hard";
    default:
        return "Unknown";
    }
}

const char *Common_resultName(GameResult result)
{
    switch (result) {
    case RESULT_RUNNING:
        return "Running";
    case RESULT_PLAYER_WIN:
        return "Player Wins";
    case RESULT_AI_WIN:
        return "AI Wins";
    case RESULT_DRAW:
        return "Draw";
    case RESULT_TIME_UP:
        return "Time Up";
    case RESULT_PLAYER_DEAD:
        return "Game Over";
    case RESULT_P1_WIN:
        return "P1 Wins";
    case RESULT_P2_WIN:
        return "P2 Wins";
    default:
        return "Unknown";
    }
}

bool Common_isOpposite(Direction a, Direction b)
{
    return (a == DIR_UP && b == DIR_DOWN)
        || (a == DIR_DOWN && b == DIR_UP)
        || (a == DIR_LEFT && b == DIR_RIGHT)
        || (a == DIR_RIGHT && b == DIR_LEFT);
}

Pos Common_nextPos(Pos pos, Direction dir)
{
    Pos next = pos;

    switch (dir) {
    case DIR_UP:
        next.row--;
        break;
    case DIR_DOWN:
        next.row++;
        break;
    case DIR_LEFT:
        next.col--;
        break;
    case DIR_RIGHT:
        next.col++;
        break;
    default:
        break;
    }

    return next;
}

int Common_manhattan(Pos a, Pos b)
{
    int rowDiff = a.row - b.row;
    int colDiff = a.col - b.col;

    if (rowDiff < 0) {
        rowDiff = -rowDiff;
    }
    if (colDiff < 0) {
        colDiff = -colDiff;
    }

    return rowDiff + colDiff;
}
