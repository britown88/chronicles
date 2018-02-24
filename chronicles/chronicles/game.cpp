#include "game.h"
#include "app.h"

#include "imgui.h"
#include "ega.h"

struct Game {
   GameData data;
};


static void _gameDataInit(GameData* game) {
   game->rendering.primaryFrameBuffer = textureCreateCustom(EGA_RES_WIDTH, EGA_RES_HEIGHT, { RepeatType_CLAMP, FilterType_LINEAR });
   game->rendering.primaryEGAFrameBuffer = egaTextureCreate(EGA_RES_WIDTH, EGA_RES_HEIGHT);
}


Game* gameCreate() {
   auto out = new Game();
   _gameDataInit(&out->data);
   return out;
}

GameData* gameData(Game* game) { return &game->data; }

void gameUpdate(Game* game, Window* wnd) {


   gameDoUI(&game->data, wnd);
}

void gameDestroy(Game* game) {

   egaTextureDestroy(game->data.rendering.primaryEGAFrameBuffer);
   textureDestroy(game->data.rendering.primaryFrameBuffer);

   delete game;
}