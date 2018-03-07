#include "game.h"
#include "app.h"

#include "imgui.h"
#include "ega.h"
#include "chronwin.h"
#include "scf.h"
#include <unordered_map>

static const StringView PalettePath = "pal.bin";

struct Game {
   GameData data;
};


static GameData* g_gameData = nullptr;
GameData* gameGet() {
   return g_gameData;
}

struct PaletteManager {
   std::unordered_map<std::string, EGAPalette> palettes;
};

static void _loadPalettes(PaletteManager *manager) {
   u64 bSize = 0;
   auto buff = readFullFile(PalettePath, &bSize);

   if (buff) {
      auto view = scfView(buff);
      if (!scfReaderNull(view)) {
         while (!scfReaderAtEnd(view)) {
            SCFReader kvp = scfReadList(view);
            if (!scfReaderNull(kvp)) {
               std::string key;
               EGAPalette value;

               if (auto k = scfReadString(kvp)) {
                  key = k;
               }
               else {
                  return;
               }

               u32 byteCount = 0;
               if (auto bytes = scfReadBytes(kvp, &byteCount)) {
                  if (byteCount == sizeof(EGAPalette)) {
                     value = *(EGAPalette*)bytes;
                  }
               }
               else {
                  return;
               }

               manager->palettes[key] = value;
            }
            else {
               return;
            }
         }
      }
   }
}

static void _savePalettes(PaletteManager* manager) {
   auto writer = scfWriterCreate();

   for (auto &p : manager->palettes) {
      scfWriteListBegin(writer);
      scfWriteString(writer, p.first.c_str());
      scfWriteBytes(writer, (byte*)&p.second, sizeof(EGAPalette));
      scfWriteListEnd(writer);
   }

   u32 bSize = 0;
   auto out = scfWriteToBuffer(writer, &bSize);
   writeBinaryFile(PalettePath, (byte*)out, bSize);
   delete[] out;
}

bool paletteExists(PaletteManager* manager, StringView name) {
   return manager->palettes.find(name) != manager->palettes.end();
}

void paletteSave(PaletteManager* manager, StringView name, EGAPalette *pal) {
   manager->palettes[name] = *pal;
   _savePalettes(manager);
}
void paletteDelete(PaletteManager* manager, StringView pal) {
   manager->palettes.erase(pal);
   _savePalettes(manager);
}
void paletteLoad(PaletteManager* manager, StringView name, EGAPalette *pal) {
   auto found = manager->palettes.find(name);
   if (found != manager->palettes.end()) {
      *pal = found->second;
   }
}
std::vector<std::string> paletteList(PaletteManager* manager, StringView search) {
   std::vector<std::string> out;
   auto searchlen = strlen(search);
   for (auto &p : manager->palettes) {

      if (searchlen == 0 || p.first.find(search) != std::string::npos) {
         out.push_back(p.first);
      }
   }
   return out;
}

static void _gameDataInit(GameData* game) {
   egaStartup();

   game->primaryView.palette = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
   game->primaryView.egaTexture = egaTextureCreate(EGA_RES_WIDTH, EGA_RES_HEIGHT);
   game->primaryView.texture = textureCreateCustom(EGA_RES_WIDTH, EGA_RES_HEIGHT, {RepeatType_CLAMP, FilterType_NEAREST}); 

   egaClear(game->primaryView.egaTexture, 0);

   game->assets.palettes = new PaletteManager();
   _loadPalettes(game->assets.palettes);
   
}


Game* gameCreate() {
   auto out = new Game();
   _gameDataInit(&out->data);
   g_gameData = &out->data;
   return out;
}

GameData* gameData(Game* game) { return &game->data; }



void gameUpdate(Game* game, Window* wnd) {
   static int x = 0, y = 0;   

   auto ega = game->data.primaryView.egaTexture;
   //egaClear(ega, 0);

   //egaRenderPoint(ega, { x++, y++ }, x % 16);

   //egaRenderLine(ega, 
   //   { rand() % EGA_RES_WIDTH , rand() % EGA_RES_HEIGHT }, 
   //   { rand() % EGA_RES_WIDTH , rand() % EGA_RES_HEIGHT }, x++ % 13);

   egaTextureDecode(game->data.primaryView.egaTexture, game->data.primaryView.texture, &game->data.primaryView.palette);
   gameDoUI(wnd);
}

void gameDestroy(Game* game) {

   egaTextureDestroy(game->data.primaryView.egaTexture);

   _savePalettes(game->data.assets.palettes);
   delete game->data.assets.palettes;

   delete game;
}