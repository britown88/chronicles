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
   EGAPColor clickedColor = 0;

   float zoomLevel = 1.0f;
   Int2 zoomOffset = {};

   bool egaStretch = false;
   ImVec4 bgColor;
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
            float pxWidth = 1.0f;
            float pxHeight = 1.0f;

            if (state.egaStretch) {
               pxWidth = EGA_PIXEL_WIDTH;
               pxHeight = EGA_PIXEL_HEIGHT;
            }

            Float2 drawSize = { texSize.x * pxWidth, texSize.y * pxHeight };

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            const ImVec2 p = ImGui::GetCursorScreenPos();

            if (state.ega) {
               egaTextureDecode(state.ega, state.pngTex, &state.encPal);
            }

            auto &io = ImGui::GetIO();

            auto a = ImVec2(state.zoomOffset.x + p.x, state.zoomOffset.y + p.y);
            auto b = ImVec2(state.zoomOffset.x + p.x  + drawSize.x*state.zoomLevel, state.zoomOffset.y + p.y + drawSize.y*state.zoomLevel);

            float mouseX = (io.MousePos.x - p.x - state.zoomOffset.x) / state.zoomLevel / pxWidth;
            float mouseY = (io.MousePos.y - p.y - state.zoomOffset.y) / state.zoomLevel / pxHeight;

            bool mouseInImage = mouseX > 0.0f && mouseX < texSize.x && mouseY > 0.0f && mouseY < texSize.y;
            
            draw_list->PushClipRect(a, b, true);
            draw_list->AddRect(a, b, IM_COL32(0, 0, 0, 255));

            auto bgCol = ImGui::GetColorU32(state.bgColor);

            ImGui::RenderColorRectWithAlphaCheckerboard(a, b, bgCol, 10 * state.zoomLevel, ImVec2());
            draw_list->PopClipRect();
            //draw_list->AddRectFilled(a, b, IM_COL32_BLACK_TRANS);

            
            auto sz = ImVec2(drawSize.x*state.zoomLevel, drawSize.y*state.zoomLevel);
            //ImGui::Image((ImTextureID)textureGetHandle(state.pngTex), sz);

            ImGui::InvisibleButton("##dummy", csz);
            if (ImGui::IsItemActive()){
               if (ImGui::IsMouseDragging()) {
                  state.zoomOffset.x += ImGui::GetIO().MouseDelta.x;
                  state.zoomOffset.y += ImGui::GetIO().MouseDelta.y;
               }
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlapped)) {
               if (fabs(io.MouseWheel) > 0.0f) {
                  auto zoom = state.zoomLevel;
                  state.zoomLevel = MIN(100, MAX(0.1f, state.zoomLevel + io.MouseWheel * 0.05f * state.zoomLevel));

                  state.zoomOffset.x = -(mouseX * state.zoomLevel * pxWidth - (io.MousePos.x - p.x));
                  state.zoomOffset.y = -(mouseY * state.zoomLevel * pxHeight - (io.MousePos.y - p.y));
               }

               if (mouseInImage) {
                  if (ImGui::IsMouseClicked(1)) {
                     if (state.ega) {
                        auto c = egaTextureGetColorAt(state.ega, (u32)mouseX, (u32)mouseY);
                        if (c < EGA_COLOR_UNDEFINED) {
                           state.clickedColor = c;
                           ImGui::OpenPopup("egapicker");
                        }
                     }
                  }
               }
            }

            draw_list->AddImage((ImTextureID)textureGetHandle(state.pngTex), a, b);

            auto &imStyle = ImGui::GetStyle();
            static float statsCol = 32.0f;

            ImGui::SetCursorPos(imStyle.WindowPadding);
            ImGui::Text(ICON_FA_SEARCH); ImGui::SameLine(statsCol);
            ImGui::Text("Scale: %.1f%%",state.zoomLevel * 100.0f);

            if (mouseInImage) {    
               ImGui::Text(ICON_FA_MOUSE_POINTER); ImGui::SameLine(statsCol);
               ImGui::Text("Mouse: (%.1f, %.1f)", mouseX, mouseY);

               if (state.ega) {
                  auto c = egaTextureGetColorAt(state.ega, (u32)mouseX, (u32)mouseY);
                  if (c < EGA_COLOR_UNDEFINED) {
                     ImGui::Text(ICON_FA_PAINT_BRUSH); ImGui::SameLine(statsCol);
                     ImGui::Text("Color: %d (Palette %d)", state.encPal.colors[c], c);
                  }
               }
            }

            if (state.ega) {
               uiPaletteColorPicker("egapicker", &state.encPal.colors[state.clickedColor]);
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

      ImGui::SameLine();
      ImGui::BeginGroup();

      ImGui::Text("Options");
      ImGui::Checkbox("EGA Stretch", &state.egaStretch);

      ImGui::ColorEdit4("BG", (float*)&state.bgColor,
         ImGuiColorEditFlags_NoInputs |
         ImGuiColorEditFlags_AlphaPreview | 
         ImGuiColorEditFlags_HSV |
         ImGuiColorEditFlags_AlphaBar |
         ImGuiColorEditFlags_PickerHueBar);

      ImGui::EndGroup();      
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