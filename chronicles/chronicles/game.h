#pragma once

#include "math.h"
#include "ega.h"

#include <vector>
#include <string>

typedef struct Texture Texture;
typedef struct EGATexture EGATexture;

typedef struct PaletteManager PaletteManager;

struct GameData {
   struct {
      ColorRGBAf bgClearColor = { 0.45f, 0.55f, 0.60f, 1.0f };  // clear color behond all imgui windows
      bool showUI = false;                               // whether to show the ui or just a fullscreen viewer
   } imgui;

   struct {
      Texture* texture = nullptr;                        // populated with egaTexture every frame
      EGATexture* egaTexture = nullptr;                  //drawn to every game update
      EGAPalette palette;

   } primaryView;

   struct {
      PaletteManager *palettes;
   } assets;
};

GameData* gameGet();

typedef struct Game Game;
Game* gameCreate();
GameData* gameData(Game* game);

typedef struct Window Window;
void gameUpdate(Game* game, Window* wnd);

void gameDestroy(Game* game);
void gameDoUI(Window* wnd);

bool paletteExists(PaletteManager* manager, StringView name);
void paletteSave(PaletteManager* manager, StringView name, EGAPalette *pal);
void paletteLoad(PaletteManager* manager, StringView name, EGAPalette *pal);
void paletteDelete(PaletteManager* manager, StringView pal);

std::vector<std::string> paletteList(PaletteManager* manager, StringView search = nullptr);


