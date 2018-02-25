#include "chronwin.h"
#include <nowide/convert.hpp>
#include <Windows.h>

std::string openFile(OpenFileConfig const& config) {

   OPENFILENAME ofn = { 0 };

   char out[MAX_PATH] = { 0 };

   char* filter = nullptr;
   if (!config.filterNames.empty()) {

      auto nSize = config.filterNames.size();
      auto eSize = config.filterExtensions.size();
      auto fullSize = nSize + eSize + 3;

      filter = new char[fullSize];
      memset(filter, 0, fullSize);

      memcpy(filter, config.filterNames.c_str(), nSize);
      memcpy(filter + nSize + 1, config.filterExtensions.c_str(), eSize);
   }

   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.lpstrFilter = filter;
   ofn.nFilterIndex = config.filterIndex;
   ofn.lpstrFile = out;
   ofn.nMaxFile = MAX_PATH;


   if (!config.initialDir.empty()) {
      ofn.lpstrInitialDir = config.initialDir.c_str();
   }

   if (!config.title.empty()) {
      ofn.lpstrTitle = config.title.c_str();
   }

   ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

   GetOpenFileName(&ofn);

   return out;
}

std::string cwd() {
   char out[MAX_PATH] = { 0 };
   GetCurrentDirectory(MAX_PATH, out);
   return out;
}

byte *readFullFile(StringView path, u64 *fsize) {
   byte *string;
   u64 fsizeBuffer = 0;
   FILE *f = fopen(path, "rb");

   if (!fsize) {
      fsize = &fsizeBuffer;
   }

   if (!f) {
      return nullptr;
   }

   fseek(f, 0, SEEK_END);
   *fsize = ftell(f);
   fseek(f, 0, SEEK_SET);

   string = new byte[*fsize + 1];
   fread(string, *fsize, 1, f);
   fclose(f);

   string[*fsize] = 0;

   return string;
}

int writeBinaryFile(StringView path, byte* buffer, u64 size) {
   auto fOut = fopen(path, "wb");
   if (!fOut) {
      return 0;
   }

   fwrite(buffer, sizeof(byte), size, fOut);
   fclose(fOut);
   return 1;
}