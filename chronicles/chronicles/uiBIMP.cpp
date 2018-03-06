#include "ui.h"
#include "game.h"
#include "app.h"
#include "chronwin.h"

#include <imgui.h>

#include "IconsFontAwesome.h"

#include "imgui_internal.h" // for checkerboard

#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_mouse.h>

#define MAX_IMG_DIM 0x100000

#define POPUPID_COLORPICKER "egapicker"

enum ToolStates_ {
   ToolStates_PENCIL = 0,
   ToolStates_LINES,
   ToolStates_RECT,
   ToolStates_FLOODFILL,
   ToolStates_EYEDROP,
   ToolStates_REGION_PICK,
   ToolStates_REGION_PICKED,
   ToolStates_REGION_DRAG,
};
typedef byte ToolStates;


struct BIMPState {

   Texture* pngTex = nullptr;
   EGATexture *ega = nullptr;
   EGAPalette encPal;
   char palName[64];

   ToolStates toolState =  ToolStates_PENCIL;
   EGAPColor popupCLickedColor = 0; // for color picker popup
   Float2 mousePos = { 0 }; //mouse positon within the image coords (updated per frame)
   bool mouseInImage = false; //helper boolean for every frame

   float zoomLevel = 1.0f;
   Int2 zoomOffset = {};
   Float2 windowSize = { 0 }; //set every frame
   Int2 enteredSize = { 32, 32 }; // for holding onto size selection

   bool egaStretch = false;
   ImVec4 bgColor;

   std::string winName;
   Int2 lastMouse = { 0 };

   EGAColor useColors[2] = { 0, 1 };


};

static std::string _genWinTitle(BIMPState *state) {
   return format("%s BIMP - Brandon's Image Manipulation Program###BIMP%p", ICON_FA_PAINT_BRUSH, state);
}

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

static void _fitToWindow(BIMPState &state) {
   auto texSize = textureGetSize(state.pngTex);

   auto rect = getProportionallyFitRect(texSize, { (i32)state.windowSize.x, (i32)state.windowSize.y });
   state.zoomLevel = rect.h / (float)texSize.y;
   state.zoomOffset = {0,0};
}

static void _loadPNG(BIMPState &state) {
   auto png = _getPng();
   if (!png.empty()) {
      
      _stateTexCleanup(state);
      state.pngTex = textureCreateFromPath(png.c_str(), { RepeatType_CLAMP, FilterType_NEAREST });      

      auto palName = pathGetFilename(png.c_str());
      strcpy(state.palName, palName.c_str());

      state.enteredSize = textureGetSize(state.pngTex);
      _fitToWindow(state);
   }
}

static void _colorButtonEGAStart(EGAColor c) {
   auto egac = egaGetColor(c);
   ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(egac.r, egac.g, egac.b, 255));
   ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(egac.r, egac.g, egac.b, 128));
   ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(egac.r, egac.g, egac.b, 255));
}
static void _colorButtonEGAEnd() {
   ImGui::PopStyleColor(3);
}

static bool _doColorSelectButton(BIMPState &state, u32 idx) {
   auto label = format("##select%d", idx);
   auto popLabel = format("%spopup", label.c_str());
   _colorButtonEGAStart(state.encPal.colors[state.useColors[idx]]);
   auto out = ImGui::Button(label.c_str(), ImVec2(32.0f, 32.0f));

   

   if (ImGui::BeginDragDropTarget()) {
      if (auto payload = ImGui::AcceptDragDropPayload(UI_DRAGDROP_PALCOLOR)) {
         auto plData = (uiDragDropPalColor*)payload->Data;
         if (plData->paletteIndex < EGA_PALETTE_COLORS) {
            state.useColors[idx] = plData->paletteIndex;
         }
      }
      ImGui::EndDragDropTarget();
   }
   _colorButtonEGAEnd();

   if (out) {
      ImGui::OpenPopup(popLabel.c_str());
   }

   if (ImGui::BeginPopup(popLabel.c_str(), ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1, 1));

      for (byte i = 0; i < 16; ++i) {
         ImGui::PushID(i);
         _colorButtonEGAStart(state.encPal.colors[i]);
         if (ImGui::Button("", ImVec2(20,20))) {
            state.useColors[idx] = i;
            ImGui::CloseCurrentPopup();
         }         
         _colorButtonEGAEnd();
         ImGui::PopID();

         ImGui::SameLine();
      }

      ImGui::PopStyleVar();
      ImGui::EndPopup();
   }

   return out;
}

