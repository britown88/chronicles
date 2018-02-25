#include "app.h"
#include "game.h"
#include "ega.h"

#include <imgui.h>
#include <SDL2/SDL.h>
#include "IconsFontAwesome.h"

#include "scf.h"
#include "ui.h"

static ImGuiWindowFlags BorderlessFlags =
      ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoScrollbar |
      ImGuiWindowFlags_NoScrollWithMouse |
      ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoSavedSettings;

static void _doStatsWindow(Window* wnd) {
   auto sz = windowSize(wnd);

   ImGui::SetNextWindowPos(ImVec2((float)sz.x, ImGui::GetFrameHeightWithSpacing()), ImGuiCond_Always, ImVec2(1, 0));
   if (ImGui::Begin("Stats", nullptr, BorderlessFlags | ImGuiWindowFlags_AlwaysAutoResize)) {
      auto txt = format("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      //auto txtSize = ImGui::CalcTextSize(txt.c_str());

      //ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - txtSize.x);
      ImGui::Text(txt.c_str());
   }
   ImGui::End();
}

struct SCFTestState {
   SCFWriter *writer = nullptr;
   char stringBuff[256] = { 0 };
   int i = 0;
   float f = 0.0;
};

struct SCFTestResultState {
   void* data;
   u32 size;
};

static void _uiDoSCFReader(SCFReader &view) {
   while (!scfReaderNull(view) && !scfReaderAtEnd(view)) {
      if (auto i = scfReadInt(view)) {
         ImGui::PushID(i);
         ImGui::Text("Int");
         ImGui::NextColumn();
         ImGui::Text("%d", *i);
         ImGui::NextColumn();
         ImGui::PopID();
      }
      else if (auto f = scfReadFloat(view)) {
         ImGui::PushID(f);
         ImGui::Text("Float");
         ImGui::NextColumn();
         ImGui::Text("%f", *f);
         ImGui::NextColumn();
         ImGui::PopID();
      }
      else if (auto str = scfReadString(view)) {
         ImGui::PushID(str);
         ImGui::Text("String");
         ImGui::NextColumn();
         ImGui::Text("%s", str);
         ImGui::NextColumn();
         ImGui::PopID();
      }
      else {
         SCFReader subreader = scfReadList(view);
         if (!scfReaderNull(subreader)) {
            ImGui::PushID(subreader.pos);
            if (ImGui::TreeNode("List")) {
               ImGui::NextColumn(); ImGui::NextColumn();
               _uiDoSCFReader(subreader);
               ImGui::TreePop();
            }
            else {
               ImGui::NextColumn(); ImGui::NextColumn();
            }
            ImGui::PopID();
            
         }
         else {
            scfReaderSkip(view);
         }
      }
   }
}

static bool _doSCFTest(Window* wnd, SCFTestState& state) {
   bool p_open = true;

   if (ImGui::Begin("SCF Testing", &p_open, 0)) {
      if (!state.writer) {
         if (ImGui::Button("Create a writer")) {
            state.writer = scfWriterCreate();
         }
      }

      else{
         ImGui::Columns(2);
         
         ImGui::InputInt("Int", &state.i);
         ImGui::NextColumn();
         if (ImGui::Button("Add##int")) {
            scfWriteInt(state.writer, state.i);
         }

         ImGui::NextColumn();
         ImGui::InputFloat("Float", &state.f);
         ImGui::NextColumn();
         if (ImGui::Button("Add##float")) {
            scfWriteFloat(state.writer, state.f);
         }

         ImGui::NextColumn();
         ImGui::InputText("String", state.stringBuff, 256);
         ImGui::NextColumn();
         if (ImGui::Button("Add##string")) {
            scfWriteString(state.writer, state.stringBuff);
         }

         ImGui::NextColumn(); ImGui::NextColumn();
         if (ImGui::Button("Start a new sublist")) {
            scfWriteListBegin(state.writer);
         }
         ImGui::NextColumn(); ImGui::NextColumn();
         if (ImGui::Button("End current sublist")) {
            scfWriteListEnd(state.writer);
         }

         ImGui::NextColumn();

         ImGui::Columns(1);

         DEBUG_imShowWriterStats(state.writer);

         if (ImGui::Button("Reset")) {
            scfWriterDestroy(state.writer);
            state.writer = nullptr;
         }

         if (ImGui::Button("Write and show results")) {
            SCFTestResultState result;
            result.data = scfWriteToBuffer(state.writer, &result.size);

            scfWriterDestroy(state.writer);
            state.writer = nullptr;

            auto lbl = format("SCF Result##%p", result.data);

            windowAddGUI(wnd, lbl.c_str(), [=](Window*wnd) mutable {
               bool p_open = true;

               ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
               if (ImGui::Begin(lbl.c_str(), &p_open, 0)) {
                  auto view = scfView(result.data);

                  ImGui::Columns(2);
                  _uiDoSCFReader(view);
                  ImGui::Columns(1);
                  
               }
               ImGui::End();

               if (!p_open) {
                  delete[] result.data;
               }

               return p_open;
            });
         }

         
      }


   }
   ImGui::End();

   if (!p_open && state.writer) {
      scfWriterDestroy(state.writer);
      state.writer = nullptr;
   }

   return p_open;
}



static void _mainMenu( Window* wnd) {
   auto game = gameGet();
   if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("Debug")) {

         ImGui::ColorEdit3("Clear Color", (float*)&game->imgui.bgClearColor);

         if (ImGui::MenuItem("Dialog Stats")) {
            windowAddGUI(wnd, "DialogStats", [=](Window*wnd) {

               bool p_open = true;
               if (ImGui::Begin("Dialog Stats", &p_open, ImGuiWindowFlags_AlwaysAutoResize)) {
                  ImGui::Text("Active Dialogs: %d", DEBUG_windowGetDialogCount(wnd));

                  if (ImGui::Button("Open a test dialog")) {
                     static int testest = 0;
                     auto label = format("Dialog Test##%d", testest++);

                     windowAddGUI(wnd, label.c_str(), [=](Window*wnd) {
                        bool p_open = true;
                        if (ImGui::Begin(label.c_str(), &p_open, ImGuiWindowFlags_AlwaysAutoResize)) {
                           ImGui::Text("Hi!");
                        }
                        ImGui::End();
                        return p_open;
                     });
                  }
               }
               ImGui::End();
               return p_open;
            });
         }
         if (ImGui::MenuItem("Goku")) {

            Texture *gokuTex = nullptr;

            windowAddGUI(wnd, "goku", [=](Window*wnd) mutable {
               bool p_open = true;

               if (!gokuTex) {
                  gokuTex = textureCreateFromPath("goku.png", { RepeatType_CLAMP, FilterType_LINEAR });
               }

               if (ImGui::Begin("goku", &p_open)) {
                  auto sz = ImGui::GetWindowContentRegionMax();
                  ImGui::Image((ImTextureID)textureGetHandle(gokuTex), ImVec2(sz.x, sz.y));
               }

               if (!p_open) {
                  textureDestroy(gokuTex);
               }

               ImGui::End();
               return p_open;
            });
         }
         if (ImGui::MenuItem("ImGui Demo")) {
            windowAddGUI(wnd, "imguidemo", [=](Window*wnd) mutable {
               bool p_open = true;
               ImGui::ShowDemoWindow(&p_open);
               return p_open;
            });
         }

         if (ImGui::MenuItem("SCF Testing")) {
            SCFTestState state;
            windowAddGUI(wnd, "SCF Testing", [=](Window*wnd) mutable {
               return _doSCFTest(wnd, state);
            });
         }


         ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Tools")) {
         if (ImGui::MenuItem("EGA Encoder")) {
            uiToolStartEGAEncoder(wnd);
         }

         ImGui::EndMenu();
      }

      ImGui::EndMainMenuBar();
   }
}



