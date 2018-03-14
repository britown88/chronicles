#pragma once

#include "math.h"
#include "ega.h"

#include <vector>
#include <string>

typedef struct Texture Texture;
typedef struct EGATexture EGATexture;

typedef struct Assets Assets;

struct GameData {
   struct {
      ColorRGBAf bgClearColor = { 0.45f, 0.55f, 0.60f, 1.0f };  // clear color behond all imgui windows
      bool showUI = true;                               // whether to show the ui or just a fullscreen viewer
   } imgui;

   struct {
      Texture* texture = nullptr;                        // populated with egaTexture every frame
      EGATexture* egaTexture = nullptr;                  //drawn to every game update
      EGAPalette palette;

   } primaryView;

   Assets *assets = nullptr;
};

GameData* gameGet();

typedef struct Game Game;
Game* gameCreate(StringView assetsFolder);
GameData* gameData(Game* game);

typedef struct Window Window;
void gameUpdate(Game* game, Window* wnd);

void gameDestroy(Game* game);
void gameDoUI(Window* wnd);

void        assetsPaletteStore(Assets *assets, StringView name, EGAPalette *pal);
void        assetsPaletteDelete(Assets *assets, StringView name);
EGAPalette *assetsPaletteRetrieve(Assets *assets, StringView name);
std::vector<std::string> assetsPaletteGetList(Assets *assets, StringView search = nullptr);



