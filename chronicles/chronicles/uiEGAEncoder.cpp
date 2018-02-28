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

   float zoomLevel = 1.0f;
   Int2 zoomOffset = {};
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
            auto texSize = textureGetSize(state.pngTex);
            auto rect = Recti{ 0,0, texSize.x, texSize.y }; //getProportionallyFitRect(textureGetSize(state.pngTex), { (i32)csz.x, (i32)csz.y });
            rect.x = 0;

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            const ImVec2 p = ImGui::GetCursorScreenPos();

            if (state.ega) {
               egaTextureDecode(state.ega, state.pngTex, &state.encPal);
            }

            auto a = ImVec2(state.zoomOffset.x + p.x + rect.x, state.zoomOffset.y + p.y + rect.y);
            auto b = ImVec2(state.zoomOffset.x + p.x + rect.x + rect.w*state.zoomLevel, state.zoomOffset.y + p.y + rect.y + rect.h*state.zoomLevel);

            
            draw_list->PushClipRect(a, b, true);
            draw_list->AddRect(a, b, IM_COL32(0, 0, 0, 255));
            ImGui::RenderColorRectWithAlphaCheckerboard(a, b, IM_COL32_BLACK_TRANS, 10 * state.zoomLevel, ImVec2());
            draw_list->PopClipRect();
            //draw_list->AddRectFilled(a, b, IM_COL32_BLACK_TRANS);

            
            auto sz = ImVec2(rect.w*state.zoomLevel, rect.h*state.zoomLevel);
            //ImGui::Image((ImTextureID)textureGetHandle(state.pngTex), sz);

            ImGui::InvisibleButton("##dummy", csz);
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging()) { 
               state.zoomOffset.x += ImGui::GetIO().MouseDelta.x; 
               state.zoomOffset.y += ImGui::GetIO().MouseDelta.y;
            }


            draw_list->AddImage((ImTextureID)textureGetHandle(state.pngTex), a, b);

            auto &io = ImGui::GetIO();
            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) { 

               if (fabs(io.MouseWheel) > 0.0f) {
                  state.zoomLevel = MIN(100, MAX(0.1f, state.zoomLevel + io.MouseWheel * 0.05f * state.zoomLevel));

               }
            }
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