static void _renderViewerTexture(Texture* texture, Int2 srcSize) {
   if (!texture) {
      return;
   }

   auto sz = ImGui::GetContentRegionAvail();
   auto texSz = textureGetSize(texture);

   auto rect = getProportionallyFitRect(srcSize, { (i32)sz.x, (i32)sz.y });

   ImDrawList* draw_list = ImGui::GetWindowDrawList();
   const ImVec2 p = ImGui::GetCursorScreenPos();

   draw_list->AddImage(
      (ImTextureID)textureGetHandle(texture), 
      ImVec2(p.x + rect.x, p.y + rect.y), //a
      ImVec2(p.x + rect.x + rect.w, p.y + rect.y + rect.h)
      );
}

static void _showFullScreenViewer(Window* wnd) {
   auto game = gameGet();
   auto sz = windowSize(wnd);

   auto &style = ImGui::GetStyle();

   ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
   ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
   ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

   ImGui::SetNextWindowPos(ImVec2(), ImGuiCond_Always);
   ImGui::SetNextWindowSize(ImVec2((float)sz.x, (float)sz.y), ImGuiCond_Always);

   if (ImGui::Begin("GameWindow", nullptr, BorderlessFlags)) {
      Int2 sz = { (i32)(EGA_RES_WIDTH * EGA_PIXEL_WIDTH), (i32)(EGA_RES_HEIGHT * EGA_PIXEL_HEIGHT) };
      _renderViewerTexture(game->primaryView.texture, sz);
   }
   ImGui::End();

   ImGui::PopStyleVar(3);
}

