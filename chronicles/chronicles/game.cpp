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

struct Assets {
   StringView assetsFolder = nullptr;

   std::unordered_map<std::string, EGAPalette*> palettes;
};

static void _assetsDestroy(Assets* assets) {
   for (auto &p : assets->palettes) {
      delete p.second;
   }

   delete assets;
}

static GameData* g_gameData = nullptr;
GameData* gameGet() {
   return g_gameData;
}


static std::string _assetPath(Assets *assets, StringView path) {
   return assets->assetsFolder ? format("%s/%s", assets->assetsFolder, path) : path;
}

static void _loadPalettes(Assets *assets) {
   u64 bSize = 0;
   auto buff = readFullFile(_assetPath(assets, PalettePath).c_str(), &bSize);

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

               if (auto existing = assetsPaletteRetrieve(assets, key.c_str())) {
                  *existing = value;
               }
               else {
                  EGAPalette *newPal = new EGAPalette;
                  *newPal = value;
                  assets->palettes.insert({ key, newPal });
               }
            }
            else {
               return;
            }
         }
      }
   }
}

static void _savePalettes(Assets* assets) {
   auto writer = scfWriterCreate();

   for (auto &p : assets->palettes) {
      scfWriteListBegin(writer);
      scfWriteString(writer, p.first.c_str());
      scfWriteBytes(writer, (byte*)&p.second, sizeof(EGAPalette));
      scfWriteListEnd(writer);
   }

   u32 bSize = 0;
   auto out = scfWriteToBuffer(writer, &bSize);
   writeBinaryFile(_assetPath(assets, PalettePath).c_str(), (byte*)out, bSize);
   delete[] out;
}

void assetsPaletteStore(Assets *assets, StringView name, EGAPalette *pal) {
   if (auto existing = assetsPaletteRetrieve(assets, name)) {
      *existing = *pal;
   }
   else {
      EGAPalette *newPal = new EGAPalette;
      *newPal = *pal;
      assets->palettes.insert({ name, newPal });
   }
   _savePalettes(assets);
}
void assetsPaletteDelete(Assets *assets, StringView name) {
   if (auto existing = assetsPaletteRetrieve(assets, name)) {
      delete existing;
      assets->palettes.erase(name);
   }
   
   _savePalettes(assets);
}
EGAPalette *assetsPaletteRetrieve(Assets *assets, StringView name) {
   auto found = assets->palettes.find(name);
   if (found != assets->palettes.end()) {
      return found->second;
   }
   return nullptr;
}
std::vector<std::string> assetsPaletteGetList(Assets *assets, StringView search) {
   std::vector<std::string> out;
   auto searchlen = strlen(search);
   for (auto p : assets->palettes) {
      if (searchlen == 0 || p.first.find(search) != std::string::npos) {
         out.push_back(p.first);
      }
   }
   return out;
}

static void _gameDataInit(GameData* game, StringView assetsFolder) {
   egaStartup();

   game->assets = new Assets();
   game->assets->assetsFolder = assetsFolder;

   game->primaryView.palette = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
   game->primaryView.egaTexture = egaTextureCreate(EGA_RES_WIDTH, EGA_RES_HEIGHT);
   game->primaryView.texture = textureCreateCustom(EGA_RES_WIDTH, EGA_RES_HEIGHT, {RepeatType_CLAMP, FilterType_NEAREST}); 

   egaClear(game->primaryView.egaTexture, 0);

   _loadPalettes(game->assets);
   
}


Game* gameCreate(StringView assetsFolder) {
   auto out = new Game();
   _gameDataInit(&out->data, assetsFolder);
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

   _assetsDestroy(game->data.assets);

   delete game;
}