#include "ui.h"
#include "game.h"
#include "app.h"
#include "chronwin.h"

#include <imgui.h>

#include "IconsFontAwesome.h"

#include "imgui_internal.h" // for checkerboard

#include "SDL2/SDL_scancode.h"

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

   std::string winName;
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

static void _doToolbar(Window* wnd, EncoderState &state) {
   ImGui::BeginGroup();

   if (ImGui::CollapsingHeader("Palette", ImGuiTreeNodeFlags_DefaultOpen)) {
      uiPaletteEditor(wnd, &state.encPal, state.palName, 64, PaletteEditorFlags_ENCODE);
   }
   if (ImGui::CollapsingHeader("Tools", ImGuiTreeNodeFlags_DefaultOpen)) {
      bool btnPencil = ImGui::Button(ICON_FA_PENCIL_ALT); ImGui::SameLine();
      bool btnErase = ImGui::Button(ICON_FA_ERASER); ImGui::SameLine();
      bool btnFill = ImGui::Button(ICON_FA_PAINT_BRUSH); ImGui::SameLine();

      ImGui::NewLine();

      bool btnRegion = ImGui::Button(ICON_FA_EXPAND); ImGui::SameLine();
      bool btnCrop = ImGui::Button(ICON_FA_CROP); ImGui::SameLine();
      bool btnRect = ImGui::Button(ICON_FA_SQUARE);
   }
   if (ImGui::CollapsingHeader("Encoding", ImGuiTreeNodeFlags_DefaultOpen)) {

      ImGui::Indent();
      bool btnOpen = ImGui::Button(ICON_FA_FOLDER_OPEN " Load PNG"); 

      if (!state.pngTex) { ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f); }

      bool btnClose = ImGui::Button(ICON_FA_TRASH_ALT " Close Texture");
      bool encode = ImGui::Button(ICON_FA_IMAGE " Encode!");

      if (!state.pngTex) { ImGui::PopStyleVar(); }

      ImGui::Unindent();

      if (btnOpen) {
         _loadPNG(state);
      }

      

      if (btnClose) {
         if (state.pngTex) {
            textureDestroy(state.pngTex);
            state.pngTex = nullptr;
         }
         if (state.ega) {
            egaTextureDestroy(state.ega);
            state.ega = nullptr;
         }
      } 

      if (encode && state.pngTex) {
         if (state.ega) {
            egaTextureDestroy(state.ega);
         }

         EGAPalette resultPal = { 0 };

         if (auto decoded = egaTextureCreateFromTextureEncode(state.pngTex, &state.encPal, &resultPal)) {
            state.ega = decoded;
            state.encPal = resultPal;

            egaTextureDecode(state.ega, state.pngTex, &state.encPal);

         }
         else {
            ImGui::OpenPopup("Encode Failed!");
         }
      }
      uiModalPopup("Encode Failed!", "Failed to finish encoding... does your palette have at least one color in it?");
   }
   if (ImGui::CollapsingHeader("Options", ImGuiTreeNodeFlags_DefaultOpen)) {

      ImGui::Indent();

      ImGui::Checkbox("EGA Stretch", &state.egaStretch);

      ImGui::ColorEdit4("Background Color", (float*)&state.bgColor,
         ImGuiColorEditFlags_NoInputs |
         ImGuiColorEditFlags_AlphaPreview | 
         ImGuiColorEditFlags_HSV |
         ImGuiColorEditFlags_AlphaBar |
         ImGuiColorEditFlags_PickerHueBar);

      ImGui::Unindent();
   }

   

   ImGui::EndGroup();
}

bool _doUI(Window* wnd, EncoderState &state) {
   auto game = gameGet();
   auto &imStyle = ImGui::GetStyle();
   bool p_open = true;

   ImGui::SetNextWindowSize(ImVec2(800, 800), ImGuiCond_Appearing);
   if (ImGui::Begin(state.winName.c_str(), &p_open, 0)) {
      ImGui::Columns(2, 0, false);
      ImGui::SetColumnWidth(0, uiPaletteEditorWidth() + imStyle.WindowPadding.x * 2);


      _doToolbar(wnd, state);

      ImGui::NextColumn();

      auto sz = ImGui::GetContentRegionAvail();
      auto palHeight = uiPaletteEditorHeight();

      auto y = ImGui::GetCursorPosY();

      

      if (ImGui::BeginChild("DrawArea", sz, true, ImGuiWindowFlags_NoScrollWithMouse)) {
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
            if (ImGui::BeginDragDropTarget()) {

               if (auto payload = ImGui::AcceptDragDropPayload(UI_DRAGDROP_PALCOLOR)) {
                  if (state.ega && mouseInImage) {
                     auto c = egaTextureGetColorAt(state.ega, (u32)mouseX, (u32)mouseY);
                     if (c < EGA_COLOR_UNDEFINED) {
                        auto plData = (uiDragDropPalColor*)payload->Data;
                        state.encPal.colors[c] = plData->color;
                     }
                  }
               }

               ImGui::EndDragDropTarget();
            }

            if (ImGui::IsItemActive()){
               if (io.KeyCtrl && ImGui::IsMouseDragging()) {
                  state.zoomOffset.x += ImGui::GetIO().MouseDelta.x;
                  state.zoomOffset.y += ImGui::GetIO().MouseDelta.y;
               }
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlapped)) {
               if (io.KeyCtrl && fabs(io.MouseWheel) > 0.0f) {
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

            ImGui::SetCursorPos(ImVec2(imStyle.WindowPadding.x, imStyle.WindowPadding.y + csz.y - ImGui::GetTextLineHeight() * 2 - imStyle.WindowPadding.y));
            if (mouseInImage) {    
               ImGui::Text(ICON_FA_MOUSE_POINTER); ImGui::SameLine(statsCol);
               ImGui::Text("Mouse: (%.1f, %.1f)", mouseX, mouseY);
            }
            else {
               ImGui::Text("");
            }


            ImGui::Text(ICON_FA_SEARCH); ImGui::SameLine(statsCol); 
            ImGui::Text("Scale: %.1f%%",state.zoomLevel * 100.0f); //ImGui::SameLine();

            if (state.ega) {
               uiPaletteColorPicker("egapicker", &state.encPal.colors[state.clickedColor]);
            }
         }
      }
      ImGui::EndChild();

      ImGui::Columns(1);
   }
   ImGui::End();

   return p_open;
}

static std::string _genWinTitle(EncoderState *state) {
   return format("%s EGAPaint###EGAPaint%p", ICON_FA_PAINT_BRUSH, state);
}

void uiToolStartEGAPaint( Window* wnd) {
   auto game = gameGet();
   EncoderState *state = new EncoderState();

   strcpy(state->palName, "default");
   paletteLoad(game->assets.palettes, state->palName, &state->encPal);

   state->winName = _genWinTitle(state);

   windowAddGUI(wnd, state->winName.c_str(), [=](Window*wnd) mutable {
      bool ret = _doUI(wnd, *state);
      if (!ret) {
         delete state;
      }
      return ret;
   });
}