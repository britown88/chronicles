#include "ui.h"
#include "game.h"
#include "app.h"
#include "chronwin.h"

#include <imgui.h>

#include "IconsFontAwesome.h"

#include "imgui_internal.h" // for checkerboard

#include "SDL2/SDL_scancode.h"

#define POPUPID_COLORPICKER "egapicker"

struct BIMPState {
   Texture* pngTex = nullptr;
   EGATexture *ega = nullptr;
   EGAPalette encPal;
   char palName[64];

   EGAPColor popupCLickedColor = 0; // for color picker popup
   Float2 mousePos = { 0 }; //mouse positon within the image coords (updated per frame)
   bool mouseInImage = false; //helper boolean for every frame

   float zoomLevel = 1.0f;
   Int2 zoomOffset = {};

   bool egaStretch = false;
   ImVec4 bgColor;

   std::string winName;
   Int2 selectedSize = { 0 };
   Int2 lastMouse = { 0 };
};

static void _stateTexCleanup(BIMPState &state) {
   if (state.pngTex) {
      textureDestroy(state.pngTex);
      state.pngTex = nullptr;
   }

   if (state.ega) {
      egaTextureDestroy(state.ega);
      state.ega = nullptr;
   }
}

static std::string _getPng() {
   OpenFileConfig cfg;
   cfg.filterNames = "PNG Image (*.png)";
   cfg.filterExtensions = "*.png";
   cfg.initialDir = cwd();

   return openFile(cfg);
}

static void _loadPNG(BIMPState &state) {
   auto png = _getPng();
   if (!png.empty()) {
      
      _stateTexCleanup(state);
      state.pngTex = textureCreateFromPath(png.c_str(), { RepeatType_CLAMP, FilterType_NEAREST });      

      auto palName = pathGetFilename(png.c_str());
      strcpy(state.palName, palName.c_str());

      state.selectedSize = textureGetSize(state.pngTex);
   }
}

