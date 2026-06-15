#ifndef BUPT_SNAKE_GAME_H
#define BUPT_SNAKE_GAME_H

#include "common.h"

void Game_makeDefaultConfig(GameConfig *config);
void Game_applyModeDefaults(GameConfig *config, GameMode mode);
void Game_init(GameState *state, const GameConfig *config);

/* Called once when the player presses W/A/S/D at the start prompt. */
void Game_preparePlayerStart(GameState *state, Direction dir);
void Game_prepareP2Start(GameState *state, Direction dir);
void Game_setPlayerDirection(GameState *state, Direction dir);
void Game_setP2Direction(GameState *state, Direction dir);
void Game_update(GameState *state, int deltaMs);
bool Game_isFinished(const GameState *state);

void Game_adjustSpeed(GameState *state, int delta);
bool Game_playerFireArrow(GameState *state);
bool Game_player2FireArrow(GameState *state);
bool Game_aiFireArrow(GameState *state);
bool Game_hasClearShot(const GameState *state, int shooterIndex, bool requireHead);

bool Game_isInsideMap(Pos pos, int mapSize);
bool Game_isInside(const GameState *state, Pos pos);
bool Game_isFood(CellType cell);
bool Game_isBattleItem(CellType cell);
bool Game_isDangerCell(CellType cell);
int Game_itemScore(CellType cell);
bool Game_itemGrows(CellType cell);
int Game_validMapSize(int mapSize);
const Snake *Game_getSnake(const GameState *state, int snakeIndex);
Snake *Game_getMutableSnake(GameState *state, int snakeIndex);
bool Game_snakeContains(const Snake *snake, Pos pos, bool ignoreTail);
bool Game_cellHasSnake(const GameState *state, Pos pos, int ignoreSnakeIndex, bool ignoreTail);
int Game_countFoodCells(const GameState *state);
void Game_pushSoundEvent(GameState *state, SoundEvent event);
int Game_consumeSoundEvents(GameState *state, SoundEvent outEvents[], int maxCount);

/* Random event queries for rendering layer */
bool Game_isInBombZone(const GameState *state, Pos pos);
bool Game_isBombActive(const GameState *state);
bool Game_isBorderFlashing(const GameState *state);
int Game_getBorderSourceCount(const GameState *state);
const BorderSource *Game_getBorderSources(const GameState *state);
RandomEventType Game_getActiveEvent(const GameState *state);
bool Game_isUsingParticles(void);

#endif
