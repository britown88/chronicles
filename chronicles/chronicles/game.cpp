#include "game.h"
#include "app.h"

#include "imgui.h"
#include "ega.h"

struct Game {
   GameData data;
};


static void _gameDataInit(GameData* game) {
   game->primaryView.palette = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
   game->primaryView.egaTexture = egaTextureCreate(EGA_RES_WIDTH, EGA_RES_HEIGHT);
   game->primaryView.texture = egaTextureDecode(game->primaryView.egaTexture, &game->primaryView.palette);
}


Game* gameCreate() {
   auto out = new Game();
   _gameDataInit(&out->data);
   return out;
}

GameData* gameData(Game* game) { return &game->data; }



void gameUpdate(Game* game, Window* wnd) {
   static int i = 0;   

   auto ega = game->data.primaryView.egaTexture;
   egaClear(ega, 0);

   egaTextureDecode(game->data.primaryView.egaTexture, &game->data.primaryView.palette);
   gameDoUI(&game->data, wnd);
}

void gameDestroy(Game* game) {

   egaTextureDestroy(game->data.primaryView.egaTexture);

   delete game;
}