static void _doToolbar(Window* wnd, BIMPState &state) {
   auto &imStyle = ImGui::GetStyle();

   if (ImGui::CollapsingHeader("Palette", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Indent();
      uiPaletteEditor(wnd, &state.encPal, state.palName, 64, state.ega ? 0 : PaletteEditorFlags_ENCODE);
      ImGui::Unindent();
   }
   if (ImGui::CollapsingHeader("Image", ImGuiTreeNodeFlags_DefaultOpen)) {

      ImGui::Indent();

      bool btnNew = ImGui::Button(ICON_FA_FILE " New"); ImGui::SameLine();
      bool btnLoad = ImGui::Button(ICON_FA_UPLOAD " Load"); ImGui::SameLine();

      if (!state.ega) { ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f); }

      bool btnSave = ImGui::Button(ICON_FA_DOWNLOAD " Save");       

      ImGui::Separator();

      ImGui::BeginGroup();
      bool btnPencil = ImGui::Button(ICON_FA_PENCIL_ALT " Pencil");
      bool btnErase = ImGui::Button(ICON_FA_ERASER " Eraser");
      bool btnFill = ImGui::Button(ICON_FA_PAINT_BRUSH " Flood Fill");
      ImGui::EndGroup();
      ImGui::SameLine();
      ImGui::BeginGroup();
      bool btnRect = ImGui::Button(ICON_FA_SQUARE " Draw Rect");
      bool btnRegion = ImGui::Button(ICON_FA_EXPAND " Region Select");
      bool btnCrop = ImGui::Button(ICON_FA_CROP " Crop");
      ImGui::EndGroup();

      ImGui::Separator();

      float spd = state.ega ? 1.0f : 0.0f;
      i32 size[2] = { 0 };
      memcpy(size, &state.selectedSize, sizeof(i32) * 2);
      ImGui::DragInt2("Size", size, spd, 0, 0);
      if (memcmp(size, &state.selectedSize, sizeof(i32) * 2)) {
         if (state.pngTex) {
            // change happened, resize
            //memcpy(state.selectedSize, size, sizeof(int) * 2);
         }
      }

      if (!state.ega) { ImGui::PopStyleVar(); }

      ImGui::Unindent();

      //do button logic
      if (btnNew) {
         ImGui::OpenPopup("New Image Size");
      }
      if (ImGui::BeginPopupModal("New Image Size")) {
         ImGui::Dummy(ImVec2(200, 0));
         ImGui::DragInt2("Size", (i32*)&state.selectedSize, 1, 0, 0);
         if (ImGui::Button("Create!")) {
            _stateTexCleanup(state);
            state.pngTex = textureCreateCustom(state.selectedSize.x, state.selectedSize.y, { RepeatType_CLAMP, FilterType_NEAREST });
            state.ega = egaTextureCreate(state.selectedSize.x, state.selectedSize.y);
            egaClearAlpha(state.ega);

            ImGui::CloseCurrentPopup();
         }


         ImGui::EndPopup();
      }
      
   }
   if (ImGui::CollapsingHeader("Encoding", ImGuiTreeNodeFlags_DefaultOpen)) {

      ImGui::Indent();

      ImGui::BeginGroup();
      bool btnOpen = ImGui::Button(ICON_FA_FOLDER_OPEN " Load PNG"); 

      if (!state.pngTex) { ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f); }

      bool btnClose = ImGui::Button(ICON_FA_TRASH_ALT " Close Texture");

      ImGui::EndGroup();
      ImGui::SameLine();
      ImGui::BeginGroup();

      auto encodeBtnSize = ImVec2(
         0, 
         ImGui::GetFrameHeight() * 2 + imStyle.ItemSpacing.y);

      bool encode = ImGui::Button(ICON_FA_IMAGE " Encode!", encodeBtnSize);
      ImGui::EndGroup();

      if (!state.pngTex) { ImGui::PopStyleVar(); }

      ImGui::Unindent();

      if (btnOpen) {
         _loadPNG(state);
      }

      if (btnClose) {
         _stateTexCleanup(state);
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
}

// this function immediately follows the invisibile dummy button for the viewer
// all handling of mouse/keyboard for interactions with the viewer should go here!
static void _handleInput(BIMPState &state, Float2 pxSize, Float2 cursorPos) {
   auto &io = ImGui::GetIO();

   if (ImGui::BeginDragDropTarget()) {
      if (auto payload = ImGui::AcceptDragDropPayload(UI_DRAGDROP_PALCOLOR)) {
         if (state.ega && state.mouseInImage) {
            auto c = egaTextureGetColorAt(state.ega, (u32)state.mousePos.x, (u32)state.mousePos.y);
            if (c < EGA_COLOR_UNDEFINED) {
               auto plData = (uiDragDropPalColor*)payload->Data;
               state.encPal.colors[c] = plData->color;
            }
         }
      }
      ImGui::EndDragDropTarget();
   }

   if (ImGui::IsItemActive()) {
      if (io.KeyCtrl && ImGui::IsMouseDragging()) {
         state.zoomOffset.x += ImGui::GetIO().MouseDelta.x;
         state.zoomOffset.y += ImGui::GetIO().MouseDelta.y;
      }

      ImGui::Begin("asddfasd");
      ImGui::End();
   }
   if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlapped)) {
      if (io.KeyCtrl && fabs(io.MouseWheel) > 0.0f) {
         auto zoom = state.zoomLevel;
         state.zoomLevel = MIN(100, MAX(0.1f, state.zoomLevel + io.MouseWheel * 0.05f * state.zoomLevel));

         state.zoomOffset.x = -(state.mousePos.x * state.zoomLevel * pxSize.x - (io.MousePos.x - cursorPos.x));
         state.zoomOffset.y = -(state.mousePos.y * state.zoomLevel * pxSize.y - (io.MousePos.y - cursorPos.y));
      }

      if (ImGui::IsMouseDown(0)) {

         if (state.ega) {
            Int2 m = { (i32)state.mousePos.x, (i32)state.mousePos.y };
            egaRenderLine(state.ega, state.lastMouse, m, 0);
            state.lastMouse = m;
            //egaRenderPoint(state.ega, { (i32)mouseX, (i32)mouseY }, 0);
         }

      }

      if (state.mouseInImage) {
         if (ImGui::IsMouseClicked(1)) {
            if (state.ega) {
               auto c = egaTextureGetColorAt(state.ega, (u32)state.mousePos.x, (u32)state.mousePos.y);
               if (c < EGA_COLOR_UNDEFINED) {
                  state.popupCLickedColor = c;
                  ImGui::OpenPopup(POPUPID_COLORPICKER);
               }
            }
         }
      }
   }
}

static void _showStats(BIMPState &state, float viewHeight) {
   auto &imStyle = ImGui::GetStyle();

   static float statsCol = 32.0f;
   ImGui::SetCursorPos(ImVec2(imStyle.WindowPadding.x, imStyle.WindowPadding.y + viewHeight - ImGui::GetTextLineHeight() * 2 - imStyle.WindowPadding.y));

   if (state.mouseInImage) {
      ImGui::Text(ICON_FA_MOUSE_POINTER); ImGui::SameLine(statsCol);
      ImGui::Text("Mouse: (%.1f, %.1f)", state.mousePos.x, state.mousePos.y);
   }
   else {
      ImGui::Text("");
   }

   ImGui::Text(ICON_FA_SEARCH); ImGui::SameLine(statsCol);
   ImGui::Text("Scale: %.1f%%", state.zoomLevel * 100.0f); //ImGui::SameLine();
}