static bool _imWindowContextMenu(const char* str_id, int mouse_button) {
   if (ImGui::IsMouseReleased(mouse_button) && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup)) {
      ImGui::OpenPopup(str_id);
   }      
   return ImGui::BeginPopup(str_id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
}

float uiPaletteEditorWidth() {
   auto &imStyle = ImGui::GetStyle();
   auto lnHeight = ImGui::GetTextLineHeight();
   float btnSize = lnHeight + imStyle.FramePadding.y;
   return btnSize * 16 + imStyle.WindowPadding.x * 2;
}

void uiPaletteEditor(Window* wnd, EGAPalette* pal, char* palName, u32 palNameSize, PaletteEditorFlags flags) {
   auto game = gameGet();
   auto &imStyle = ImGui::GetStyle();

   auto lnHeight = ImGui::GetTextLineHeight();
   ImVec2 size;

   float btnSize = lnHeight + imStyle.FramePadding.y;

   if (flags&PaletteEditorFlags_POPPED_OUT) {
      btnSize = MAX(btnSize, ImGui::GetContentRegionAvailWidth() / 16);
   }

   size.y = btnSize + ImGui::GetFrameHeightWithSpacing() + ImGui::GetTextLineHeightWithSpacing() + imStyle.WindowPadding.y * 2;
   size.x = btnSize * 16 + imStyle.WindowPadding.x * 2;

   bool doUi = true;

   if (!(flags&PaletteEditorFlags_POPPED_OUT)) {
      doUi = ImGui::BeginChild("PaletteEditor", size, true);
   }

   if (doUi) {

      if (!(flags&PaletteEditorFlags_POPPED_OUT)) {
         ImGui::Text("Palette Editor");
      }

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, imStyle.ItemSpacing.y));
      for (int i = 0; i < EGA_PALETTE_COLORS; ++i) {
         auto pcolor = egaGetColor(pal->colors[i]);

         ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(pcolor.r, pcolor.g, pcolor.b, 255));
         ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(pcolor.r, pcolor.g, pcolor.b, 128));

         ImGui::PushID(i);
         if (ImGui::Button("", ImVec2(btnSize, btnSize))) {
            ImGui::OpenPopup("EGAColorPicker");
         }
         if (ImGui::IsItemHovered()) ImGui::SetTooltip("Idx: %d\nCol: %d", i, pal->colors[i]);

         if (ImGui::BeginPopup("EGAColorPicker")) {

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
            for (int y = 0; y < 8; ++y) {
               for (int x = 0; x < 8; ++x) {
                  byte c = y * 8 + x;
                  auto egac = egaGetColor(c);

                  ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(egac.r, egac.g, egac.b, 255));
                  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(egac.r, egac.g, egac.b, 128));

                  ImGui::PushID(c);
                  if (ImGui::Button("", ImVec2(20, 20))) {
                     ImGui::CloseCurrentPopup();
                  }
                  if (ImGui::IsItemHovered()) {
                     pal->colors[i] = c;
                  }

                  ImGui::PopID();
                  ImGui::SameLine();

                  ImGui::PopStyleColor(2);
               }
               ImGui::NewLine();
            }
            ImGui::PopStyleVar();

            ImGui::Text("Color: %d", pal->colors[i]);
            ImGui::EndPopup();

         }

         ImGui::PopID();
         ImGui::PopStyleColor(2);
         ImGui::SameLine();
      }
      ImGui::NewLine();
      ImGui::PopStyleVar();

      if (!palName) {
         //not using a pre-allocated palName buffer so use window store
         // I guess this is fine that it leaks?? shrug
         auto imStore = ImGui::GetStateStorage();

         auto storedPalName = imStore->GetVoidPtrRef(0);
         auto storedPalSize = imStore->GetIntRef(1);

         if (!*storedPalName) {
            *storedPalName = new char[128];
            memset(*storedPalName, 0, 128);
            *storedPalSize = 128;
         }

         palName = (char*)*storedPalName;
         palNameSize = *storedPalSize;
      }

      ImGui::PushItemWidth(btnSize * 8);
      ImGui::InputText("##Name", palName, palNameSize);
      ImGui::PopItemWidth();

      auto currentNameLen = strlen(palName);
      if (currentNameLen == 0) {
         ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha*0.5f);
      }

      ImGui::SameLine();
      auto btnSave = ImGui::Button(ICON_FA_DOWNLOAD);
      if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save");
      if (btnSave && currentNameLen != 0) {
         paletteSave(game->assets.palettes, palName, pal);
      }

      if (currentNameLen == 0) {
         ImGui::PopStyleVar();
      }

      ImGui::SameLine();
      auto btnLoad = ImGui::Button(ICON_FA_UPLOAD);
      if (ImGui::IsItemHovered()) ImGui::SetTooltip("Load");

      auto storage = ImGui::GetStateStorage();
      bool *loadOpened = storage->GetBoolRef(2, false);

      if (btnLoad) {
         ImGui::OpenPopup("Load Palette");
         *loadOpened = true;
      }
      if (ImGui::BeginPopup("Load Palette")) {
         static char search[64] = { 0 };

         if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
         }
         ImGui::InputText(ICON_FA_SEARCH, search, 64);

         auto pals = paletteList(game->assets.palettes, search);

         if (ImGui::BeginChild("List", ImVec2(ImGui::GetContentRegionAvailWidth(), ImGui::GetFrameHeightWithSpacing() * 5), true)) {

            if (pals.empty()) {
               ImGui::Text("No Palettes!");
            }
            else {
               auto &imStyle = ImGui::GetStyle();
               auto imBtnAlign = imStyle.ButtonTextAlign;
               for (auto p : pals) {
                  ImGui::PushID(p.c_str());

                  bool btnDelete = ImGui::Button(ICON_FA_TRASH_ALT);
                  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delete");
                  if (btnDelete) {
                     paletteDelete(game->assets.palettes, p.c_str());
                     ImGui::PopID();
                     break;
                  }

                  imStyle.ButtonTextAlign = ImVec2(0.f, 0.5f);
                  ImGui::SameLine();
                  bool clicked = ImGui::Button(p.c_str(), ImVec2(ImGui::GetContentRegionAvailWidth(), 0));
                  if (ImGui::IsItemHovered()) {
                     paletteLoad(game->assets.palettes, p.c_str(), pal);
                  }
                  if (clicked) {
                     paletteLoad(game->assets.palettes, p.c_str(), pal);
                     strcpy(palName, p.c_str());
                     ImGui::CloseCurrentPopup();
                  }
                  imStyle.ButtonTextAlign = imBtnAlign;
                  ImGui::PopID();
               }
            }

         }
         ImGui::EndChild();
         ImGui::EndPopup();
      }
      else {
         if (*loadOpened) {
            *loadOpened = false;
            paletteLoad(game->assets.palettes, palName, pal);
         }
      }
      ImGui::SameLine();
      auto btnRefresh = ImGui::Button(ICON_FA_SYNC_ALT);
      if (ImGui::IsItemHovered()) ImGui::SetTooltip("Refresh");
      if (btnRefresh) {
         paletteLoad(game->assets.palettes, palName, pal);
      }

      if (!(flags&PaletteEditorFlags_POPPED_OUT)) {
         ImGui::SameLine();
         auto btnPopOut = ImGui::Button(ICON_FA_SIGN_OUT_ALT);
         if (ImGui::IsItemHovered()) ImGui::SetTooltip("Pop Out");
         if (btnPopOut) {
            auto label = format("Palette Editor##%p%p%d", pal, palName, palNameSize);
            windowAddGUI(wnd, label.c_str(), [=](Window*wnd) {
               bool p_open = true;
               ImGui::SetWindowSize(size, ImGuiCond_Appearing);
               ImGui::SetNextWindowPosCenter(ImGuiCond_FirstUseEver);
               if (ImGui::Begin(label.c_str(), &p_open)) {
                  uiPaletteEditor(wnd, pal, palName, palNameSize, flags | PaletteEditorFlags_POPPED_OUT);
               }
               ImGui::End();
               return p_open;
            });
         }
      }
   }

   if (!(flags&PaletteEditorFlags_POPPED_OUT)) {
      ImGui::EndChild();
   }
}


