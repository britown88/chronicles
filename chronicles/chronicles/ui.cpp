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

         ImGui::ColorEdit4("Clear Color", (float*)&game->imgui.bgClearColor);

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
         if (ImGui::MenuItem(ICON_FA_PAINT_BRUSH" BIMP")) {
            uiToolStartBIMP(wnd);
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

float uiPaletteEditorHeight() {
   auto &imStyle = ImGui::GetStyle();
   auto lnHeight = ImGui::GetTextLineHeight();
   float btnSize = lnHeight + imStyle.FramePadding.y;
   return btnSize + ImGui::GetFrameHeightWithSpacing() * 2 + imStyle.WindowPadding.y * 2;
}

#include <algorithm>

static u32 _8x8indexFromHSV(ColorHSV const& hsv) {
   auto hrad = hsv.h * 0.0174533f;
   float x = cosf(hrad) / 4.0f;
   float y = sinf(hrad) / 4.0f;

   x *= hsv.s;
   y *= hsv.s;

   byte gridX = (byte)(4.0f + x);
   byte gridY = (byte)(4.0f + y);

   return gridY * 8 + gridX;
}

void uiPaletteColorPicker(StringView label, EGAColor *color) {

   //struct EGAHSV{
   //   ColorHSV hsv = { 0 };
   //   EGAColor ega = 0;
   //};

   //std::vector<EGAHSV> colors(64);
   //for (EGAColor i = 0; i < 64; ++i) {
   //   colors[i].ega = i;
   //   colors[i].hsv = srgbToHSV(egaGetColor(i));
   //}

   //std::sort(colors.begin(), colors.end(), [](EGAHSV const&a, EGAHSV const&b) {
   //   return _8x8indexFromHSV(a.hsv) > _8x8indexFromHSV(b.hsv);
   //});


   if (ImGui::BeginPopup(label)) {
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
            if (ImGui::BeginDragDropSource(0, 0)) {
               uiDragDropPalColor payload = {c, EGA_PALETTE_COLORS};
               ImGui::SetDragDropPayload(UI_DRAGDROP_PALCOLOR, &payload, sizeof(payload), ImGuiCond_Once);
               ImGui::Button("", ImVec2(20, 20));
               ImGui::EndDragDropSource();
            }
            if (ImGui::IsItemHovered()) {
               *color = c;
            }

            ImGui::PopID();
            ImGui::SameLine();

            ImGui::PopStyleColor(2);
         }
         ImGui::NewLine();
      }
      ImGui::PopStyleVar();

      //ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, imItemSpacing);
      //ImGui::PushStyleColor(ImGuiCol_Button, imButtonColor);
      //ImGui::PushStyleColor(ImGuiCol_ButtonHovered, imButtonHoveredColor);

      //ImGui::SetCursorPosY(ImGui::GetCursorPosY() + imItemSpacing.y);

      //if (flags&PaletteEditorFlags_ENCODE) {
      //   auto btnAny = ImGui::Button(ICON_FA_QUESTION);
      //   if (ImGui::IsItemHovered()) ImGui::SetTooltip("Allow Encoding to use\nany color here");

      //   ImGui::SameLine();
      //   auto btnUnused = ImGui::Button(ICON_FA_TIMES_CIRCLE);
      //   if (ImGui::IsItemHovered()) ImGui::SetTooltip("Block Encoding from\nusing this slot");

      //   if (btnAny) {
      //      pal->colors[i] = EGA_COLOR_UNDEFINED;
      //      ImGui::CloseCurrentPopup();
      //   }

      //   if (btnUnused) {
      //      pal->colors[i] = EGA_COLOR_UNUSED;
      //      ImGui::CloseCurrentPopup();
      //   }
      //}

      ImGui::Text("Color: %d", *color);

      //ImGui::PopStyleVar();
      //ImGui::PopStyleColor(2);

      ImGui::EndPopup();
   }
}