bool _doUI(Window* wnd, BIMPState &state) {
   auto game = gameGet();
   auto &imStyle = ImGui::GetStyle();
   bool p_open = true;

   ImGui::SetNextWindowSize(ImVec2(800, 800), ImGuiCond_Appearing);
   if (ImGui::Begin(state.winName.c_str(), &p_open, 0)) {
      auto availSize = ImGui::GetContentRegionAvail();
      if (ImGui::BeginChild("Toolbar", ImVec2(uiPaletteEditorWidth() + imStyle.IndentSpacing + imStyle.WindowPadding.x * 2, availSize.y))) {
         _doToolbar(wnd, state);
      }
      ImGui::EndChild();

      ImGui::SameLine();
      availSize = ImGui::GetContentRegionAvail();
      if (ImGui::BeginChild("DrawArea", availSize, true, ImGuiWindowFlags_NoScrollWithMouse)) {
         if (state.pngTex) {
            auto viewSize = ImGui::GetContentRegionAvail();
            auto texSize = textureGetSize(state.pngTex);
            float pxWidth = 1.0f;
            float pxHeight = 1.0f;

            if (state.egaStretch) {
               pxWidth = EGA_PIXEL_WIDTH;
               pxHeight = EGA_PIXEL_HEIGHT;
            }

            Float2 drawSize = { texSize.x * pxWidth, texSize.y * pxHeight };

            // this is a bit of a silly check.. imgui doesnt let us exceed custom draw calls past a certain point so 10k+
            // draw calls from this alpha grid is bad juju so imma try and limit it here
            auto pxSize = drawSize.x * drawSize.y;
            float checkerGridSize = 10;
            if (pxSize / (checkerGridSize * checkerGridSize) > 0xFFF) {
               checkerGridSize = sqrtf(pxSize / 0xFFF);
            }

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            const ImVec2 p = ImGui::GetCursorScreenPos();

            // before anything we want to udpate png with the current ega texture
            if (state.ega) {
               egaTextureDecode(state.ega, state.pngTex, &state.encPal);
            }

            // drawlists use screen coords
            auto a = ImVec2(state.zoomOffset.x + p.x, state.zoomOffset.y + p.y);
            auto b = ImVec2(state.zoomOffset.x + p.x  + drawSize.x*state.zoomLevel, state.zoomOffset.y + p.y + drawSize.y*state.zoomLevel);

            // get mouse position within the iamge
            auto &io = ImGui::GetIO();
            state.mousePos.x = (io.MousePos.x - p.x - state.zoomOffset.x) / state.zoomLevel / pxWidth;
            state.mousePos.y = (io.MousePos.y - p.y - state.zoomOffset.y) / state.zoomLevel / pxHeight;
            state.mouseInImage = 
               state.mousePos.x > 0.0f && state.mousePos.x < texSize.x && 
               state.mousePos.y > 0.0f && state.mousePos.y < texSize.y;

            //in our own clip rect we draw an alpha grid and a border            
            draw_list->PushClipRect(a, b, true);
            ImGui::RenderColorRectWithAlphaCheckerboard(a, b, ImGui::GetColorU32(state.bgColor), checkerGridSize * state.zoomLevel, ImVec2());
            draw_list->AddRect(a, b, IM_COL32(0, 0, 0, 255));
            draw_list->PopClipRect();

            //make a view-sized dummy button for capturing input
            ImGui::InvisibleButton("##dummy", viewSize);
            _handleInput(state, { pxWidth, pxHeight }, {p.x, p.y});

            // Draw the actual image
            draw_list->AddImage((ImTextureID)textureGetHandle(state.pngTex), a, b);

            // show stats in the lower left
            _showStats(state, viewSize.y);

            // the popup for colorpicker from pixel
            if (state.ega) {
               uiPaletteColorPicker(POPUPID_COLORPICKER, &state.encPal.colors[state.popupCLickedColor]);
            }
         }
      }
      ImGui::EndChild();

   }
   ImGui::End();

   return p_open;
}

static std::string _genWinTitle(BIMPState *state) {
   return format("%s BIMP - Brandon's Image Manipulation Program###BIMP%p", ICON_FA_PAINT_BRUSH, state);
}

void uiToolStartBIMP( Window* wnd) {
   auto game = gameGet();
   BIMPState *state = new BIMPState();

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