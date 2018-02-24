#pragma once

#include "math.h"
#include "ega.h"

typedef struct Texture Texture;
typedef struct EGATexture EGATexture;

struct GameData {
   struct {
      ColorRGBf bgClearColor = { 0.45f, 0.55f, 0.60f };  // clear color behond all imgui windows
      bool showUI = false;                               // whether to show the ui or just a fullscreen viewer
   } imgui;

   struct {
      Texture* texture = nullptr;                        // populated with egaTexture every frame
      EGATexture* egaTexture = nullptr;                  //drawn to every game update
      EGAPalette palette;

   } primaryView;
};

typedef struct Game Game;
Game* gameCreate();
GameData* gameData(Game* game);

typedef struct Window Window;
void gameUpdate(Game* game, Window* wnd);

void gameDestroy(Game* game);


void gameDoUI(GameData* data, Window* wnd);
