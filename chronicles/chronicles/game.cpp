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
   game->primaryView.texture = textureCreateCustom(EGA_RES_WIDTH, EGA_RES_HEIGHT, {RepeatType_CLAMP, FilterType_NEAREST}); 

   egaClear(game->primaryView.egaTexture, 0);
}


Game* gameCreate() {
   auto out = new Game();
   _gameDataInit(&out->data);
   return out;
}

GameData* gameData(Game* game) { return &game->data; }



void gameUpdate(Game* game, Window* wnd) {
   static int x = 0, y = 0;   

   auto ega = game->data.primaryView.egaTexture;
   //egaClear(ega, 0);

   egaRenderPoint(ega, { x++, y++ }, x % 16);

   egaRenderLine(ega, 
      { rand() % EGA_RES_WIDTH , rand() % EGA_RES_HEIGHT }, 
      { rand() % EGA_RES_WIDTH , rand() % EGA_RES_HEIGHT }, x % 16);

   

   egaTextureDecode(game->data.primaryView.egaTexture, game->data.primaryView.texture, &game->data.primaryView.palette);
   gameDoUI(&game->data, wnd);
}

void gameDestroy(Game* game) {

   egaTextureDestroy(game->data.primaryView.egaTexture);

   delete game;
}