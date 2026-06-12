#ifndef BUPT_SNAKE_RENDER_H
#define BUPT_SNAKE_RENDER_H

#include <stdbool.h>
#include <tchar.h>
#include <graphics.h>
#include "common.h"

typedef enum TextureId {
    TEX_GROUND = 0,
    TEX_WALL,
    TEX_OBSTACLE,
    TEX_FOOD,
    TEX_FOOD_BONUS,
    TEX_FOOD_SPEED,
    TEX_FOOD_SLOW,
    TEX_TRAP,
    TEX_SHIELD,
    TEX_BOW,
    TEX_BATTLE_SHIELD,
    TEX_SPIKE,
    TEX_CLOCK,
    TEX_PLAYER_HEAD,
    TEX_PLAYER_BODY,
    TEX_AI_HEAD,
    TEX_AI_BODY,
    TEX_COUNT
} TextureId;

typedef struct TextureSlot {
    IMAGE image;
    bool loaded;
    COLORREF fallbackColor;
} TextureSlot;

typedef struct RenderContext {
    TextureSlot textures[TEX_COUNT];
    int skinId;
    int textureCellSize;
    int boardPixelSize;
    int windowWidth;
    int windowHeight;
    DisplayResolution resolution;
    bool fullscreen;
    TCHAR skinName[64];
} RenderContext;

bool Render_init(RenderContext *render, int skinId);
void Render_shutdown(void);
bool Render_loadSkin(RenderContext *render, int skinId);
bool Render_applyResolution(RenderContext *render, DisplayResolution resolution);
bool Render_applyDisplayMode(RenderContext *render, DisplayResolution resolution, bool fullscreen);
int Render_skinCount(void);
const TCHAR *Render_skinDisplayName(int skinId);
const TCHAR *Render_resolutionDisplayName(DisplayResolution resolution);

void Render_drawWelcome(int selected);
void Render_drawVariantMenu(MapVariant selected);
void Render_drawDifficultyMenu(AiDifficulty selected);
void Render_drawSkinMenu(int selectedSkin);
void Render_drawSettings(const GameConfig *config, int selectedRow);
void Render_drawGame(RenderContext *render, const GameState *state,
    bool paused, bool waitingForStart);
void Render_drawGameOver(const GameState *state, int selectedAction);

#endif