static void _showWindowedViewer(Window* wnd) {
   auto game = gameGet();

   auto sz = windowSize(wnd);
   ImGui::SetNextWindowSize(ImVec2(sz.x/2.0f, sz.y/2.0f), ImGuiCond_Appearing);
   static Int2 viewersz = { (i32)(EGA_RES_WIDTH * EGA_PIXEL_WIDTH), (i32)(EGA_RES_HEIGHT * EGA_PIXEL_HEIGHT) };

   if (ImGui::Begin("Viewer", nullptr, 0)) {

      ImGui::Columns(2);

      if (ImGui::IsWindowAppearing()) {
         auto &imStyle = ImGui::GetStyle();
         ImGui::SetColumnWidth(0, uiPaletteEditorWidth() + imStyle.WindowPadding.x * 2);
      }
      
     

      uiPaletteEditor(wnd, &game->primaryView.palette);

      ImGui::NextColumn();

      _renderViewerTexture(game->primaryView.texture, viewersz);   

      if (_imWindowContextMenu("Viewer Context", 1)) {
         if (ImGui::Selectable("Edit Size")) {
            windowAddGUI(wnd, "viewerContextResize", [&](Window*wnd) mutable {
               bool p_open = true;
               ImGui::OpenPopup("Change Viewer Size");
               if (ImGui::BeginPopupModal("Change Viewer Size", NULL, ImGuiWindowFlags_AlwaysAutoResize))  {
                  ImGui::DragInt("Width", &viewersz.x);
                  ImGui::DragInt("Height", &viewersz.y);

                  if (ImGui::Button("OK")) {
                     ImGui::CloseCurrentPopup();
                     p_open = false;
                  }
                  ImGui::EndPopup();
               }
               return p_open;
            });
         }

         ImGui::EndPopup();
      }
   }
   ImGui::End();

}


void gameDoUI(Window* wnd) {
   auto game = gameGet();

   if (ImGui::IsKeyPressed(SDL_SCANCODE_F1)) {
      game->imgui.showUI = !game->imgui.showUI;
   }

   if (game->imgui.showUI) {
      _mainMenu(wnd);
      _doStatsWindow(wnd);
      _showWindowedViewer(wnd);
   }
   else {
      _showFullScreenViewer(wnd);
   }
   
}