void uiPaletteEditor(Window* wnd, EGAPalette* pal, char* palName, u32 palNameSize, PaletteEditorFlags flags) {

   // keep track of strage variables
   enum {
      STORE_PALNAME = 0,// void* (leaks)
      STORE_PALNAMESIZE,// int
      STORE_LOADOPENED, // bool
      STORE_LOADCURSOR  // int
   };

   auto imStore = ImGui::GetStateStorage();

   auto game = gameGet();
   auto &imStyle = ImGui::GetStyle();

   auto lnHeight = ImGui::GetTextLineHeight();
   ImVec2 size;

   float btnSize = lnHeight + imStyle.FramePadding.y;

   if (flags&PaletteEditorFlags_POPPED_OUT) {
      btnSize = MAX(btnSize, ImGui::GetContentRegionAvailWidth() / 16);
   }


   size.y = btnSize + ImGui::GetFrameHeightWithSpacing() * 2 + imStyle.WindowPadding.y * 2;
   size.x = btnSize * 16 + imStyle.WindowPadding.x * 2;

   bool doUi = true;

   if (!(flags&PaletteEditorFlags_POPPED_OUT)) {
      doUi = ImGui::BeginChild("PaletteEditor", size, true);
   }

   if (doUi) {

      if (!(flags&PaletteEditorFlags_POPPED_OUT)) {
         ImGui::AlignTextToFramePadding();
         ImGui::Text("Palette Editor");
      }

      // undefined and unused buttons in the top right if using an encoding flag
      if (flags&PaletteEditorFlags_ENCODE) {
         static StringView encButtons = ICON_FA_QUESTION ICON_FA_TIMES_CIRCLE;
         auto sz = ImGui::CalcTextSize(encButtons);
         float widgetWIdth = sz.x + imStyle.ItemSpacing.x + (imStyle.FramePadding.x * 2);
         ImGui::SameLine(ImGui::GetContentRegionAvailWidth() - widgetWIdth);//
         bool undef = ImGui::Button(ICON_FA_QUESTION);
         if (ImGui::BeginDragDropSource(0, 0)) {
            uiDragDropPalColor payload = { EGA_COLOR_UNDEFINED, EGA_PALETTE_COLORS };
            ImGui::SetDragDropPayload(UI_DRAGDROP_PALCOLOR, &payload, sizeof(payload), ImGuiCond_Once);
            ImGui::Button(ICON_FA_QUESTION, ImVec2(btnSize, btnSize));
            ImGui::EndDragDropSource();
         }
         if (ImGui::IsItemHovered()) ImGui::SetTooltip("Fill undefined");
         ImGui::SameLine();
         bool unused = ImGui::Button(ICON_FA_TIMES_CIRCLE);
         if (ImGui::BeginDragDropSource(0, 0)) {
            uiDragDropPalColor payload = { EGA_COLOR_UNUSED, EGA_PALETTE_COLORS };
            ImGui::SetDragDropPayload(UI_DRAGDROP_PALCOLOR, &payload, sizeof(payload), ImGuiCond_Once);
            ImGui::Button(ICON_FA_TIMES_CIRCLE, ImVec2(btnSize, btnSize));
            ImGui::EndDragDropSource();
         }
         if (ImGui::IsItemHovered()) ImGui::SetTooltip("Fill unused");

         if (undef) {
            memset(pal->colors, EGA_COLOR_UNDEFINED, 16);
         }
         if (unused) {
            memset(pal->colors, EGA_COLOR_UNUSED, 16);
         }
      }

      auto imItemSpacing = imStyle.ItemSpacing;
      auto imButtonColor = imStyle.Colors[ImGuiCol_Button];
      auto imButtonHoveredColor = imStyle.Colors[ImGuiCol_ButtonHovered];

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, imStyle.ItemSpacing.y));
      for (int i = 0; i < EGA_PALETTE_COLORS; ++i) {
         auto cval = pal->colors[i];
         auto pcolor = egaGetColor(cval);

         bool encodeVal = cval >= EGA_COLOR_UNDEFINED;

         ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(pcolor.r, pcolor.g, pcolor.b, encodeVal ? 0 : 255));
         ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(pcolor.r, pcolor.g, pcolor.b, encodeVal ? 0 : 128));

         const char* btnText = (cval == EGA_COLOR_UNDEFINED) ? ICON_FA_QUESTION : (cval == EGA_COLOR_UNUSED ? ICON_FA_TIMES_CIRCLE : "");

         ImGui::PushID(i);
         static const StringView cpickerLabel = "EGAColorPicker";

         auto btnPressed = ImGui::Button(btnText, ImVec2(btnSize, btnSize));

         if (ImGui::BeginDragDropSource(0, 0)) {
            uiDragDropPalColor payload = { pal->colors[i], i };
            ImGui::SetDragDropPayload(UI_DRAGDROP_PALCOLOR, &payload, sizeof(payload), ImGuiCond_Once);
            ImGui::Button(btnText, ImVec2(btnSize, btnSize));
            ImGui::EndDragDropSource();
         }
         if (ImGui::BeginDragDropTarget()) {
            if (auto payload = ImGui::AcceptDragDropPayload(UI_DRAGDROP_PALCOLOR)) {
               auto plData = (uiDragDropPalColor*)payload->Data;
               pal->colors[i] = plData->color;
            }
            ImGui::EndDragDropTarget();
         }
         if (ImGui::IsItemHovered()) ImGui::SetTooltip("Idx: %d\nCol: %d", i, pal->colors[i]);
         

         if (btnPressed) {
            if (ImGui::GetIO().KeyCtrl) {
               switch (pal->colors[i]) {
               case EGA_COLOR_UNDEFINED:
                  pal->colors[i] = EGA_COLOR_UNUSED;
                  break;
               default:
                  pal->colors[i] = EGA_COLOR_UNDEFINED;
                  break;
               }
            }
            else {
               ImGui::OpenPopup(cpickerLabel);
            }
         }

         uiPaletteColorPicker(cpickerLabel, &pal->colors[i]);

         ImGui::PopID();
         ImGui::PopStyleColor(2);
         ImGui::SameLine();
      }
      ImGui::NewLine();
      ImGui::PopStyleVar();

      if (!palName) {
         //not using a pre-allocated palName buffer so use window store
         // I guess this is fine that it leaks?? shrug
         

         auto storedPalName = imStore->GetVoidPtrRef(STORE_PALNAME);
         auto storedPalSize = imStore->GetIntRef(STORE_PALNAMESIZE);

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

      ImGui::SameLine();

      // load button
      auto btnLoad = ImGui::Button(ICON_FA_UPLOAD);
      if (ImGui::IsItemHovered()) ImGui::SetTooltip("Load");

      bool *loadOpened = imStore->GetBoolRef(STORE_LOADOPENED, false);

      if (btnLoad) {
         ImGui::OpenPopup("Load Palette");
         *loadOpened = true;
      }
      if (ImGui::BeginPopup("Load Palette")) {
         static char search[64] = { 0 };

         int *loadCursor = imStore->GetIntRef(STORE_LOADCURSOR, 0);

         if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
         }
         ImGui::InputText(ICON_FA_SEARCH, search, 64);

         auto pals = paletteList(game->assets.palettes, search);
         *loadCursor = MIN(*loadCursor, MAX(0, pals.size()-1));

         if (ImGui::BeginChild("List", ImVec2(ImGui::GetContentRegionAvailWidth(), ImGui::GetFrameHeightWithSpacing() * 5), true)) {

            if (pals.empty()) {
               ImGui::Text("No Palettes!");
            }
            else {
               auto &imStyle = ImGui::GetStyle();
               auto imBtnAlign = imStyle.ButtonTextAlign;
               int palIdx = 0;
               for (auto p : pals) {
                  ImGui::PushID(p.c_str());

                  bool btnDelete = ImGui::Button(ICON_FA_TRASH_ALT);
                  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delete");
                  if (btnDelete) {
                     ImGui::OpenPopup("Delete Confirm");
                  }

                  if (uiModalPopup("Delete Confirm", "Delete this palette?", uiModalTypes_YESNO, ICON_FA_EXCLAMATION_TRIANGLE) == uiModalResults_YES) {
                     paletteDelete(game->assets.palettes, p.c_str());
                  }

                  if (palIdx == *loadCursor) {
                     ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
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

                  if (palIdx == *loadCursor) {
                     ImGui::PopStyleColor();
                  }

                  ImGui::PopID();
                  ++palIdx;
               }

               if (ImGui::IsKeyPressed(SDL_SCANCODE_RETURN)) {
                  auto &p = pals[*loadCursor];
                  paletteLoad(game->assets.palettes, p.c_str(), pal);
                  strcpy(palName, p.c_str());
                  ImGui::CloseCurrentPopup();
               }

               if (ImGui::IsKeyPressed(SDL_SCANCODE_DOWN)) {
                  ++*loadCursor;
               }

               if (ImGui::IsKeyPressed(SDL_SCANCODE_UP)) {
                  --*loadCursor;
               }

               if (ImGui::IsKeyPressed(SDLK_ESCAPE)) {
                  ImGui::CloseCurrentPopup();
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

      // save button
      auto currentNameLen = strlen(palName);
      bool nameValid = currentNameLen != 0;

      if (!nameValid) {
         ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha*0.5f);
      }

      bool palExists = paletteExists(game->assets.palettes, palName);
      auto btnSave = ImGui::Button(ICON_FA_DOWNLOAD);

      static StringView ttip_save = "Save";
      static StringView ttip_saveinvalid = "Save (File name can't be empty)";
      if (ImGui::IsItemHovered()) ImGui::SetTooltip(nameValid ? ttip_save : ttip_saveinvalid);

      if (btnSave && nameValid) {
         if (palExists) {
            ImGui::OpenPopup("Save Confirm");
         }
         else {
            paletteSave(game->assets.palettes, palName, pal);
            ImGui::OpenPopup("Saved");
         }
      }
      if (uiModalPopup("Save Confirm", "Palette Exists\nOverwrite?", uiModalTypes_YESNO, ICON_FA_EXCLAMATION) == uiModalResults_YES) {
         paletteSave(game->assets.palettes, palName, pal);
         ImGui::OpenPopup("Saved");
      }
      uiModalPopup("Saved", "Palette Saved!", uiModalTypes_OK, ICON_FA_CHECK);

      if (!nameValid) {
         ImGui::PopStyleVar();
      }

      ImGui::SameLine();

      // refresh button
      if (!palExists) {
         ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha*0.5f);
      }

      auto btnRefresh = ImGui::Button(ICON_FA_SYNC_ALT);
      static StringView ttip_refresh = "Refresh";
      static StringView ttip_refreshinvalid = "Refresh (palette not found)";
      if (ImGui::IsItemHovered()) ImGui::SetTooltip(palExists ? ttip_refresh : ttip_refreshinvalid);
      if (btnRefresh && palExists) {
         paletteLoad(game->assets.palettes, palName, pal);
      }

      if (!palExists) {
         ImGui::PopStyleVar();
      }

      // popout button
      if (!(flags&PaletteEditorFlags_POPPED_OUT)) {
         ImGui::SameLine();
         auto btnPopOut = ImGui::Button(ICON_FA_EXTERNAL_LINK_ALT);
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

uiModalResult uiModalPopup(StringView title, StringView msg, uiModalType type, StringView icon) {
   uiModalResult result = uiModalResults_CLOSED;

   if (ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      result = uiModalResults_OPEN;

      if (icon) {
         ImGui::Text(icon);
         ImGui::SameLine();
      }

      ImGui::Text(msg);

      switch (type) {
      case uiModalTypes_YESNO: {
         bool yes = ImGui::Button("Yes");
         ImGui::SameLine();
         bool no = ImGui::Button("No");

         if (ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
            no = true;
         }

         if (yes) { result = uiModalResults_YES; }
         else if (no) { result = uiModalResults_NO; }
         break; }

      case uiModalTypes_YESNOCANCEL: {
         bool yes = ImGui::Button("Yes");
         ImGui::SameLine();
         bool no = ImGui::Button("No");
         ImGui::SameLine();
         bool cancel = ImGui::Button("Cancel");

         if (ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
            cancel = true;
         }

         if (yes) { result = uiModalResults_YES; }
         else if (no) { result = uiModalResults_NO; }
         else if (cancel) { result = uiModalResults_CANCEL; }
         break; }

      case uiModalTypes_OK: {
         bool ok = ImGui::Button("OK");

         if (ImGui::IsKeyPressed(SDL_SCANCODE_RETURN) ||
             ImGui::IsKeyPressed(SDL_SCANCODE_SPACE)) {
            ok = true;
         }

         if (ok) { result = uiModalResults_OK; }
         break; }

      case uiModalTypes_OKCANCEL: {
         bool ok = ImGui::Button("OK");
         ImGui::SameLine();
         bool cancel = ImGui::Button("Cancel");

         if (ImGui::IsKeyPressed(SDL_SCANCODE_RETURN) ||
            ImGui::IsKeyPressed(SDL_SCANCODE_SPACE)) {
            ok = true;
         }
         if (ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
            cancel = true;
         }

         if (ok) { result = uiModalResults_OK; }
         else if (cancel) { result = uiModalResults_CANCEL; }
         break; }
      }

      if (result != uiModalResults_OPEN) {
         ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();       
   }

   return result;
}


static void _showWindowedViewer(Window* wnd) {
   auto game = gameGet();

   auto sz = windowSize(wnd);
   ImGui::SetNextWindowSize(ImVec2(sz.x/2.0f, sz.y/2.0f), ImGuiCond_Appearing);
   static Int2 viewersz = { (i32)(EGA_RES_WIDTH * EGA_PIXEL_WIDTH), (i32)(EGA_RES_HEIGHT * EGA_PIXEL_HEIGHT) };

   if (ImGui::Begin("Viewer", nullptr, 0)) {

      ImGui::Columns(2);

      //if (ImGui::IsWindowAppearing()) {
         auto &imStyle = ImGui::GetStyle();
         ImGui::SetColumnWidth(0, uiPaletteEditorWidth() + imStyle.WindowPadding.x * 2);
      //}

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

      if (ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
         windowClose(wnd);
      }


   }
   
}

