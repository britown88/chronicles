#include "ui.h"
#include "game.h"
#include "app.h"

#include <imgui.h>

#include "IconsFontAwesome.h"

struct EncoderState {

};

bool _doUI(GameData* game, Window* wnd, EncoderState &state) {
   bool p_open = true;

   ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);

   if (ImGui::Begin("EGA Encoder", &p_open)) {
      auto loadBtn = ImGui::Button(ICON_FA_FOLDER_OPEN);
      if(ImGui::IsItemHovered()) ImGui::SetTooltip("Open PNG");

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