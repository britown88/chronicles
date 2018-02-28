#include "ui.h"
#include "game.h"
#include "app.h"
#include "chronwin.h"

#include <imgui.h>

#include "IconsFontAwesome.h"

#include "imgui_internal.h" // for checkerboard

struct EncoderState {
   Texture* pngTex = nullptr;
   EGATexture *ega = nullptr;
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

static void _loadPNG(EncoderState &state) {
   auto png = _getPng();
   if (!png.empty()) {
      if (state.pngTex) {
         textureDestroy(state.pngTex);
      }
      state.pngTex = textureCreateFromPath(png.c_str(), { RepeatType_CLAMP, FilterType_NEAREST });
      if (state.ega) {
         egaTextureDestroy(state.ega);
         state.ega = nullptr;
      }

      auto palName = pathGetFilename(png.c_str());
      strcpy(state.palName, palName.c_str());
   }
}

bool _doUI(Window* wnd, EncoderState &state) {
   auto game = gameGet();
   bool p_open = true;

   ImGui::SetNextWindowSize(ImVec2(500, 800), ImGuiCond_Appearing);
   if (ImGui::Begin("EGA Encoder", &p_open, ImGuiWindowFlags_MenuBar)) {
      if (ImGui::BeginMenuBar()) {

         if (ImGui::BeginMenu("File")) {

            if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Load PNG")) {
               _loadPNG(state);
            }

            if (state.pngTex) {
               if (ImGui::MenuItem(ICON_FA_TRASH_ALT " Close Texture")) {
                  if (state.pngTex) {
                     textureDestroy(state.pngTex);
                     state.pngTex = nullptr;
                  }

                  if (state.ega) {
                     egaTextureDestroy(state.ega);
                     state.ega = nullptr;
                  }
               }
            }

            ImGui::EndMenu();
         }

         ImGui::EndMenuBar();
      }

      if (ImGui::IsWindowAppearing()) {
         _loadPNG(state);
      }

      auto sz = ImGui::GetContentRegionAvail();
      auto palHeight = uiPaletteEditorHeight();

      auto y = ImGui::GetCursorPosY();

      if (ImGui::BeginChild("DrawArea", ImVec2(sz.x, sz.y - palHeight), true)) {
         if (state.pngTex) {
            auto csz = ImGui::GetContentRegionAvail();
            auto rect = getProportionallyFitRect(textureGetSize(state.pngTex), { (i32)csz.x, (i32)csz.y });
            rect.x = 0;

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            const ImVec2 p = ImGui::GetCursorScreenPos();

            if (state.ega) {
               egaTextureDecode(state.ega, state.pngTex, &state.encPal);
            }

            auto a = ImVec2(p.x + rect.x, p.y + rect.y);
            auto b = ImVec2(p.x + rect.x + rect.w, p.y + rect.y + rect.h);

            draw_list->AddRect(a, b, IM_COL32(0, 0, 0, 255));
            //draw_list->AddRectFilled(a, b, IM_COL32_BLACK_TRANS);

            ImGui::RenderColorRectWithAlphaCheckerboard(a, b, IM_COL32_BLACK_TRANS, 10, ImVec2());
            draw_list->AddImage((ImTextureID)textureGetHandle(state.pngTex), a, b);

            
         }

         ImGui::EndChild();
      }
      
      auto &imStyle = ImGui::GetStyle();

      ImGui::SetCursorPosY(y + sz.y - palHeight);
      uiPaletteEditor(wnd, &state.encPal, state.palName, 64, PaletteEditorFlags_ENCODE);

      if (state.pngTex) {
         ImGui::SameLine(uiPaletteEditorWidth() + imStyle.ItemSpacing.x * 2);
         if (ImGui::Button("Encode!", ImVec2(palHeight, palHeight))) {
            if (state.ega) {
               egaTextureDestroy(state.ega);
            }

            EGAPalette resultPal = { 0 };
            state.ega = egaTextureCreateFromTextureEncode(state.pngTex, &state.encPal, &resultPal);
            state.encPal = resultPal;

            egaTextureDecode(state.ega, state.pngTex, &state.encPal);
         }
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