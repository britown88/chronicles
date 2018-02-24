#include "app.h"
#include "game.h"
#include "ega.h"

#include <imgui.h>
#include <SDL2/SDL.h>

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

static void _mainMenu(GameData* game, Window* wnd) {
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


         ImGui::EndMenu();
      }

      ImGui::EndMainMenuBar();
   }
}

static Recti _getProportionallyFitRect(Int2 srcSize, Int2 destSize) {
   float rw = (float)destSize.x;
   float rh = (float)destSize.y;
   float cw = (float)srcSize.x;
   float ch = (float)srcSize.y;

   float ratio = MIN(rw / cw, rh / ch);

   Recti out = { 0, 0, (i32)(cw * ratio), (i32)(ch * ratio) };
   out.x += (i32)((rw - out.w) / 2.0f);
   out.y += (i32)((rh - out.h) / 2.0f);

   return out;
}

static void _renderViewerTexture(Texture* texture, Int2 srcSize) {
   if (!texture) {
      return;
   }

   auto sz = ImGui::GetContentRegionAvail();

   auto rect = _getProportionallyFitRect(srcSize, { (i32)sz.x, (i32)sz.y });

   ImDrawList* draw_list = ImGui::GetWindowDrawList();
   const ImVec2 p = ImGui::GetCursorScreenPos();

   draw_list->AddImage(
      (ImTextureID)textureGetHandle(texture), 
      ImVec2(p.x + rect.x, p.y + rect.y), 
      ImVec2(p.x + rect.x + rect.w, p.y + rect.y + rect.h));
}

static void _showFullScreenViewer(GameData* game, Window* wnd) {
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

static void _showWindowedViewer(GameData* game, Window* wnd) {

   auto sz = windowSize(wnd);
   ImGui::SetNextWindowSize(ImVec2(sz.x/2.0f, sz.y/2.0f), ImGuiCond_Appearing);
   static Int2 viewersz = { 100,100 };

   if (ImGui::Begin("Viewer", nullptr, 0)) {
      _renderViewerTexture(game->primaryView.texture, viewersz);

      if (_imWindowContextMenu("Viewer Context", 1)) {
         if (ImGui::Selectable("Edit Size")) {
            windowAddGUI(wnd, "viewerContextResize", [&](Window*wnd) mutable {
               bool p_open = true;
               ImGui::OpenPopup("Change Viewer Size");
               if (ImGui::BeginPopupModal("Change Viewer Size", NULL, ImGuiWindowFlags_AlwaysAutoResize))
               {
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


void gameDoUI(GameData* game, Window* wnd) {

   if (ImGui::IsKeyPressed(SDL_SCANCODE_F1)) {
      game->imgui.showUI = !game->imgui.showUI;
   }

   if (game->imgui.showUI) {
      _mainMenu(game, wnd);
      _doStatsWindow(wnd);
      _showWindowedViewer(game, wnd);
   }
   else {
      _showFullScreenViewer(game, wnd);
   }
   
}

