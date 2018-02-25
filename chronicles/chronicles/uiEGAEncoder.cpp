#include "ui.h"
#include "game.h"
#include "app.h"
#include "chronwin.h"

#include <imgui.h>

#include "IconsFontAwesome.h"

struct EncoderState {
   Texture* pngTex = nullptr;
};

static std::string _getPng() {
   OpenFileConfig cfg;
   cfg.filterNames = "PNG Image (*.png)";
   cfg.filterExtensions = "*.png";
   cfg.initialDir = cwd();

   return openFile(cfg);
}

bool _doUI(GameData* game, Window* wnd, EncoderState &state) {
   bool p_open = true;

   ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);

   if (ImGui::Begin("EGA Encoder", &p_open)) {
      auto loadBtn = ImGui::Button(ICON_FA_FOLDER_OPEN);
      if(ImGui::IsItemHovered()) ImGui::SetTooltip("Open PNG");

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

void uiToolStartEGAEncoder(GameData* game, Window* wnd) {
   EncoderState state;
   windowAddGUI(wnd, "EGA Encoder", [=](Window*wnd) mutable {
      return _doUI(game, wnd, state);
   });
}