static void _doToolbar(Window* wnd, BIMPState &state) {
   auto &imStyle = ImGui::GetStyle();

   if (ImGui::CollapsingHeader("Palette", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Indent();
      uiPaletteEditor(wnd, &state.encPal, state.palName, 64, PaletteEditorFlags_ENCODE);
      ImGui::Unindent();
   }
   if (ImGui::CollapsingHeader("Image", ImGuiTreeNodeFlags_DefaultOpen)) {

      ImGui::Indent();

      bool btnNew = ImGui::Button(ICON_FA_FILE " New"); ImGui::SameLine();
      bool btnLoad = ImGui::Button(ICON_FA_UPLOAD " Load"); ImGui::SameLine();
      if (!state.ega) { ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f); }
      bool btnSave = ImGui::Button(ICON_FA_DOWNLOAD " Save");       

      ImGui::Separator();

      for (u32 i = 0; i < 2; ++i) {
         _doColorSelectButton(state, i);
         ImGui::SameLine();
      }

      ImGui::NewLine();
      ImGui::Separator();

      ImGui::BeginGroup();     
      if (state.toolState == ToolStates_PENCIL) { ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)); }
      bool btnPencil = ImGui::Button(ICON_FA_PENCIL_ALT " Pencil");  
      if (state.toolState == ToolStates_PENCIL) { ImGui::PopStyleColor(); }
      if (state.toolState == ToolStates_FLOODFILL) { ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)); }
      bool btnFill = ImGui::Button(ICON_FA_PAINT_BRUSH " Flood Fill");
      if (state.toolState == ToolStates_FLOODFILL) { ImGui::PopStyleColor(); }
      bool btnErase = ImGui::Button(ICON_FA_ERASER " Eraser");
      if (state.toolState == ToolStates_EYEDROP) { ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)); }
      bool btnColorPicker = ImGui::Button(ICON_FA_EYE_DROPPER " Pick Color");
      if (state.toolState == ToolStates_EYEDROP) { ImGui::PopStyleColor(); }
      ImGui::EndGroup();

      ImGui::SameLine();

      ImGui::BeginGroup();
      if (state.toolState == ToolStates_LINES) { ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)); }
      bool btnLines = ImGui::Button(ICON_FA_STAR " Draw Lines");
      if (state.toolState == ToolStates_LINES) { ImGui::PopStyleColor(); }
      if (state.toolState == ToolStates_RECT) { ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)); }
      bool btnRect = ImGui::Button(ICON_FA_SQUARE " Draw Rect");
      if (state.toolState == ToolStates_RECT) { ImGui::PopStyleColor(); }
      bool regionState = state.toolState >= ToolStates_REGION_PICK; //TODO: This is bad
      if (regionState) { ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)); }
      bool btnRegion = ImGui::Button(ICON_FA_EXPAND " Region Select");
      if (regionState) { ImGui::PopStyleColor(); }
      bool btnCrop = ImGui::Button(ICON_FA_CROP " Crop");
      ImGui::EndGroup();

      ImGui::Separator();

      float spd = state.ega ? 1.0f : 0.0f;
      i32 size[2] = { 0 };
      memcpy(size, &state.enteredSize, sizeof(i32) * 2);
      ImGui::DragInt2("Size", size, spd, 0, MAX_IMG_DIM);
      if (memcmp(size, &state.enteredSize, sizeof(i32) * 2)) {
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
         ImGui::DragInt2("Size", (i32*)&state.enteredSize, 1, 0, MAX_IMG_DIM);
         if (ImGui::Button("Create!")) {
            _stateTexCleanup(state);
            state.pngTex = textureCreateCustom(state.enteredSize.x, state.enteredSize.y, { RepeatType_CLAMP, FilterType_NEAREST });
            state.ega = egaTextureCreate(state.enteredSize.x, state.enteredSize.y);
            egaClearAlpha(state.ega);
            _fitToWindow(state);
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

   // clicked actions
   if (ImGui::IsItemActive()) {
      // ctrl+dragging move image
      if (io.KeyCtrl && ImGui::IsMouseDragging()) {
         state.zoomOffset.x += (i32)ImGui::GetIO().MouseDelta.x;
         state.zoomOffset.y += (i32)ImGui::GetIO().MouseDelta.y;
      }


   }

   // hover actions
   if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlapped)) {

      // ctrl+wheel zooming
      if (io.KeyCtrl && fabs(io.MouseWheel) > 0.0f) {
         auto zoom = state.zoomLevel;
         state.zoomLevel = MIN(100, MAX(0.1f, state.zoomLevel + io.MouseWheel * 0.05f * state.zoomLevel));
         state.zoomOffset.x = -(i32)(state.mousePos.x * state.zoomLevel * pxSize.x - (io.MousePos.x - cursorPos.x));
         state.zoomOffset.y = -(i32)(state.mousePos.y * state.zoomLevel * pxSize.y - (io.MousePos.y - cursorPos.y));
      }

      if (ImGui::IsMouseDown(MOUSE_LEFT)) {
         if (state.ega) {
            Int2 m = { (i32)state.mousePos.x, (i32)state.mousePos.y };
            egaRenderLine(state.ega, state.lastMouse, m, state.useColors[0]);
            state.lastMouse = m;
            
         }
      }

      if (state.mouseInImage) {
         if (ImGui::IsMouseClicked(MOUSE_RIGHT)) {
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
         auto viewSize = ImGui::GetContentRegionAvail();
         state.windowSize = { viewSize.x, viewSize.y };

         if (state.pngTex) {            
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
