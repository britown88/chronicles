#pragma once

#include "math.h"

struct GameData {
   struct {
      ColorRGBf bgClearColor = { 0.45f, 0.55f, 0.60f };
   } imgui;
};

typedef struct Game Game;
Game* gameCreate();
GameData* gameData(Game* game);

typedef struct Window Window;
void gameUpdate(Game* game, Window* wnd);

void gameDestroy(Game* game);


