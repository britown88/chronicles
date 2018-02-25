#include "ui.h"
#include "game.h"
#include "app.h"
#include "chronwin.h"

#include <imgui.h>

#include "IconsFontAwesome.h"

struct EncoderState {
   Texture* pngTex = nullptr;
   EGAPalette encPal;
   char palName[64];
};

static std::string _getPng() {
   OpenFileConfig cfg;
   cfg.filterNames = "PNG Image (*.png)";
   cfg.filterExtensions = "*.png";
   cfg.initialDir = cwd();

   return openFile(cfg);
}

bool _doUI(Window* wnd, EncoderState &state) {
   auto game = gameGet();
   bool p_open = true;

   ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);

   if (ImGui::Begin("EGA Encoder", &p_open)) {
      auto loadBtn = ImGui::Button(ICON_FA_FOLDER_OPEN);
      if(ImGui::IsItemHovered()) ImGui::SetTooltip("Open PNG");

      if (state.pngTex) {
         ImGui::SameLine();
         auto closeBtn = ImGui::Button(ICON_FA_TRASH_ALT);
         if (ImGui::IsItemHovered()) ImGui::SetTooltip("Close PNG");

         if (closeBtn) {
            if (state.pngTex) {
               textureDestroy(state.pngTex);
               state.pngTex = nullptr;
            }
         }
      }
      
      uiPaletteEditor(wnd, &state.encPal, state.palName, 64, PaletteEditorFlags_ENCODE);

      if (loadBtn) {
         auto png = _getPng();
         if (!png.empty()) {
            if (state.pngTex) {
               textureDestroy(state.pngTex);
            }

            state.pngTex = textureCreateFromPath(png.c_str(), { RepeatType_CLAMP, FilterType_NEAREST });
         }
      }

      auto sz = ImGui::GetContentRegionAvail();
      if (state.pngTex) {
         auto rect = getProportionallyFitRect(textureGetSize(state.pngTex), { (i32)sz.x, (i32)sz.y });

         ImDrawList* draw_list = ImGui::GetWindowDrawList();
         const ImVec2 p = ImGui::GetCursorScreenPos();

         draw_list->AddImage(
            (ImTextureID)textureGetHandle(state.pngTex),
            ImVec2(p.x + rect.x, p.y + rect.y), //a
            ImVec2(p.x + rect.x + rect.w, p.y + rect.y + rect.h)
         );
      }

   }
   ImGui::End();

   return p_open;
}

void uiToolStartEGAEncoder( Window* wnd) {
   auto game = gameGet();
   EncoderState state;

   strcpy(state.palName, "default");
   paletteLoad(game->assets.palettes, state.palName, &state.encPal);

   windowAddGUI(wnd, "EGA Encoder", [=](Window*wnd) mutable {
      return _doUI(wnd, state);
   });
}